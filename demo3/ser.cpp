#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm> // Add this include for std::min with initializer list

struct Seller
{
    int roses, tulips, lilies;
    double rose_price, tulip_price, lily_price;

    Seller(int r, int t, int l, double rp, double tp, double lp)
        : roses(r), tulips(t), lilies(l), rose_price(rp), tulip_price(tp), lily_price(lp) {}
};

struct Buyer
{
    int id;
    int rose_demand, tulip_demand, lily_demand;
    double rose_budget, tulip_budget, lily_budget;
    int roses_bought, tulips_bought, lilies_bought;

    Buyer(int _id, int rd, int td, int ld, double rb, double tb, double lb)
        : id(_id), rose_demand(rd), tulip_demand(td), lily_demand(ld),
          rose_budget(rb), tulip_budget(tb), lily_budget(lb),
          roses_bought(0), tulips_bought(0), lilies_bought(0) {}

    int remaining_rose_demand() const { return rose_demand - roses_bought; }
    int remaining_tulip_demand() const { return tulip_demand - tulips_bought; }
    int remaining_lily_demand() const { return lily_demand - lilies_bought; }
};

class FlowerMarket
{
private:
    std::vector<Seller> sellers;
    std::vector<Buyer> buyers;

public:
    FlowerMarket()
    {
        // Initialize 3 sellers with different inventories and prices
        sellers.push_back(Seller(50, 40, 30, 5.0, 3.0, 4.0)); // Seller 0
        sellers.push_back(Seller(60, 35, 45, 4.5, 3.5, 3.8)); // Seller 1
        sellers.push_back(Seller(40, 50, 35, 5.2, 2.8, 4.2)); // Seller 2

        // Initialize 10 buyers with different demands and budgets
        buyers.push_back(Buyer(0, 5, 3, 2, 25.0, 15.0, 10.0));
        buyers.push_back(Buyer(1, 3, 4, 3, 20.0, 18.0, 15.0));
        buyers.push_back(Buyer(2, 7, 2, 4, 35.0, 12.0, 20.0));
        buyers.push_back(Buyer(3, 2, 5, 1, 15.0, 20.0, 8.0));
        buyers.push_back(Buyer(4, 4, 3, 5, 22.0, 16.0, 25.0));
        buyers.push_back(Buyer(5, 6, 1, 3, 30.0, 8.0, 18.0));
        buyers.push_back(Buyer(6, 1, 6, 2, 8.0, 25.0, 12.0));
        buyers.push_back(Buyer(7, 5, 2, 4, 28.0, 14.0, 22.0));
        buyers.push_back(Buyer(8, 3, 4, 1, 18.0, 20.0, 6.0));
        buyers.push_back(Buyer(9, 4, 3, 6, 24.0, 18.0, 30.0));
    }

    void simulate()
    {
        std::cout << "=== SERIAL VERSION ===" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        // Process each buyer sequentially
        for (auto &buyer : buyers)
        {
            processBuyer(buyer);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        printResults();
        std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    }

    void processBuyer(Buyer &buyer)
    {
        // Try to buy roses
        for (auto &seller : sellers)
        {
            int can_afford = (int)(buyer.rose_budget / seller.rose_price);
            int to_buy = std::min({buyer.remaining_rose_demand(), seller.roses, can_afford});
            if (to_buy > 0)
            {
                buyer.roses_bought += to_buy;
                seller.roses -= to_buy;
                buyer.rose_budget -= to_buy * seller.rose_price;
            }
        }

        // Try to buy tulips
        for (auto &seller : sellers)
        {
            int can_afford = (int)(buyer.tulip_budget / seller.tulip_price);
            int to_buy = std::min({buyer.remaining_tulip_demand(), seller.tulips, can_afford});
            if (to_buy > 0)
            {
                buyer.tulips_bought += to_buy;
                seller.tulips -= to_buy;
                buyer.tulip_budget -= to_buy * seller.tulip_price;
            }
        }

        // Try to buy lilies
        for (auto &seller : sellers)
        {
            int can_afford = (int)(buyer.lily_budget / seller.lily_price);
            int to_buy = std::min({buyer.remaining_lily_demand(), seller.lilies, can_afford});
            if (to_buy > 0)
            {
                buyer.lilies_bought += to_buy;
                seller.lilies -= to_buy;
                buyer.lily_budget -= to_buy * seller.lily_price;
            }
        }
    }

    void printResults() const
    {
        std::cout << "\nBuyer Results:" << std::endl;
        std::cout << "ID\tRoses(D/B/R)\tTulips(D/B/R)\tLilies(D/B/R)" << std::endl;
        for (const auto &buyer : buyers)
        {
            std::cout << buyer.id << "\t"
                      << buyer.rose_demand << "/" << buyer.roses_bought << "/" << buyer.remaining_rose_demand() << "\t\t"
                      << buyer.tulip_demand << "/" << buyer.tulips_bought << "/" << buyer.remaining_tulip_demand() << "\t\t"
                      << buyer.lily_demand << "/" << buyer.lilies_bought << "/" << buyer.remaining_lily_demand() << std::endl;
        }

        std::cout << "\nSeller Inventory Remaining:" << std::endl;
        for (int i = 0; i < sellers.size(); i++)
        {
            std::cout << "Seller " << i << ": Roses=" << sellers[i].roses
                      << ", Tulips=" << sellers[i].tulips
                      << ", Lilies=" << sellers[i].lilies << std::endl;
        }
    }

    std::vector<Buyer> getBuyers() const { return buyers; }
};

int main()
{
    FlowerMarket market;
    market.simulate();
    return 0;
}