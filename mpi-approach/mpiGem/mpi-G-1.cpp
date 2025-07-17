#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring> // For strcpy
#include <iomanip> // For std::fixed and std::setprecision

// Enum and constants remain the same
enum FlowerType
{
    ROSE = 0,
    SUNFLOWER = 1,
    TULIP = 2
};
const char *FlowerNames[3] = {"Rose", "Sunflower", "Tulip"};
const int NUM_ROUNDS = 3;

// Structs for Seller, Order, TradeResult remain the same,
// but ensure they are "MPI-friendly" (no complex data structures like std::string or std::vector inside)
struct Seller
{
    char name[20];
    int quantity[3];
    double price[3];
};

struct Order
{
    int buyerId;    // Original ID of the logical buyer (1-20)
    int senderRank; // MPI rank of the process that sent this order
    int demand[3];
    double budget;
};

struct TradeResult
{
    int buyerId; // Original ID of the logical buyer (1-20)
    int fulfilled[3];
    double remaining_budget;
};

// MPI Message Tags
#define ORDER_TAG 100
#define RESULT_TAG 101

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        if (rank == 0)
            std::cout << "Run with at least 2 processes (1 master + 1+ buyers).\n";
        MPI_Finalize();
        return 0;
    }

    // --- Define all 20 logical buyers (accessible to all for consistent assignment) ---
    // In a real scenario, this might be loaded from a file or a database.
    // For simplicity, we define it once and each process determines its share.
    std::vector<Order> all_logical_buyers = {
        {1, 0, {5, 3, 2}, 50.0}, {2, 0, {6, 4, 3}, 60.0}, {3, 0, {7, 2, 2}, 75.0}, {4, 0, {4, 5, 3}, 55.0}, {5, 0, {8, 1, 4}, 80.0}, {6, 0, {5, 3, 2}, 65.0}, {7, 0, {6, 4, 3}, 70.0}, {8, 0, {7, 2, 2}, 85.0}, {9, 0, {4, 5, 3}, 60.0}, {10, 0, {8, 1, 4}, 90.0}, {11, 0, {5, 3, 2}, 52.0}, {12, 0, {6, 4, 3}, 62.0}, {13, 0, {7, 2, 2}, 77.0}, {14, 0, {4, 5, 3}, 57.0}, {15, 0, {8, 1, 4}, 82.0}, {16, 0, {5, 3, 2}, 67.0}, {17, 0, {6, 4, 3}, 72.0}, {18, 0, {7, 2, 2}, 87.0}, {19, 0, {4, 5, 3}, 62.0}, {20, 0, {8, 1, 4}, 92.0}};
    const int NUM_LOGICAL_BUYERS = all_logical_buyers.size();

    // --- Determine which logical buyers each worker process will handle ---
    int num_worker_processes = size - 1; // Ranks 1 to (size-1) are workers
    std::vector<Order> my_assigned_buyers;
    if (rank != 0)
    { // Only worker processes need to know their assigned buyers
        int buyers_per_worker = NUM_LOGICAL_BUYERS / num_worker_processes;
        int remaining_buyers = NUM_LOGICAL_BUYERS % num_worker_processes;

        int start_idx = (rank - 1) * buyers_per_worker;
        // Distribute remaining buyers to the first 'remaining_buyers' workers
        if (rank - 1 < remaining_buyers)
        {
            start_idx += (rank - 1);
        }
        else
        {
            start_idx += remaining_buyers;
        }

        int end_idx = start_idx + buyers_per_worker;
        if (rank - 1 < remaining_buyers)
        {
            end_idx++; // This worker gets one extra buyer
        }

        for (int i = start_idx; i < end_idx; ++i)
        {
            my_assigned_buyers.push_back(all_logical_buyers[i]);
            // Store original budget/demand as they change with 'result'
            // In a real scenario, you'd probably store initial demand/budget separately for each round.
        }
    }

    // --- Main Simulation Loop ---
    for (int round = 1; round <= NUM_ROUNDS; ++round)
    {
        if (rank == 0)
        {
            // ---------------- Master Process ----------------
            std::cout << "\n========================================\n";
            std::cout << "🔁 ROUND " << round << " STARTS 🔁\n";
            std::cout << "========================================\n";

            // Master's seller data (only master holds this)
            // Initialized once, then modified across rounds
            static std::vector<Seller> sellers = {// Use static to persist across rounds
                                                  {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
                                                  {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
                                                  {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

            // Track if each seller sold a particular flower type in this round
            std::vector<std::vector<bool>> flowerSold(sellers.size(), std::vector<bool>(3, false));

            // Master receives orders from all worker processes
            for (int i = 0; i < NUM_LOGICAL_BUYERS; ++i)
            {
                Order received_order;
                MPI_Status status;
                // Receive from any source (MPI_ANY_SOURCE) to allow workers to send orders in any order
                MPI_Recv(&received_order, sizeof(Order), MPI_BYTE, MPI_ANY_SOURCE, ORDER_TAG, MPI_COMM_WORLD, &status);

                TradeResult result = {{0, 0, 0}, received_order.budget}; // Initialize result for this order

                // --- Trade Processing Logic (same as before) ---
                for (int flower = 0; flower < 3; ++flower)
                {
                    int needed = received_order.demand[flower]; // Start with full demand for this flower

                    for (int s = 0; s < sellers.size(); ++s)
                    {
                        Seller &seller = sellers[s];
                        if (needed <= 0)
                            break;

                        int affordable = (seller.price[flower] > 0) ? static_cast<int>(result.remaining_budget / seller.price[flower]) : 0;
                        int available = seller.quantity[flower];
                        int buying = std::min({needed, available, affordable});

                        if (buying > 0)
                        {
                            double cost = buying * seller.price[flower];
                            result.fulfilled[flower] += buying;
                            result.remaining_budget -= cost;
                            seller.quantity[flower] -= buying;
                            needed -= buying; // Update remaining needed for THIS flower from THIS order
                            flowerSold[s][flower] = true;

                            std::cout << "Master: Buyer " << received_order.buyerId << " bought " << buying << " "
                                      << FlowerNames[flower] << "(s) from " << seller.name
                                      << " for $" << std::fixed << std::setprecision(2) << cost << "\n";
                        }
                    }
                }
                // --- End Trade Processing Logic ---

                result.buyerId = received_order.buyerId; // Pass buyer ID back with result
                MPI_Send(&result, sizeof(TradeResult), MPI_BYTE, status.MPI_SOURCE, RESULT_TAG, MPI_COMM_WORLD);
            }

            // Master waits for all workers to finish their sends/receives before price adjustment
            MPI_Barrier(MPI_COMM_WORLD);

            // 🔽 Price Drop Logic: If flower was not sold, reduce price by 20%
            std::cout << "\n--- Price Adjustments for Round " << round << " ---\n";
            for (int s = 0; s < sellers.size(); ++s)
            {
                Seller &seller = sellers[s];
                for (int f = 0; f < 3; ++f)
                {
                    if (!flowerSold[s][f] && seller.quantity[f] > 0)
                    {
                        double old_price = seller.price[f];
                        seller.price[f] *= 0.8; // drop 20%
                        std::cout << "  ⚠️ Price Drop: " << seller.name << "'s " << FlowerNames[f]
                                  << " price dropped from $" << std::fixed << std::setprecision(2) << old_price
                                  << " to $" << std::fixed << std::setprecision(2) << seller.price[f] << "\n";
                    }
                }
            }
        }
        else
        {
            // ---------------- Buyer Processes (Workers) ----------------
            std::cout << "\n🛒 Buyer Process " << rank << " - ROUND " << round << " STARTS 🛒\n";

            // Each worker processes its assigned logical buyers
            for (const auto &initial_buyer_data : my_assigned_buyers)
            {
                Order myOrder = initial_buyer_data; // Copy initial data
                myOrder.senderRank = rank;          // Important: Identify sender rank

                // Send the order to the master
                MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, ORDER_TAG, MPI_COMM_WORLD);

                TradeResult result;
                // Receive the trade result back from the master
                MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                std::cout << "  🛒 Buyer " << result.buyerId << " (Process " << rank << ") - ROUND " << round << " Result:\n";
                for (int i = 0; i < 3; ++i)
                    std::cout << "    " << result.fulfilled[i] << " " << FlowerNames[i] << "(s)\n";
                std::cout << "    Budget left: $" << std::fixed << std::setprecision(2) << result.remaining_budget << "\n";
            }
            // All workers wait here before the master performs price adjustments
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    if (rank == 0)
    {
        // Final summary by the master (only if needed, as sellers are static)
        // If sellers were dynamic, this is where master would gather and report final state.
        // For this example, sellers are static and their final state is just after price drops in the last round.
        std::cout << "\n========================================\n";
        std::cout << "🕒 Simulation Finished after " << NUM_ROUNDS << " Rounds\n";
        std::cout << "========================================\n";
    }

    MPI_Finalize();
    return 0;
}