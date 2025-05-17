#include <iostream>
#include <vector>
#include <omp.h>
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
    int quantity[3]; // Quantities of [ROSE, SUNFLOWER, TULIP]
    double price[3]; // Prices for each flower type
};

struct Buyer
{
    char name[20];
    int demand[3]; // Demands of [ROSE, SUNFLOWER, TULIP]
    double budget;
};

int main()
{
    // Sample sellers
    std::vector<Seller> sellers = {
        {"Alice", {30, 10, 20}, {2.0, 3.0, 4.0}},
        {"Bob", {20, 20, 10}, {2.5, 2.8, 3.5}},
        {"Charlie", {10, 5, 10}, {1.8, 2.5, 4.2}}};

    // Sample buyers
    std::vector<Buyer> buyers = {
        {"Dan", {10, 5, 2}, 50},
        {"Eve", {5, 5, 0}, 30},
        {"Fay", {15, 10, 5}, 100}};

// Parallel region: match buyers to sellers for each flower type
#pragma omp parallel for collapse(2)
    for (int i = 0; i < buyers.size(); ++i)
    {
        for (int flower = 0; flower < 3; ++flower)
        {
            Buyer &buyer = buyers[i];

            for (int j = 0; j < sellers.size(); ++j)
            {
                Seller &seller = sellers[j];

// Critical section: protect shared seller and buyer data
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
                                      << "s from " << seller.name << " for $" << cost << "\n";
                        }
                    }
                }
            }
        }
    }

    // Final stock summary
    std::cout << "\nFinal Seller Stocks:\n";
    for (auto &seller : sellers)
    {
        std::cout << seller.name << " has ";
        for (int i = 0; i < 3; ++i)
            std::cout << seller.quantity[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "\n");
    }

    std::cout << "\nFinal Buyer Status:\n";
    for (auto &buyer : buyers)
    {
        std::cout << buyer.name << " wants ";
        for (int i = 0; i < 3; ++i)
            std::cout << buyer.demand[i] << " " << FlowerNames[i] << (i < 2 ? ", " : "");
        std::cout << " and has $" << buyer.budget << " left\n";
    }

    return 0;
}
