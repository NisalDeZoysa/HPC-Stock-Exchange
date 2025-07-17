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

    std::vector<Seller> sellers;

    // Master list of 23 buyers
    std::vector<Buyer> allBuyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}}, {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}}, {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}}, {"Ben", {10, 0, 5}, 350, {4.5, 0.0, 5.0}}, {"Lia", {2, 2, 2}, 100, {4.0, 4.0, 4.0}}, {"Joe", {5, 10, 5}, 400, {5.0, 5.0, 5.0}}, {"Sue", {5, 5, 5}, 200, {4.5, 4.5, 4.5}}, {"Amy", {1, 1, 1}, 50, {3.0, 3.0, 3.0}}, {"Tim", {4, 6, 3}, 250, {4.5, 4.5, 5.0}}, {"Sam", {7, 8, 4}, 600, {5.0, 5.0, 5.0}}, {"Jill", {3, 4, 5}, 200, {4.0, 4.5, 5.0}}, {"Zoe", {6, 3, 7}, 300, {4.0, 5.0, 5.5}}, {"Max", {5, 5, 5}, 250, {4.5, 4.5, 4.5}}, {"Ivy", {8, 6, 4}, 550, {5.0, 5.0, 5.0}}, {"Leo", {9, 0, 2}, 350, {4.2, 0.0, 5.0}}, {"Kim", {3, 3, 3}, 180, {4.0, 4.0, 4.0}}, {"Tom", {6, 5, 3}, 400, {4.8, 4.8, 5.0}}, {"Nina", {4, 2, 6}, 280, {4.0, 4.0, 5.0}}, {"Ray", {3, 5, 4}, 300, {4.5, 4.5, 4.5}}, {"Liv", {5, 3, 2}, 250, {4.0, 4.0, 4.5}}, {"Oli", {6, 6, 6}, 450, {5.0, 5.0, 5.0}}, {"Ken", {2, 2, 2}, 100, {3.5, 3.5, 3.5}}, {"Ana", {7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    // Manager sets sellers
    if (rank == 0)
    {
        sellers = {
            {"Alice", {100, 100, 100}, {6.0, 5.5, 7.0}},
            {"Bob", {100, 100, 100}, {5.5, 5.2, 6.5}},
            {"Charlie", {100, 100, 100}, {6.8, 5.0, 7.5}}};
    }

    // Distribute buyers across processes
    std::vector<Buyer> myBuyers;
    if (rank != 0)
    {
        for (size_t i = rank - 1; i < allBuyers.size(); i += (size - 1))
        {
            myBuyers.push_back(allBuyers[i]);
        }
    }

    bool global_done = false;
    int round = 0;

    while (!global_done)
    {
        round++;

        // Step 1: Broadcast sellers
        if (rank == 0)
        {
            for (int i = 1; i < size; ++i)
                MPI_Send(sellers.data(), sellers.size() * sizeof(Seller), MPI_BYTE, i, 0, MPI_COMM_WORLD);
        }
        else
        {
            sellers.resize(3);
            MPI_Recv(sellers.data(), sellers.size() * sizeof(Seller), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Step 2: Buyers in each process trade
        std::vector<std::vector<int>> tradeQty(myBuyers.size(), std::vector<int>(3, 0));
        std::vector<std::vector<int>> sellerIdx(myBuyers.size(), std::vector<int>(3, -1));

#pragma omp parallel for collapse(2)
        for (int b = 0; b < myBuyers.size(); ++b)
        {
            for (int f = 0; f < 3; ++f)
            {
                for (int i = 0; i < sellers.size(); ++i)
                {
#pragma omp critical
                    {
                        Buyer &buyer = myBuyers[b];
                        Seller &seller = sellers[i];
                        if (buyer.demand[f] > 0 &&
                            seller.quantity[f] > 0 &&
                            seller.price[f] <= buyer.buy_price[f] &&
                            buyer.budget >= seller.price[f])
                        {
                            int max_affordable = buyer.budget / seller.price[f];
                            int qty = std::min({seller.quantity[f], buyer.demand[f], max_affordable});
                            if (qty > 0)
                            {
                                double cost = qty * seller.price[f];
                                buyer.demand[f] -= qty;
                                buyer.budget -= cost;
                                tradeQty[b][f] = qty;
                                sellerIdx[b][f] = i;
                                std::cout << "[Rank " << rank << "] " << buyer.name
                                          << " wants " << qty << " " << FlowerNames[f]
                                          << "(s) from " << seller.name << " for $" << cost << "\n";
                            }
                        }
                    }
                }
            }
        }

        // Step 3: Send trade decisions to manager
        if (rank != 0)
        {
            for (int b = 0; b < myBuyers.size(); ++b)
            {
                MPI_Send(tradeQty[b].data(), 3, MPI_INT, 0, 1, MPI_COMM_WORLD);
                MPI_Send(sellerIdx[b].data(), 3, MPI_INT, 0, 2, MPI_COMM_WORLD);
            }
        }

        // Step 4: Manager updates seller inventory
        if (rank == 0)
        {
            for (int r = 1; r < size; ++r)
            {
                int buyersPerProc = (allBuyers.size() + size - 2) / (size - 1);
                for (int b = 0; b < buyersPerProc; ++b)
                {
                    std::vector<int> q(3), s(3);
                    MPI_Recv(q.data(), 3, MPI_INT, r, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(s.data(), 3, MPI_INT, r, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int f = 0; f < 3; ++f)
                    {
                        if (s[f] >= 0)
                            sellers[s[f]].quantity[f] -= q[f];
                    }
                }
            }
            // Decrease prices
            for (auto &seller : sellers)
                for (int f = 0; f < 3; ++f)
                    if (seller.price[f] > 0.2)
                        seller.price[f] -= 0.2;
        }

        // Step 5: Check for global completion
        int local_done = 1;
        for (auto &b : myBuyers)
            if (demandsLeft(b))
                local_done = 0;

        MPI_Allreduce(&local_done, &global_done, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
    }

    double end_time = MPI_Wtime();
    if (rank != 0)
    {
        for (auto &b : myBuyers)
        {
            std::cout << "[Rank " << rank << "] ✅ " << b.name << " finished with $" << b.budget << "\n";
        }
    }

    if (rank == 0)
    {
        std::cout << "\n📊 Final Seller Stocks:\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int f = 0; f < 3; ++f)
                std::cout << FlowerNames[f] << "=" << s.quantity[f] << " ";
            std::cout << "\n";
        }
        std::cout << "\n⏱️ Total Time: " << end_time - start_time << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}
