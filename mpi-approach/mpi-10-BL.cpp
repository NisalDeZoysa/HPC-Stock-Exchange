#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>

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

struct Order
{
    int demand[3];
    double budget;
    double buy_price[3];
};

struct TradeResult
{
    int fulfilled[3];
    double remaining_budget;
};

// Function to check if all sellers are out of stock
bool allSellersOut(const std::vector<Seller> &sellers)
{
    for (const auto &s : sellers)
        for (int i = 0; i < 3; ++i)
            if (s.quantity[i] > 0)
                return false;
    return true;
}

// Function to print the current status of sellers and buyers
void printStatus(const std::vector<Seller> &sellers, const std::vector<Order> &buyerStates, const std::vector<std::string> &buyerNames)
{
    std::cout << "\n📦 Seller Inventory:\n";
    for (const auto &s : sellers)
    {
        std::cout << s.name << ": ";
        for (int i = 0; i < 3; ++i)
            std::cout << FlowerNames[i] << "=" << s.quantity[i] << " ($" << s.price[i] << ") ";
        std::cout << "\n";
    }

    std::cout << "\n🧍 Buyer Demands:\n";
    for (size_t i = 0; i < buyerStates.size(); ++i)
    {
        std::cout << buyerNames[i] << ": ";
        for (int f = 0; f < 3; ++f)
            std::cout << FlowerNames[f] << "=" << buyerStates[i].demand[f] << " ";
        std::cout << " | Budget: $" << buyerStates[i].budget << "\n";
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int numBuyers = size - 1;

    if (size < 2)
    {
        if (rank == 0)
            std::cerr << "At least 2 processes are needed (1 master + 1 buyer).\n";
        MPI_Finalize();
        return 0;
    }

    // Initial buyer demands and budgets (unchanged)
    std::vector<Order> buyerStates = {
        {{10, 5, 2}, 500, {4.0, 4.0, 5.0}},
        {{5, 5, 0}, 300, {3.5, 3.5, 0.0}},
        {{15, 10, 5}, 1000, {5.0, 4.5, 5.5}},
        {{10, 0, 5}, 350, {4.5, 0.0, 5.0}},
        {{2, 2, 2}, 100, {4.0, 4.0, 4.0}},
        {{5, 10, 5}, 400, {5.0, 5.0, 5.0}},
        {{5, 5, 5}, 200, {4.5, 4.5, 4.5}},
        {{1, 1, 1}, 50, {3.0, 3.0, 3.0}},
        {{4, 6, 3}, 250, {4.5, 4.5, 5.0}},
        {{7, 8, 4}, 600, {5.0, 5.0, 5.0}},
        {{3, 4, 5}, 200, {4.0, 4.5, 5.0}},
        {{6, 3, 7}, 300, {4.0, 5.0, 5.5}},
        {{5, 5, 5}, 250, {4.5, 4.5, 4.5}},
        {{8, 6, 4}, 550, {5.0, 5.0, 5.0}},
        {{9, 0, 2}, 350, {4.2, 0.0, 5.0}},
        {{3, 3, 3}, 180, {4.0, 4.0, 4.0}},
        {{6, 5, 3}, 400, {4.8, 4.8, 5.0}},
        {{4, 2, 6}, 280, {4.0, 4.0, 5.0}},
        {{3, 5, 4}, 300, {4.5, 4.5, 4.5}},
        {{5, 3, 2}, 250, {4.0, 4.0, 4.5}},
        {{6, 6, 6}, 450, {5.0, 5.0, 5.0}},
        {{2, 2, 2}, 100, {3.5, 3.5, 3.5}},
        {{7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    std::vector<std::string> buyerNames = {
        "Dan", "Eve", "Fay", "Ben", "Lia", "Joe", "Sue", "Amy", "Tim", "Sam",
        "Jill", "Zoe", "Max", "Ivy", "Leo", "Kim", "Tom", "Nina", "Ray", "Liv", "Oli", "Ken", "Ana"};

    double start_time = 0.0, end_time = 0.0;

    if (rank == 0) // Master Process
    {
        start_time = MPI_Wtime();

        // Adjusted initial seller prices to be more aligned with buyer buy_prices
        std::vector<Seller> sellers = {
            {"Alice", {100, 100, 100}, {4.5, 4.0, 5.0}},    // Reduced initial prices
            {"Bob", {100, 100, 100}, {4.0, 3.8, 4.8}},      // Reduced initial prices
            {"Charlie", {100, 100, 100}, {5.0, 3.5, 5.2}}}; // Reduced initial prices

        int round = 0;
        bool marketOpen = true;

        std::cout << "🌼 MPI Trading Market Simulation Started (" << numBuyers << " buyers)\n";

        while (marketOpen)
        {
            round++;
            std::cout << "\n--- Round " << round << " ---\n";
            bool any_trade_in_round = false; // Flag to track if any trade occurred in the current round

            // Receive orders from buyers
            std::vector<Order> currentOrders(numBuyers);
            for (int b = 1; b <= numBuyers; ++b)
            {
                // Ensure buyerStates is correctly indexed for MPI_Recv to match the buyer's rank
                // Buyer ranks are 1 to numBuyers, so buyerStates[b-1] corresponds to buyer rank 'b'
                MPI_Recv(&currentOrders[b - 1], sizeof(Order), MPI_BYTE, b, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            std::vector<TradeResult> results(numBuyers);

            // Process each buyer's order
            for (int b_idx = 0; b_idx < numBuyers; ++b_idx) // Use b_idx for vector index
            {
                TradeResult &result = results[b_idx];
                Order &order = currentOrders[b_idx]; // This 'order' is the buyer's current demand/budget

                // Initialize fulfilled quantities for the current buyer for this round
                for (int f = 0; f < 3; ++f)
                    result.fulfilled[f] = 0;

                // Try to fulfill demand for each flower type
                for (int f = 0; f < 3; ++f)
                {
                    if (order.demand[f] <= 0)
                        continue; // No demand for this flower

                    int bestSellerIdx = -1;
                    double bestPrice = 1e9; // Initialize with a very high price

                    // Find the best seller for the current flower type
                    for (int s_idx = 0; s_idx < (int)sellers.size(); ++s_idx)
                    {
                        if (sellers[s_idx].quantity[f] > 0 &&
                            sellers[s_idx].price[f] <= order.buy_price[f] && // Seller's price within buyer's budget limit for this flower
                            sellers[s_idx].price[f] < bestPrice)             // Found a better price
                        {
                            bestPrice = sellers[s_idx].price[f];
                            bestSellerIdx = s_idx;
                        }
                    }

                    if (bestSellerIdx != -1) // A suitable seller was found
                    {
                        Seller &seller = sellers[bestSellerIdx]; // Get a reference to the best seller

                        // Calculate how many the buyer can afford based on their budget
                        int maxAffordableByBudget = (seller.price[f] > 0) ? (int)(order.budget / seller.price[f]) : order.demand[f]; // Avoid division by zero

                        // Determine the actual quantity to buy
                        // It's the minimum of: what the buyer needs, what the seller has, what the buyer can afford
                        int bought = std::min({order.demand[f], seller.quantity[f], maxAffordableByBudget});

                        if (bought > 0)
                        {
                            double cost = bought * seller.price[f];
                            order.budget -= cost;         // Deduct cost from buyer's budget
                            seller.quantity[f] -= bought; // Decrease seller's quantity
                            order.demand[f] -= bought;    // Decrease buyer's demand
                            result.fulfilled[f] = bought; // Record fulfilled quantity
                            any_trade_in_round = true;    // Mark that a trade occurred

                            std::cout << buyerNames[b_idx] << " bought " << bought << " " << FlowerNames[f]
                                      << " from " << seller.name << " at $" << seller.price[f] << "\n";
                        }
                    }
                }

                result.remaining_budget = order.budget; // Update remaining budget for the buyer
                buyerStates[b_idx] = order;             // Update the master's copy of buyer's state
            }

            // Send results back to buyers
            for (int b = 1; b <= numBuyers; ++b)
                MPI_Send(&results[b - 1], sizeof(TradeResult), MPI_BYTE, b, 1, MPI_COMM_WORLD);

            // If no trades occurred in this round, reduce seller prices (but not below 0.2)
            if (!any_trade_in_round)
            {
                for (auto &s : sellers)
                {
                    for (int f = 0; f < 3; ++f)
                    {
                        if (s.price[f] > 0.2) // Ensure price doesn't go below 0.2
                        {
                            s.price[f] -= 0.2; // Fixed drop
                            // Alternative: s.price[f] *= 0.95; // Percentage drop
                        }
                    }
                }
                std::cout << "⚠️ No trades occurred in this round. Seller prices dropped.\n";
            }

            // Print the status of sellers and buyers
            printStatus(sellers, buyerStates, buyerNames);

            // Check market closure conditions
            bool allBuyersDone = true;
            for (const auto &b : buyerStates)
                for (int f = 0; f < 3; ++f)
                    if (b.demand[f] > 0)
                        allBuyersDone = false;

            marketOpen = !(allBuyersDone || allSellersOut(sellers));

            // Inform buyers whether the market is still open for the next round
            for (int b = 1; b <= numBuyers; ++b)
                MPI_Send(&marketOpen, 1, MPI_CXX_BOOL, b, 2, MPI_COMM_WORLD);

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small delay for readability
        }

        end_time = MPI_Wtime();
        std::cout << "\nTotal simulation time: " << (end_time - start_time) << " seconds.\n";
        std::cout << "\n✅ Market closed after " << round << " rounds.\n";
    }
    else // Buyer Processes (rank > 0)
    {
        // Each buyer manages their own order state
        Order myOrder = buyerStates[rank - 1]; // rank-1 because buyerNames and buyerStates are 0-indexed vectors

        bool marketOpen = true;

        while (marketOpen)
        {
            // Send current demand and budget to the master
            MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

            // Receive trade results from the master
            TradeResult result;
            MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Update local buyer state based on trade results
            for (int f = 0; f < 3; ++f)
                myOrder.demand[f] -= result.fulfilled[f];
            myOrder.budget = result.remaining_budget;

            // Receive market status from the master
            MPI_Recv(&marketOpen, 1, MPI_CXX_BOOL, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    MPI_Finalize();
    return 0;
}