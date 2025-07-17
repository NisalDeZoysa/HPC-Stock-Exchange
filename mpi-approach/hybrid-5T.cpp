#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <mutex>

enum FlowerType
{
    ROSE = 0,
    SUNFLOWER = 1,
    TULIP = 2
};
const char *FlowerNames[3] = {"Rose", "Sunflower", "Tulip"};

struct Seller
{
    char name[20];
    std::atomic<int> quantity[3]; // Thread-safe quantities
    double price[3];
    std::mutex price_mutex; // For price updates
};

struct Buyer
{
    char name[20];
    int demand[3];
    double budget;
    double buy_price[3];
};

struct TradeResult
{
    int total_trades;
    bool any_demands_left;
    double total_volume;
};

// Batch processing function - processes multiple buyers at once
TradeResult processBuyerBatch(std::vector<Buyer> &buyers, std::vector<Seller> &sellers,
                              int start_idx, int end_idx)
{
    TradeResult result = {0, false, 0.0};

#pragma omp parallel for reduction(+ : result.total_trades, result.total_volume)
    for (int b = start_idx; b < end_idx; ++b)
    {
        for (int f = 0; f < 3; ++f)
        {
            if (buyers[b].demand[f] > 0)
            {
                result.any_demands_left = true;

                // Find best seller atomically
                int best_seller = -1;
                double best_price = 999999.0;

                for (int s = 0; s < 3; ++s)
                {
                    int current_qty = sellers[s].quantity[f].load();
                    if (current_qty > 0 &&
                        sellers[s].price[f] <= buyers[b].buy_price[f] &&
                        sellers[s].price[f] < best_price)
                    {
                        best_seller = s;
                        best_price = sellers[s].price[f];
                    }
                }

                if (best_seller >= 0)
                {
                    int max_affordable = (int)(buyers[b].budget / sellers[best_seller].price[f]);
                    int desired_qty = std::min({buyers[b].demand[f], max_affordable});

                    // Atomic transaction - try to reserve quantity
                    int current_qty = sellers[best_seller].quantity[f].load();
                    int actual_qty = std::min(desired_qty, current_qty);

                    if (actual_qty > 0)
                    {
                        // Use compare_exchange to ensure atomic update
                        int expected = current_qty;
                        while (actual_qty > 0 &&
                               !sellers[best_seller].quantity[f].compare_exchange_weak(
                                   expected, expected - actual_qty))
                        {
                            // Retry with updated quantity
                            actual_qty = std::min(desired_qty, expected);
                        }

                        if (actual_qty > 0)
                        {
                            double cost = actual_qty * sellers[best_seller].price[f];
                            buyers[b].demand[f] -= actual_qty;
                            buyers[b].budget -= cost;

                            result.total_trades++;
                            result.total_volume += cost;
                        }
                    }
                }
            }
        }
    }

    return result;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Initialize sellers with atomic quantities
    std::vector<Seller> sellers(3);
    if (rank == 0)
    {
        strcpy(sellers[0].name, "Alice");
        strcpy(sellers[1].name, "Bob");
        strcpy(sellers[2].name, "Charlie");

        // Initialize atomic quantities
        for (int i = 0; i < 3; ++i)
        {
            sellers[i].quantity[0] = 1000; // More inventory for better parallelization
            sellers[i].quantity[1] = 1000;
            sellers[i].quantity[2] = 1000;
        }

        sellers[0].price[0] = 6.0;
        sellers[0].price[1] = 5.5;
        sellers[0].price[2] = 7.0;
        sellers[1].price[0] = 5.5;
        sellers[1].price[1] = 5.2;
        sellers[1].price[2] = 6.5;
        sellers[2].price[0] = 6.8;
        sellers[2].price[1] = 5.0;
        sellers[2].price[2] = 7.5;
    }

    // Replicate sellers to all processes (shared memory approach)
    MPI_Bcast(sellers.data(), 3 * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);

    // Scale up buyers for better parallelization
    std::vector<Buyer> buyers;
    if (rank == 0)
    {
        // Create more buyers by replicating the original set
        std::vector<Buyer> base_buyers = {
            {"Dan", {50, 25, 10}, 2500, {4.0, 4.0, 5.0}},
            {"Eve", {25, 25, 0}, 1500, {3.5, 3.5, 0.0}},
            {"Fay", {75, 50, 25}, 5000, {5.0, 4.5, 5.5}},
            // ... add more buyers with scaled demands
        };

        // Replicate buyers to create larger workload
        for (int rep = 0; rep < 10; ++rep)
        {
            for (auto &buyer : base_buyers)
            {
                Buyer new_buyer = buyer;
                snprintf(new_buyer.name, 20, "%s_%d", buyer.name, rep);
                buyers.push_back(new_buyer);
            }
        }
    }

    // Distribute buyers across processes
    int buyers_per_process = buyers.size() / size;
    int start_idx = rank * buyers_per_process;
    int end_idx = (rank == size - 1) ? buyers.size() : (rank + 1) * buyers_per_process;

    std::vector<Buyer> my_buyers(buyers.begin() + start_idx, buyers.begin() + end_idx);

    bool global_done = false;
    int round = 0;
    const int MAX_ROUNDS = 50;

    while (!global_done && round < MAX_ROUNDS)
    {
        round++;

        // Process buyers in batches with reduced synchronization
        TradeResult result = processBuyerBatch(my_buyers, sellers, 0, my_buyers.size());

        // Less frequent synchronization - only every 5 rounds
        if (round % 5 == 0)
        {
            // Update prices periodically
            if (rank == 0)
            {
                for (auto &seller : sellers)
                {
                    std::lock_guard<std::mutex> lock(seller.price_mutex);
                    for (int f = 0; f < 3; ++f)
                    {
                        if (seller.price[f] > 0.1)
                        {
                            seller.price[f] -= 0.1;
                        }
                    }
                }
            }

            // Broadcast updated prices
            MPI_Bcast(sellers.data(), 3 * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);

            // Check global completion less frequently
            int local_done = result.any_demands_left ? 0 : 1;
            int global_done_int;
            MPI_Allreduce(&local_done, &global_done_int, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
            global_done = (global_done_int == 1);
        }

        // Remove artificial delays
        // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // REMOVED
    }

    double end_time = MPI_Wtime();
    double total_time = end_time - start_time;

    if (rank == 0)
    {
        std::cout << "Optimized Parallel Time: " << total_time << " seconds\n";
        std::cout << "Total rounds: " << round << "\n";
        std::cout << "Average time per round: " << (total_time / round) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}