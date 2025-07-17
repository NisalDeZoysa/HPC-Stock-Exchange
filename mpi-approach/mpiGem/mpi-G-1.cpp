#include <mpi.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iomanip> // For std::fixed and std::setprecision

// Enum for flower types
enum FlowerType
{
    ROSE = 0,
    SUNFLOWER = 1,
    TULIP = 2
};

// Array of flower names for easy printing
const char *FlowerNames[3] = {"Rose", "Sunflower", "Tulip"};

// Number of trading rounds for the simulation
const int NUM_ROUNDS = 3;

// Structure to represent a seller
struct Seller
{
    char name[20];   // Seller's name
    int quantity[3]; // Quantity of each flower type available
    double price[3]; // Price of each flower type
};

// Structure to represent a buyer's order
struct Order
{
    int buyerId;   // Unique ID for the buyer
    int demand[3]; // Demand for each flower type
    double budget; // Buyer's total budget
};

// Structure to store the result of a trade for a buyer
struct TradeResult
{
    int fulfilled[3];        // Quantity of each flower type successfully bought
    double remaining_budget; // Remaining budget after purchases
};

int main(int argc, char **argv)
{
    // Initialize MPI environment
    MPI_Init(&argc, &argv);
    int rank, size;
    // Get the rank of the process and total number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure the program is run with exactly one process for this centralized simulation
    if (size != 1)
    {
        if (rank == 0)
            std::cout << "⚠️ This program is designed to run with exactly 1 MPI process (mpirun -np 1).\n";
        MPI_Finalize();
        return 0;
    }

    // This block contains all the simulation logic, running on the single process (rank 0)
    if (rank == 0)
    {
        // ---------------- Master Process (Centralized Simulation) ----------------

        // Define the sellers and their initial stock/prices
        std::vector<Seller> sellers = {
            {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
            {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
            {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

        // Define the 20 pre-defined buyers and their initial demands/budgets
        std::vector<Order> buyers = {
            {1, {5, 3, 2}, 50.0}, {2, {6, 4, 3}, 60.0}, {3, {7, 2, 2}, 75.0}, {4, {4, 5, 3}, 55.0}, {5, {8, 1, 4}, 80.0}, {6, {5, 3, 2}, 65.0}, {7, {6, 4, 3}, 70.0}, {8, {7, 2, 2}, 85.0}, {9, {4, 5, 3}, 60.0}, {10, {8, 1, 4}, 90.0}, {11, {5, 3, 2}, 52.0}, {12, {6, 4, 3}, 62.0}, {13, {7, 2, 2}, 77.0}, {14, {4, 5, 3}, 57.0}, {15, {8, 1, 4}, 82.0}, {16, {5, 3, 2}, 67.0}, {17, {6, 4, 3}, 72.0}, {18, {7, 2, 2}, 87.0}, {19, {4, 5, 3}, 62.0}, {20, {8, 1, 4}, 92.0}};

        // Record the start time of the simulation
        double start_time = MPI_Wtime();

        // Loop through each trading round
        for (int round = 1; round <= NUM_ROUNDS; ++round)
        {
            std::cout << "\n========================================\n";
            std::cout << "🔁 ROUND " << round << " STARTS 🔁\n";
            std::cout << "========================================\n";

            // Track if each seller sold a particular flower type in this round
            std::vector<std::vector<bool>> flowerSold(sellers.size(), std::vector<bool>(3, false));

            // Process orders from each pre-defined buyer
            for (int b = 0; b < buyers.size(); ++b)
            {
                Order currentOrder = buyers[b];                        // Get the buyer's order for this round
                TradeResult result = {{0, 0, 0}, currentOrder.budget}; // Initialize trade result for this buyer

                std::cout << "\n--- Processing Order for Buyer " << currentOrder.buyerId << " ---\n";
                std::cout << "  Initial Demand: Rose: " << currentOrder.demand[ROSE]
                          << ", Sunflower: " << currentOrder.demand[SUNFLOWER]
                          << ", Tulip: " << currentOrder.demand[TULIP]
                          << ", Budget: $" << std::fixed << std::setprecision(2) << currentOrder.budget << "\n";

                // Iterate through each flower type to fulfill demand
                for (int flower = 0; flower < 3; ++flower)
                {
                    // Corrected: 'needed' should be the original demand minus what has already been fulfilled for this flower
                    int needed = currentOrder.demand[flower] - result.fulfilled[flower];

                    // Iterate through each seller to find available flowers
                    for (int s = 0; s < sellers.size(); ++s)
                    {
                        Seller &seller = sellers[s]; // Reference to the current seller
                        if (needed <= 0)
                            break; // If demand for this flower is met, move to the next seller or flower type

                        // Calculate how many flowers are affordable based on remaining budget
                        int affordable = (seller.price[flower] > 0) ? static_cast<int>(result.remaining_budget / seller.price[flower]) : 0;
                        // Quantity available from the seller
                        int available = seller.quantity[flower];

                        // Determine the actual quantity to buy (minimum of needed, available, and affordable)
                        int buying = std::min({needed, available, affordable});

                        if (buying > 0)
                        {
                            double cost = buying * seller.price[flower]; // Calculate the cost of the purchase
                            result.fulfilled[flower] += buying;          // Add to fulfilled quantity for the buyer
                            result.remaining_budget -= cost;             // Deduct cost from buyer's budget
                            seller.quantity[flower] -= buying;           // Reduce seller's stock
                            needed -= buying;                            // IMPORTANT: Reduce buyer's remaining needed for this flower

                            flowerSold[s][flower] = true; // Mark that this flower type was sold by this seller

                            std::cout << "  Buyer " << currentOrder.buyerId << " bought " << buying << " "
                                      << FlowerNames[flower] << "(s) from " << seller.name
                                      << " for $" << std::fixed << std::setprecision(2) << cost
                                      << " (Budget left: $" << result.remaining_budget << ")\n";
                        }
                    }
                }

                // Print buyer's result for the current round
                std::cout << "  🛒 Buyer " << currentOrder.buyerId << " - ROUND " << round << " Summary:\n";
                for (int i = 0; i < 3; ++i)
                    std::cout << "    " << result.fulfilled[i] << " " << FlowerNames[i] << "(s) bought\n";
                std::cout << "    Budget left: $" << std::fixed << std::setprecision(2) << result.remaining_budget << "\n";
            }

            // 🔽 Price Drop Logic: If a flower was not sold by a seller, reduce its price by 20%
            std::cout << "\n--- Price Adjustments for Round " << round << " ---\n";
            for (int s = 0; s < sellers.size(); ++s)
            {
                Seller &seller = sellers[s];
                for (int f = 0; f < 3; ++f)
                {
                    // Check if the flower type was NOT sold by this seller AND they still have stock
                    if (!flowerSold[s][f] && seller.quantity[f] > 0)
                    {
                        double old_price = seller.price[f];
                        seller.price[f] *= 0.8; // Reduce price by 20%
                        std::cout << "  ⚠️ Price Drop: " << seller.name << "'s " << FlowerNames[f]
                                  << " price dropped from $" << std::fixed << std::setprecision(2) << old_price
                                  << " to $" << std::fixed << std::setprecision(2) << seller.price[f] << "\n";
                    }
                }
            }
        }

        // Print final seller stock after all rounds
        std::cout << "\n========================================\n";
        std::cout << "📦 Final Seller Stock After " << NUM_ROUNDS << " Rounds:\n";
        std::cout << "========================================\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int i = 0; i < 3; ++i)
                std::cout << s.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
        }

        // Record the end time and print total simulation duration
        double end_time = MPI_Wtime();
        std::cout << "\n========================================\n";
        std::cout << "🕒 Total Simulation Time: " << std::fixed << std::setprecision(4) << (end_time - start_time) << " seconds\n";
        std::cout << "========================================\n";
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}