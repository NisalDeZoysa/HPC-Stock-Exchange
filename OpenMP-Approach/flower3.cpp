#include <iostream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib> // for rand

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
    int quantity[3]; // Quantities of [ROSE, SUNFLOWER, TULIP]
    double price[3]; // Prices for each flower type
};

struct Buyer
{
    char name[20];
    int demand[3]; // Demands of [ROSE, SUNFLOWER, TULIP]
    double budget;
};

// Helper function to print status
void printStatus(const std::vector<Seller> &sellers, const std::vector<Buyer> &buyers)
{
    std::cout << "\nCurrent Seller Stocks:\n";
    for (auto &seller : sellers)
    {
        std::cout << seller.name << " has ";
        for (int i = 0; i < 3; ++i)
            std::cout << seller.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
    }

    std::cout << "\nCurrent Buyer Status:\n";
    for (auto &buyer : buyers)
    {
        std::cout << buyer.name << " wants ";
        for (int i = 0; i < 3; ++i)
            std::cout << buyer.demand[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "");
        std::cout << " and has $" << buyer.budget << " left\n";
    }
    std::cout << std::endl;
}

// Check if all buyers have zero demand
bool allDemandsFulfilled(const std::vector<Buyer> &buyers)
{
    for (auto &buyer : buyers)
        for (int i = 0; i < 3; ++i)
            if (buyer.demand[i] > 0)
                return false;
    return true;
}

int main()
{
    std::vector<Seller> sellers = {
        {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
        {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
        {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

    std::vector<Buyer> buyers = {
        {"Dan", {10, 5, 2}, 50},
        {"Eve", {5, 5, 0}, 30},
        {"Fay", {15, 10, 5}, 100}};

    bool market_open = true;
    int round = 0;

    // OpenMP parallel region
#pragma omp parallel sections
    {
        // Section 1: Buyers dynamically placing new orders (simulated here randomly)
#pragma omp section
        {
            while (market_open)
            {
// Simulate some buyers adding demand randomly every 2 seconds
#pragma omp critical
                {
                    int b = rand() % buyers.size();
                    int f = rand() % 3;
                    int add_demand = (rand() % 3); // 0-2 new flowers

                    buyers[b].demand[f] += add_demand;

                    if (add_demand > 0)
                    {
                        std::cout << buyers[b].name << " placed new demand for "
                                  << add_demand << " " << FlowerNames[f] << "(s)\n";
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        // Section 2: Market matching buyers and sellers continuously
#pragma omp section
        {
            while (market_open)
            {
                bool any_trade = false;
// Match buyers to sellers for each flower
#pragma omp parallel for collapse(2) schedule(dynamic)
                for (int i = 0; i < buyers.size(); ++i)
                {
                    for (int flower = 0; flower < 3; ++flower)
                    {
                        Buyer &buyer = buyers[i];
                        for (int j = 0; j < sellers.size(); ++j)
                        {
                            Seller &seller = sellers[j];

#pragma omp critical
                            {
                                if (buyer.demand[flower] > 0 && seller.quantity[flower] > 0 && buyer.budget >= seller.price[flower])
                                {
                                    int affordable = (int)(buyer.budget / seller.price[flower]);
                                    int buy_amount = std::min({affordable, buyer.demand[flower], seller.quantity[flower]});
                                    double cost = buy_amount * seller.price[flower];

                                    if (buy_amount > 0)
                                    {
                                        buyer.demand[flower] -= buy_amount;
                                        buyer.budget -= cost;
                                        seller.quantity[flower] -= buy_amount;

                                        std::cout << buyer.name << " bought " << buy_amount << " " << FlowerNames[flower]
                                                  << "(s) from " << seller.name << " for $" << cost << "\n";

                                        any_trade = true;
                                    }
                                }
                            }
                        }
                    }
                }

                round++;
                if (round % 5 == 0)
                {
#pragma omp critical
                    printStatus(sellers, buyers);
                }

                if (allDemandsFulfilled(buyers))
                {
#pragma omp critical
                    std::cout << "All buyers' demands have been fulfilled. Market closing.\n";
                    market_open = false;
                }

                if (!any_trade)
                {
// No trades happened in this round, maybe market is stuck, close it
#pragma omp critical
                    std::cout << "No trades in this round, market closing.\n";
                    market_open = false;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    std::cout << "\nFinal Market Status:\n";
    printStatus(sellers, buyers);

    return 0;
}
