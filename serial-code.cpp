#include <iostream>
#include <vector>
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
    int quantity[3]; // Quantities of [ROSE, SUNFLOWER, TULIP]
    double price[3]; // Current selling price per flower
};

struct Buyer
{
    char name[20];
    int demand[3]; // Demands of [ROSE, SUNFLOWER, TULIP]
    double budget;
    double buy_price[3]; // Maximum price buyer is willing to pay per flower
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

int main()
{
    std::vector<Seller> sellers = {
        {"Alice", {30, 10, 20}, {6.0, 5.5, 7.0}},
        {"Bob", {20, 20, 10}, {5.5, 5.2, 6.5}},
        {"Charlie", {10, 5, 10}, {6.8, 5.0, 7.5}}};

    std::vector<Buyer> buyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}},
        {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}},
        {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}}};

    bool market_open = true;
    int round = 0;

    while (market_open)
    {
        bool any_trade = false;

        // Serial buyer-seller matching loop (no parallelism)
        for (int i = 0; i < buyers.size(); ++i)
        {
            Buyer &buyer = buyers[i];
            for (int flower = 0; flower < 3; ++flower)
            {
                for (int j = 0; j < sellers.size(); ++j)
                {
                    Seller &seller = sellers[j];

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

        // Price drop for sellers (serial)
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                if (sellers[i].price[flower] > 0.2)
                    sellers[i].price[flower] -= 0.2;
            }
        }

        round++;
        if (round % 5 == 0)
        {
            printStatus(sellers, buyers);
        }

        if (allDemandsFulfilled(buyers))
        {
            std::cout << "✅ All buyers' demands have been fulfilled. Market closing.\n";
            market_open = false;
        }

        if (!any_trade)
        {
            std::cout << "No trades in this round. Prices dropping...\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\n📊 Final Market Status:\n";
    printStatus(sellers, buyers);

    return 0;
}
