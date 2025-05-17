#include <iostream>
#include <vector>
#include <omp.h>

struct Seller
{
    char name[20];
    int quantity;
    double price;
};

struct Buyer
{
    char name[20];
    int demand;
    double budget;
};

int main()
{
    // Sample sellers
    std::vector<Seller> sellers = {
        {"Alice", 50, 2.5},
        {"Bob", 30, 2.0},
        {"Charlie", 20, 3.0}};

    // Sample buyers
    std::vector<Buyer> buyers = {
        {"Eve", 10, 30},
        {"Dan", 25, 50},
        {"Fay", 40, 100}};

// Parallel region: match buyers to sellers
#pragma omp parallel for
    for (int i = 0; i < buyers.size(); ++i)
    {
        Buyer &buyer = buyers[i];
        for (int j = 0; j < sellers.size(); ++j)
        {
            Seller &seller = sellers[j];

// Critical section: avoid race conditions when updating seller
#pragma omp critical
            {
                if (buyer.demand > 0 && seller.quantity > 0 && buyer.budget >= seller.price)
                {
                    int can_buy = std::min(buyer.demand, seller.quantity);
                    double cost = can_buy * seller.price;

                    if (buyer.budget >= cost)
                    {
                        // Full demand can be met
                        buyer.demand -= can_buy;
                        buyer.budget -= cost;
                        seller.quantity -= can_buy;

                        std::cout << buyer.name << " bought " << can_buy << " flowers from "
                                  << seller.name << " for $" << cost << "\n";
                    }
                    else
                    {
                        // Buy as much as budget allows
                        int affordable = (int)(buyer.budget / seller.price);
                        int buy_amount = std::min(std::min(affordable, buyer.demand), seller.quantity);
                        double partial_cost = buy_amount * seller.price;

                        if (buy_amount > 0)
                        {
                            buyer.demand -= buy_amount;
                            buyer.budget -= partial_cost;
                            seller.quantity -= buy_amount;

                            std::cout << buyer.name << " bought " << buy_amount << " flowers from "
                                      << seller.name << " for $" << partial_cost << "\n";
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
        std::cout << seller.name << " has " << seller.quantity << " flowers left\n";
    }

    std::cout << "\nFinal Buyer Status:\n";
    for (auto &buyer : buyers)
    {
        std::cout << buyer.name << " wants " << buyer.demand << " more flowers and has $" << buyer.budget << " left\n";
    }

    return 0;
}
