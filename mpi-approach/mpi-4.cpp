#include <mpi.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

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

struct Order
{
    int buyerRank;
    int demand[3];
    double budget;
};

struct TradeResult
{
    int fulfilled[3];
    double remaining_budget;
};

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

    if (rank == 0)
    {
        // ---------------- Master Process ----------------
        std::vector<Seller> sellers = {
            {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
            {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
            {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

        double start_time = MPI_Wtime();

        for (int round = 1; round <= NUM_ROUNDS; ++round)
        {
            std::cout << "\n🔁 ROUND " << round << " STARTS 🔁\n";

            // Track if each seller sold a particular flower type
            std::vector<std::vector<bool>> flowerSold(sellers.size(), std::vector<bool>(3, false));

            for (int buyerRank = 1; buyerRank < size; ++buyerRank)
            {
                Order order;
                MPI_Recv(&order, sizeof(Order), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                TradeResult result = {{0, 0, 0}, order.budget};

                for (int flower = 0; flower < 3; ++flower)
                {
                    int needed = order.demand[flower];
                    for (int s = 0; s < sellers.size(); ++s)
                    {
                        Seller &seller = sellers[s];
                        if (needed <= 0)
                            break;

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

                            std::cout << "Buyer " << buyerRank << " bought " << buying << " "
                                      << FlowerNames[flower] << "(s) from " << seller.name
                                      << " for $" << cost << "\n";
                        }
                    }
                }

                MPI_Send(&result, sizeof(TradeResult), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD);
            }

            // 🔽 Price Drop: If flower was not sold, reduce price by 10%
            for (int s = 0; s < sellers.size(); ++s)
            {
                Seller &seller = sellers[s];
                for (int f = 0; f < 3; ++f)
                {
                    if (!flowerSold[s][f] && seller.quantity[f] > 0)
                    {
                        double old_price = seller.price[f];
                        seller.price[f] *= 0.9; // drop 10%
                        std::cout << "⚠️ Price Drop: " << seller.name << "'s " << FlowerNames[f]
                                  << " price dropped from $" << old_price
                                  << " to $" << seller.price[f] << "\n";
                    }
                }
            }
        }

        std::cout << "\n📦 Final Seller Stock:\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int i = 0; i < 3; ++i)
                std::cout << s.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
        }

        double end_time = MPI_Wtime(); // <<<<<< End timer
        std::cout << "\n🕒 Total Simulation Time: " << (end_time - start_time) << " seconds\n";
    }
    else
    {
        // ---------------- Buyer Processes ----------------
        for (int round = 1; round <= NUM_ROUNDS; ++round)
        {
            Order myOrder;
            myOrder.buyerRank = rank;
            myOrder.demand[ROSE] = 5 + rank;
            myOrder.demand[SUNFLOWER] = 3 + (rank % 2);
            myOrder.demand[TULIP] = 2;
            myOrder.budget = 50 + 10 * rank;

            MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

            TradeResult result;
            MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            std::cout << "\n🛒 Buyer " << rank << " - ROUND " << round << " Result:\n";
            for (int i = 0; i < 3; ++i)
                std::cout << "  " << result.fulfilled[i] << " " << FlowerNames[i] << "(s)\n";
            std::cout << "  Budget left: $" << result.remaining_budget << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}
