#include <iostream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <string>
#include <cstdlib>
#include <sstream>

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
    int demand[3]; // Demands of [ROSE, SUNFLOWER, TULIP]
    double budget;
    double buy_price[3]; // buyer price
};

void printStatus(const std::vector<Seller> &sellers, const std::vector<Buyer> &buyers)
{
    std::cout << "\nCurrent Seller Stocks:\n";
    for (auto &seller : sellers)
    {
        std::cout << seller.name << " has ";
        for (int i = 0; i < 3; ++i)
            std::cout << seller.quantity[i] << " " << FlowerNames[i] << " ($" << seller.price[i] << ")" << (i < 2 ? ", " : "\n");
    }

    std::cout << "\nCurrent Buyer Status:\n";
    for (auto &buyer : buyers)
    {
        std::cout << buyer.name << " wants ";
        for (int i = 0; i < 3; ++i)
            std::cout << buyer.demand[i] << " " << FlowerNames[i] << " (max $" << buyer.buy_price[i] << ")" << (i < 2 ? ", " : "");
        std::cout << " and has $" << buyer.budget << " left\n";
    }
    std::cout << std::endl;
}

bool allDemandsFulfilled(const std::vector<Buyer> &buyers)
{
    for (const auto &buyer : buyers)
        for (int i = 0; i < 3; ++i)
            if (buyer.demand[i] > 0)
                return false;
    return true;
}

bool allSellersOutOfStock(const std::vector<Seller> &sellers)
{
    for (const auto &s : sellers)
        for (int i = 0; i < 3; ++i)
            if (s.quantity[i] > 0)
                return false;
    return true;
}

int main()
{
    omp_set_num_threads(6);
    std::vector<Seller> sellers = {
        {"Alice", {30, 10, 20}, {6.0, 5.5, 7.0}},
        {"Bob", {20, 20, 10}, {5.5, 5.2, 6.5}},
        {"Charlie", {10, 5, 10}, {6.8, 5.0, 7.5}}};

    std::vector<Buyer> buyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}}, {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}}, {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}}, {"Ben", {10, 0, 5}, 350, {4.5, 0.0, 5.0}}, {"Lia", {2, 2, 2}, 100, {4.0, 4.0, 4.0}}, {"Joe", {5, 10, 5}, 400, {5.0, 5.0, 5.0}}, {"Sue", {5, 5, 5}, 200, {4.5, 4.5, 4.5}}, {"Amy", {1, 1, 1}, 50, {3.0, 3.0, 3.0}}, {"Tim", {4, 6, 3}, 250, {4.5, 4.5, 5.0}}, {"Sam", {7, 8, 4}, 600, {5.0, 5.0, 5.0}}, {"Jill", {3, 4, 5}, 200, {4.0, 4.5, 5.0}}, {"Zoe", {6, 3, 7}, 300, {4.0, 5.0, 5.5}}, {"Max", {5, 5, 5}, 250, {4.5, 4.5, 4.5}}, {"Ivy", {8, 6, 4}, 550, {5.0, 5.0, 5.0}}, {"Leo", {9, 0, 2}, 350, {4.2, 0.0, 5.0}}, {"Kim", {3, 3, 3}, 180, {4.0, 4.0, 4.0}}, {"Tom", {6, 5, 3}, 400, {4.8, 4.8, 5.0}}, {"Nina", {4, 2, 6}, 280, {4.0, 4.0, 5.0}}, {"Ray", {3, 5, 4}, 300, {4.5, 4.5, 4.5}}, {"Liv", {5, 3, 2}, 250, {4.0, 4.0, 4.5}}, {"Oli", {6, 6, 6}, 450, {5.0, 5.0, 5.0}}, {"Ken", {2, 2, 2}, 100, {3.5, 3.5, 3.5}}, {"Ana", {7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    bool market_open = true;
    int round = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (market_open)
    {
        bool any_trade = false;

        // Buyer-seller matching loop
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
                        if (buyer.demand[flower] > 0 &&
                            seller.quantity[flower] > 0 &&
                            seller.price[flower] <= buyer.buy_price[flower] &&
                            buyer.budget >= seller.price[flower])
                        {
                            int affordable = static_cast<int>(buyer.budget / seller.price[flower]);
                            int buy_amount = std::min({affordable, buyer.demand[flower], seller.quantity[flower]});
                            double cost = buy_amount * seller.price[flower];

                            if (buy_amount > 0)
                            {
                                buyer.demand[flower] -= buy_amount;
                                buyer.budget -= cost;
                                seller.quantity[flower] -= buy_amount;

                                std::cout << buyer.name << " bought " << buy_amount << " " << FlowerNames[flower]
                                          << "(s) from " << seller.name << " for $" << cost << " ($"
                                          << seller.price[flower] << " each)\n";

                                any_trade = true;
                            }
                        }
                    }
                }
            }
        }

        // Price drop for sellers
#pragma omp parallel for collapse(2)
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
#pragma omp critical
                {
                    if (sellers[i].price[flower] > 0.2)
                        sellers[i].price[flower] -= 0.2;
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
            {
                std::cout << "✅ All buyers' demands have been fulfilled. Market closing.\n";
                market_open = false;
            }
        }

        if (!any_trade)
        {
#pragma omp critical
            std::cout << "No trades in this round. Prices dropping...\n";

            if (allSellersOutOfStock(sellers))
            {
#pragma omp critical
                std::cout << "❌ All sellers are out of stock. Market closing.\n";
                market_open = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "\n📊 Final Market Status:\n";
    printStatus(sellers, buyers);

    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "⏱️ Total simulation time: " << duration.count() << " seconds\n";

    return 0;
}
