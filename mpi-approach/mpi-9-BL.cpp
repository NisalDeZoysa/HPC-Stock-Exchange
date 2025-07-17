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

bool allSellersOut(const std::vector<Seller> &sellers)
{
    for (const auto &s : sellers)
        for (int i = 0; i < 3; ++i)
            if (s.quantity[i] > 0)
                return false;
    return true;
}

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
            std::cerr << "At least 2 processes are needed.\n";
        MPI_Finalize();
        return 0;
    }

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

    if (rank == 0)
    {
        start_time = MPI_Wtime();

        std::vector<Seller> sellers = {
            {"Alice", {100, 100, 100}, {6.0, 5.5, 7.0}},
            {"Bob", {100, 100, 100}, {5.5, 5.2, 6.5}},
            {"Charlie", {100, 100, 100}, {6.8, 5.0, 7.5}}};

        int round = 0;
        bool marketOpen = true;

        std::cout << "🌼 MPI Trading Market Simulation Started (" << numBuyers << " buyers)\n";

        while (marketOpen)
        {
            round++;
            std::cout << "\n--- Round " << round << " ---\n";
            bool any_trade = false;

            // Receive orders from buyers
            std::vector<Order> currentOrders(numBuyers);
            for (int b = 1; b <= numBuyers; ++b)
                MPI_Recv(&currentOrders[b - 1], sizeof(Order), MPI_BYTE, b, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            std::vector<TradeResult> results(numBuyers);

            // Process each buyer's order
            for (int b = 0; b < numBuyers; ++b)
            {
                TradeResult &result = results[b];
                Order &order = currentOrders[b];

                for (int f = 0; f < 3; ++f)
                {
                    int bestSeller = -1;
                    double bestPrice = 1e9;

                    // Find the best seller for the flower
                    for (int s = 0; s < (int)sellers.size(); ++s)
                    {
                        if (sellers[s].quantity[f] > 0 &&
                            sellers[s].price[f] <= order.buy_price[f] &&
                            sellers[s].price[f] < bestPrice)
                        {
                            bestPrice = sellers[s].price[f];
                            bestSeller = s;
                        }
                    }

                    int bought = 0;
                    if (bestSeller != -1 && order.demand[f] > 0)
                    {
                        Seller &seller = sellers[bestSeller];
                        int maxAffordable = (int)(order.budget / seller.price[f]);
                        bought = std::min({maxAffordable, order.demand[f], seller.quantity[f]});

                        if (bought > 0)
                        {
                            double cost = bought * seller.price[f];
                            order.budget -= cost;
                            seller.quantity[f] -= bought;
                            order.demand[f] -= bought;
                            any_trade = true;

                            std::cout << buyerNames[b] << " bought " << bought << " " << FlowerNames[f]
                                      << " from " << seller.name << " at $" << seller.price[f] << "\n";
                        }
                    }

                    result.fulfilled[f] = bought;
                }

                result.remaining_budget = order.budget;
                buyerStates[b] = order;
            }

            // Send results to buyers
            for (int b = 1; b <= numBuyers; ++b)
                MPI_Send(&results[b - 1], sizeof(TradeResult), MPI_BYTE, b, 1, MPI_COMM_WORLD);

            // If no trades occurred, reduce prices but not below 0.2
            if (!any_trade)
            {
                for (auto &s : sellers)
                    for (int f = 0; f < 3; ++f)
                        if (s.price[f] > 0.2)
                            s.price[f] -= 0.2;

                std::cout << "⚠️ No trades occurred. Prices dropped.\n";
            }

            printStatus(sellers, buyerStates, buyerNames);

            // Check if all buyers have fulfilled demands or sellers out of stock
            bool allBuyersDone = true;
            for (const auto &b : buyerStates)
                for (int f = 0; f < 3; ++f)
                    if (b.demand[f] > 0)
                        allBuyersDone = false;

            marketOpen = !(allBuyersDone || allSellersOut(sellers));

            // Inform buyers if market is still open
            for (int b = 1; b <= numBuyers; ++b)
                MPI_Send(&marketOpen, 1, MPI_CXX_BOOL, b, 2, MPI_COMM_WORLD);

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100 ms delay between rounds
        }

        end_time = MPI_Wtime();
        std::cout << "\nTotal simulation time: " << (end_time - start_time) << " seconds.\n";
        std::cout << "\n✅ Market closed after " << round << " rounds.\n";
    }
    else
    {
        Order myOrder = buyerStates[rank - 1];
        bool marketOpen = true;

        while (marketOpen)
        {
            MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

            TradeResult result;
            MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int f = 0; f < 3; ++f)
                myOrder.demand[f] -= result.fulfilled[f];
            myOrder.budget = result.remaining_budget;

            MPI_Recv(&marketOpen, 1, MPI_CXX_BOOL, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    MPI_Finalize();
    return 0;
}
