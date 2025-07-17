#include <mpi.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unistd.h> // For usleep function

enum FlowerType
{
    ROSE = 0,
    SUNFLOWER = 1,
    TULIP = 2
};
const char *FlowerNames[3] = {"Rose", "Sunflower", "Tulip"};

const int NUM_ROUNDS = 3; // Number of trading rounds

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
    double maxPrice[3];
};

struct Order
{
    int buyerRank;
    char buyerName[20];
    int demand[3];
    double budget;
    double maxPrice[3];
};

struct TradeResult
{
    int fulfilled[3];
    double remaining_budget;
};

void printCurrentStatus(const std::vector<Seller> &sellers, int round)
{
    std::cout << "\n📊 CURRENT STATUS AFTER ROUND " << round << " 📊\n";
    std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                     SELLER INVENTORY                        │\n";
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";

    for (const auto &seller : sellers)
    {
        std::cout << "│ " << seller.name << ":\n";
        std::cout << "│   Stock: ";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << seller.quantity[i] << " " << FlowerNames[i];
            if (i < 2)
                std::cout << ", ";
        }
        std::cout << "\n│   Prices: ";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "$" << seller.price[i] << " " << FlowerNames[i];
            if (i < 2)
                std::cout << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "└─────────────────────────────────────────────────────────────┘\n";
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        if (rank == 0)
            std::cout << "Run with at least 2 processes (1 master + 1 buyer)\n";
        MPI_Finalize();
        return 0;
    }

    // Predefined buyer list
    std::vector<Buyer> allBuyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}},
        {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}},
        {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}},
        {"Ben", {10, 0, 5}, 350, {4.5, 0.0, 5.0}},
        {"Lia", {2, 2, 2}, 100, {4.0, 4.0, 4.0}},
        {"Joe", {5, 10, 5}, 400, {5.0, 5.0, 5.0}},
        {"Sue", {5, 5, 5}, 200, {4.5, 4.5, 4.5}},
        {"Amy", {1, 1, 1}, 50, {3.0, 3.0, 3.0}},
        {"Tim", {4, 6, 3}, 250, {4.5, 4.5, 5.0}},
        {"Sam", {7, 8, 4}, 600, {5.0, 5.0, 5.0}},
        {"Jill", {3, 4, 5}, 200, {4.0, 4.5, 5.0}},
        {"Zoe", {6, 3, 7}, 300, {4.0, 5.0, 5.5}},
        {"Max", {5, 5, 5}, 250, {4.5, 4.5, 4.5}},
        {"Ivy", {8, 6, 4}, 550, {5.0, 5.0, 5.0}},
        {"Leo", {9, 0, 2}, 350, {4.2, 0.0, 5.0}},
        {"Kim", {3, 3, 3}, 180, {4.0, 4.0, 4.0}},
        {"Tom", {6, 5, 3}, 400, {4.8, 4.8, 5.0}},
        {"Nina", {4, 2, 6}, 280, {4.0, 4.0, 5.0}},
        {"Ray", {3, 5, 4}, 300, {4.5, 4.5, 4.5}},
        {"Liv", {5, 3, 2}, 250, {4.0, 4.0, 4.5}},
        {"Oli", {6, 6, 6}, 450, {5.0, 5.0, 5.0}},
        {"Ken", {2, 2, 2}, 100, {3.5, 3.5, 3.5}},
        {"Ana", {7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    if (rank == 0)
    {
        // ---------------- Master Process ----------------
        std::vector<Seller> sellers = {
            {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
            {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
            {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

        double start_time = MPI_Wtime();

        // Print initial status
        std::cout << "\n🌸 FLOWER TRADING SIMULATION STARTING 🌸\n";
        std::cout << "═══════════════════════════════════════════\n";
        printCurrentStatus(sellers, 0);

        for (int round = 1; round <= NUM_ROUNDS; ++round)
        {
            std::cout << "\n🔁 ROUND " << round << " STARTS 🔁\n";
            std::cout << "═══════════════════════════════════════\n";

            // Track if each seller sold a particular flower type
            std::vector<std::vector<bool>> flowerSold(sellers.size(), std::vector<bool>(3, false));

            for (int buyerRank = 1; buyerRank < size; ++buyerRank)
            {
                Order order;
                MPI_Recv(&order, sizeof(Order), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                TradeResult result = {{0, 0, 0}, order.budget};

                std::cout << "\n💰 Processing order from " << order.buyerName << " (Rank " << buyerRank << "):\n";
                std::cout << "   Demand: ";
                for (int i = 0; i < 3; ++i)
                {
                    std::cout << order.demand[i] << " " << FlowerNames[i];
                    if (i < 2)
                        std::cout << ", ";
                }
                std::cout << " | Budget: $" << order.budget << "\n";

                for (int flower = 0; flower < 3; ++flower)
                {
                    int needed = order.demand[flower];
                    if (needed <= 0)
                        continue;

                    std::cout << "\n   🌺 Looking for " << needed << " " << FlowerNames[flower] << "(s):\n";

                    for (int s = 0; s < sellers.size(); ++s)
                    {
                        Seller &seller = sellers[s];
                        if (needed <= 0)
                            break;

                        // Check if buyer is willing to pay seller's price
                        if (seller.price[flower] > order.maxPrice[flower])
                        {
                            std::cout << "     ❌ " << seller.name << ": Price $" << seller.price[flower]
                                      << " > Max price $" << order.maxPrice[flower] << "\n";
                            continue;
                        }

                        int affordable = static_cast<int>(result.remaining_budget / seller.price[flower]);
                        int available = seller.quantity[flower];
                        int buying = std::min({needed, available, affordable});

                        if (buying > 0)
                        {
                            double cost = buying * seller.price[flower];
                            result.fulfilled[flower] += buying;
                            result.remaining_budget -= cost;
                            seller.quantity[flower] -= buying;
                            needed -= buying;

                            flowerSold[s][flower] = true;

                            std::cout << "     ✅ " << seller.name << ": Bought " << buying
                                      << " for $" << cost << " (Stock now: " << seller.quantity[flower] << ")\n";
                        }
                        else if (available == 0)
                        {
                            std::cout << "     ❌ " << seller.name << ": Out of stock\n";
                        }
                        else if (affordable == 0)
                        {
                            std::cout << "     ❌ " << seller.name << ": Insufficient budget\n";
                        }
                    }
                }

                MPI_Send(&result, sizeof(TradeResult), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD);
            }

            // 🔽 Price Drop: If flower was not sold, reduce price by 10%
            std::cout << "\n📉 PRICE ADJUSTMENTS:\n";
            bool anyPriceDropped = false;
            for (int s = 0; s < sellers.size(); ++s)
            {
                Seller &seller = sellers[s];
                for (int f = 0; f < 3; ++f)
                {
                    if (!flowerSold[s][f] && seller.quantity[f] > 0)
                    {
                        double old_price = seller.price[f];
                        seller.price[f] *= 0.9; // drop 10%
                        std::cout << "⚠️ " << seller.name << "'s " << FlowerNames[f]
                                  << " price: $" << old_price
                                  << " → $" << seller.price[f] << " (-10%)\n";
                        anyPriceDropped = true;
                    }
                }
            }
            if (!anyPriceDropped)
            {
                std::cout << "✅ No price drops needed - all flower types were sold!\n";
            }

            // Print current status after each round
            printCurrentStatus(sellers, round);

            // Add 100ms pause between rounds (except after the last round)
            if (round < NUM_ROUNDS)
            {
                std::cout << "\n⏸️ Pausing 100ms before next round...\n";
                usleep(100000); // 100ms = 100,000 microseconds
            }
        }

        std::cout << "\n🎉 SIMULATION COMPLETE! 🎉\n";
        std::cout << "═══════════════════════════════════════\n";
        std::cout << "📦 Final Seller Stock:\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int i = 0; i < 3; ++i)
                std::cout << s.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
        }

        double end_time = MPI_Wtime();
        std::cout << "\n🕒 Total Simulation Time: " << (end_time - start_time) << " seconds\n";
    }
    else
    {
        // ---------------- Buyer Processes ----------------
        // Each process gets a buyer from the list (cycling if needed)
        int buyerIndex = (rank - 1) % allBuyers.size();
        Buyer myBuyer = allBuyers[buyerIndex];

        for (int round = 1; round <= NUM_ROUNDS; ++round)
        {
            Order myOrder;
            myOrder.buyerRank = rank;
            strcpy(myOrder.buyerName, myBuyer.name);

            // Copy demand, budget, and max prices from the buyer profile
            for (int i = 0; i < 3; ++i)
            {
                myOrder.demand[i] = myBuyer.demand[i];
                myOrder.maxPrice[i] = myBuyer.maxPrice[i];
            }
            myOrder.budget = myBuyer.budget;

            MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

            TradeResult result;
            MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            std::cout << "\n🛒 Buyer " << myBuyer.name << " (Rank " << rank << ") - ROUND " << round << " Result:\n";
            std::cout << "   Received: ";
            for (int i = 0; i < 3; ++i)
            {
                std::cout << result.fulfilled[i] << " " << FlowerNames[i];
                if (i < 2)
                    std::cout << ", ";
            }
            std::cout << "\n   Budget remaining: $" << result.remaining_budget << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}