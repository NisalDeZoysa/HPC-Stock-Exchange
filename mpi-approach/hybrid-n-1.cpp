#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>

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
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Shared seller list (everyone starts with same)
    std::vector<Seller> sellers = {
        {"Alice", {30, 10, 20}, {6.0, 5.5, 7.0}},
        {"Bob", {20, 20, 10}, {5.5, 5.2, 6.5}},
        {"Charlie", {10, 5, 10}, {6.8, 5.0, 7.5}}};

    // Each process gets its own buyer
    Buyer myBuyer;
    if (rank == 1)
    {
        myBuyer = {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}};
    }
    else if (rank == 2)
    {
        myBuyer = {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}};
    }
    else if (rank == 3)
    {
        myBuyer = {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}};
    }
    else
    {
        myBuyer = {"Idle", {0, 0, 0}, 0, {0, 0, 0}};
    }

    int round = 0;
    bool done = false;

    while (!done)
    {
        round++;
        bool traded = false;

        // Parallel matching with OpenMP
#pragma omp parallel for collapse(2) shared(sellers, myBuyer)
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
#pragma omp critical
                {
                    Seller &seller = sellers[i];

                    if (myBuyer.demand[flower] > 0 &&
                        seller.quantity[flower] > 0 &&
                        seller.price[flower] <= myBuyer.buy_price[flower] &&
                        myBuyer.budget >= seller.price[flower])
                    {
                        int max_affordable = myBuyer.budget / seller.price[flower];
                        int buy_qty = std::min({seller.quantity[flower], myBuyer.demand[flower], max_affordable});
                        double cost = buy_qty * seller.price[flower];

                        if (buy_qty > 0)
                        {
                            seller.quantity[flower] -= buy_qty;
                            myBuyer.demand[flower] -= buy_qty;
                            myBuyer.budget -= cost;
                            traded = true;

                            std::cout << "[Rank " << rank << "] " << myBuyer.name << " bought "
                                      << buy_qty << " " << FlowerNames[flower]
                                      << "(s) from " << seller.name << " for $" << cost << "\n";
                        }
                    }
                }
            }
        }

        // Drop price if no trade happened
#pragma omp parallel for
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int f = 0; f < 3; ++f)
            {
#pragma omp critical
                {
                    if (sellers[i].price[f] > 0.2)
                        sellers[i].price[f] -= 0.2;
                }
            }
        }

        // Check if done
        done = !demandsLeft(myBuyer);

        // Share done status across processes
        int global_done;
        int local_done = done ? 1 : 0;
        MPI_Allreduce(&local_done, &global_done, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        done = global_done;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[Rank " << rank << "] ✅ Finished all buying in " << round << " rounds. Budget left: $" << myBuyer.budget << "\n";

    MPI_Finalize();
    return 0;
}
