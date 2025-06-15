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

struct Seller
{
    char name[20];
    int quantity[3]; // Quantities: [ROSE, SUNFLOWER, TULIP]
    double price[3]; // Prices
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

        for (int buyerRank = 1; buyerRank < size; ++buyerRank)
        {
            Order order;
            MPI_Recv(&order, sizeof(Order), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            TradeResult result = {{0, 0, 0}, order.budget};

            for (int flower = 0; flower < 3; ++flower)
            {
                int needed = order.demand[flower];
                for (auto &seller : sellers)
                {
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

                        std::cout << "Buyer " << buyerRank << " bought " << buying << " "
                                  << FlowerNames[flower] << "(s) from " << seller.name
                                  << " for $" << cost << "\n";
                    }
                }
            }

            MPI_Send(&result, sizeof(TradeResult), MPI_BYTE, buyerRank, 0, MPI_COMM_WORLD);
        }

        std::cout << "\nFinal Seller Stock:\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int i = 0; i < 3; ++i)
            {
                std::cout << s.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
            }
        }
    }
    else
    {
        // ---------------- Buyer Processes ----------------
        Order myOrder;
        myOrder.buyerRank = rank;

        // For testing: Each buyer has different demands/budgets
        myOrder.demand[ROSE] = 5 + rank;
        myOrder.demand[SUNFLOWER] = 3 + (rank % 2);
        myOrder.demand[TULIP] = 2;
        myOrder.budget = 50 + 10 * rank;

        MPI_Send(&myOrder, sizeof(Order), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

        TradeResult result;
        MPI_Recv(&result, sizeof(TradeResult), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Buyer " << rank << " received:\n";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "  " << result.fulfilled[i] << " " << FlowerNames[i] << "(s)\n";
        }
        std::cout << "  Budget left: $" << result.remaining_budget << "\n";
    }

    MPI_Finalize();
    return 0;
}
