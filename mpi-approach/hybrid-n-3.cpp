#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

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
    int quantity[3];
    double price[3];
};

struct Buyer
{
    char name[20];
    int demand[3];
    double budget;
    double buy_price[3];
};

struct Trade
{
    int buyer_id;
    int flower_type;
    int seller_id;
    int quantity;
    double total_cost;
};

bool demandsLeft(const Buyer &buyer)
{
    for (int i = 0; i < 3; ++i)
        if (buyer.demand[i] > 0)
            return true;
    return false;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        if (rank == 0)
        {
            std::cout << "Need at least 2 processes (1 manager + 1 worker)\n";
        }
        MPI_Finalize();
        return 1;
    }

    std::vector<Seller> sellers;

    // Master list of 23 buyers
    std::vector<Buyer> allBuyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}}, {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}}, {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}}, {"Ben", {10, 0, 5}, 350, {4.5, 0.0, 5.0}}, {"Lia", {2, 2, 2}, 100, {4.0, 4.0, 4.0}}, {"Joe", {5, 10, 5}, 400, {5.0, 5.0, 5.0}}, {"Sue", {5, 5, 5}, 200, {4.5, 4.5, 4.5}}, {"Amy", {1, 1, 1}, 50, {3.0, 3.0, 3.0}}, {"Tim", {4, 6, 3}, 250, {4.5, 4.5, 5.0}}, {"Sam", {7, 8, 4}, 600, {5.0, 5.0, 5.0}}, {"Jill", {3, 4, 5}, 200, {4.0, 4.5, 5.0}}, {"Zoe", {6, 3, 7}, 300, {4.0, 5.0, 5.5}}, {"Max", {5, 5, 5}, 250, {4.5, 4.5, 4.5}}, {"Ivy", {8, 6, 4}, 550, {5.0, 5.0, 5.0}}, {"Leo", {9, 0, 2}, 350, {4.2, 0.0, 5.0}}, {"Kim", {3, 3, 3}, 180, {4.0, 4.0, 4.0}}, {"Tom", {6, 5, 3}, 400, {4.8, 4.8, 5.0}}, {"Nina", {4, 2, 6}, 280, {4.0, 4.0, 5.0}}, {"Ray", {3, 5, 4}, 300, {4.5, 4.5, 4.5}}, {"Liv", {5, 3, 2}, 250, {4.0, 4.0, 4.5}}, {"Oli", {6, 6, 6}, 450, {5.0, 5.0, 5.0}}, {"Ken", {2, 2, 2}, 100, {3.5, 3.5, 3.5}}, {"Ana", {7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    // Manager initializes sellers
    if (rank == 0)
    {
        sellers = {
            {"Alice", {100, 100, 100}, {6.0, 5.5, 7.0}},
            {"Bob", {100, 100, 100}, {5.5, 5.2, 6.5}},
            {"Charlie", {100, 100, 100}, {6.8, 5.0, 7.5}}};
    }
    else
    {
        sellers.resize(3); // Initialize for worker processes
    }

    // Distribute buyers across worker processes (rank 0 is manager)
    std::vector<Buyer> myBuyers;
    if (rank != 0)
    {
        for (size_t i = rank - 1; i < allBuyers.size(); i += (size - 1))
        {
            myBuyers.push_back(allBuyers[i]);
        }
        std::cout << "[Rank " << rank << "] Assigned " << myBuyers.size() << " buyers\n";
    }

    bool global_done = false;
    int round = 0;
    const int MAX_ROUNDS = 50; // Prevent infinite loops

    while (!global_done && round < MAX_ROUNDS)
    {
        round++;
        std::cout << "[Rank " << rank << "] Starting round " << round << "\n";

        // Step 1: Broadcast sellers from manager to all workers
        for (int i = 0; i < 3; ++i)
        {
            MPI_Bcast(&sellers[i], sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);
        }

        // Step 2: Workers process their buyers
        std::vector<Trade> trades;

        if (rank != 0)
        {
// Use OpenMP to parallelize buyer processing
#pragma omp parallel
            {
                std::vector<Trade> local_trades;

#pragma omp for
                for (int b = 0; b < (int)myBuyers.size(); ++b)
                {
                    for (int f = 0; f < 3; ++f)
                    {
                        if (myBuyers[b].demand[f] > 0)
                        {
                            // Find best seller for this flower type
                            int best_seller = -1;
                            double best_price = 999999.0;

                            for (int s = 0; s < 3; ++s)
                            {
                                if (sellers[s].quantity[f] > 0 &&
                                    sellers[s].price[f] <= myBuyers[b].buy_price[f] &&
                                    sellers[s].price[f] < best_price)
                                {
                                    best_seller = s;
                                    best_price = sellers[s].price[f];
                                }
                            }

                            if (best_seller >= 0)
                            {
                                int max_affordable = (int)(myBuyers[b].budget / sellers[best_seller].price[f]);
                                int qty = std::min({sellers[best_seller].quantity[f],
                                                    myBuyers[b].demand[f],
                                                    max_affordable});

                                if (qty > 0)
                                {
                                    double cost = qty * sellers[best_seller].price[f];

                                    // Create trade record
                                    Trade trade;
                                    trade.buyer_id = b;
                                    trade.flower_type = f;
                                    trade.seller_id = best_seller;
                                    trade.quantity = qty;
                                    trade.total_cost = cost;
                                    local_trades.push_back(trade);

                                    // Update buyer locally
                                    myBuyers[b].demand[f] -= qty;
                                    myBuyers[b].budget -= cost;
                                }
                            }
                        }
                    }
                }

#pragma omp critical
                {
                    trades.insert(trades.end(), local_trades.begin(), local_trades.end());
                }
            }

            // Print trades for this rank
            for (const auto &trade : trades)
            {
                std::cout << "[Rank " << rank << "] " << myBuyers[trade.buyer_id].name
                          << " wants " << trade.quantity << " " << FlowerNames[trade.flower_type]
                          << "(s) from " << sellers[trade.seller_id].name
                          << " for $" << trade.total_cost << "\n";
            }
        }

        // Step 3: Send trades to manager
        if (rank != 0)
        {
            int num_trades = trades.size();
            MPI_Send(&num_trades, 1, MPI_INT, 0, 100, MPI_COMM_WORLD);

            if (num_trades > 0)
            {
                MPI_Send(trades.data(), num_trades * sizeof(Trade), MPI_BYTE, 0, 101, MPI_COMM_WORLD);
            }
        }

        // Step 4: Manager receives and processes trades
        if (rank == 0)
        {
            for (int r = 1; r < size; ++r)
            {
                int num_trades;
                MPI_Recv(&num_trades, 1, MPI_INT, r, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (num_trades > 0)
                {
                    std::vector<Trade> received_trades(num_trades);
                    MPI_Recv(received_trades.data(), num_trades * sizeof(Trade), MPI_BYTE, r, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    // Process trades
                    for (const auto &trade : received_trades)
                    {
                        if (trade.seller_id >= 0 && trade.seller_id < 3)
                        {
                            sellers[trade.seller_id].quantity[trade.flower_type] -= trade.quantity;
                            if (sellers[trade.seller_id].quantity[trade.flower_type] < 0)
                                sellers[trade.seller_id].quantity[trade.flower_type] = 0;
                        }
                    }
                }
            }

            // Decrease prices for unsold flowers
            for (auto &seller : sellers)
            {
                for (int f = 0; f < 3; ++f)
                {
                    if (seller.price[f] > 0.2)
                        seller.price[f] -= 0.2;
                }
            }

            std::cout << "\n--- Round " << round << " completed ---\n";
            std::cout << "[Manager] Current seller stocks:\n";
            for (auto &s : sellers)
            {
                std::cout << s.name << ": ";
                for (int f = 0; f < 3; ++f)
                    std::cout << FlowerNames[f] << "=" << s.quantity[f] << " ";
                std::cout << "\n";
            }
        }

        // Step 5: Check if all buyers are done
        int local_done = 1;
        if (rank != 0)
        {
            for (const auto &b : myBuyers)
            {
                if (demandsLeft(b))
                {
                    local_done = 0;
                    break;
                }
            }
        }

        // Synchronize all processes before collective operation
        MPI_Barrier(MPI_COMM_WORLD);

        int global_done_int;
        MPI_Allreduce(&local_done, &global_done_int, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        global_done = (global_done_int == 1);

        std::cout << "[Rank " << rank << "] Round " << round << " - Local done: " << local_done << ", Global done: " << global_done << "\n";

        // Small delay to prevent overwhelming output
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    double end_time = MPI_Wtime();

    // Final status
    if (rank != 0)
    {
        for (const auto &b : myBuyers)
        {
            std::cout << "[Rank " << rank << "] ✅ " << b.name
                      << " finished with $" << b.budget << " left, demands: "
                      << b.demand[0] << "/" << b.demand[1] << "/" << b.demand[2] << "\n";
        }
    }

    if (rank == 0)
    {
        std::cout << "\n📊 Final Seller Stocks:\n";
        for (const auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int f = 0; f < 3; ++f)
                std::cout << FlowerNames[f] << "=" << s.quantity[f] << " ";
            std::cout << "\n";
        }
        std::cout << "\n⏱️ Total Time: " << end_time - start_time << " seconds\n";
        std::cout << "Total rounds: " << round << "\n";
    }

    MPI_Finalize();
    return 0;
}