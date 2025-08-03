#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <omp.h>
#include <algorithm>

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

class FlowerMarketOMP
{
private:
    std::vector<Seller> sellers;
    std::vector<Buyer> buyers;
    std::vector<Buyer> serial_buyers; // For comparison

public:
    FlowerMarketOMP()
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

        // Copy for serial comparison
        serial_buyers = buyers;
    }

    void simulate()
    {
        std::cout << "=== OPENMP VERSION ===" << std::endl;

        // First run serial version for comparison
        runSerial();

        // Reset sellers and buyers for OpenMP version
        resetMarket();

        auto start = std::chrono::high_resolution_clock::now();

// OpenMP parallel processing of buyers
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < buyers.size(); i++)
        {
            processBuyerParallel(buyers[i]);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        printResults();
        compareWithSerial();
        std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    }

    void processBuyerParallel(Buyer &buyer)
    {
        // Try to buy roses with critical sections for thread safety
        for (auto &seller : sellers)
        {
#pragma omp critical
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
        }

        // Try to buy tulips
        for (auto &seller : sellers)
        {
#pragma omp critical
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
        }

        // Try to buy lilies
        for (auto &seller : sellers)
        {
#pragma omp critical
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
    }

    void runSerial()
    {
        // Reset for serial run
        std::vector<Seller> serial_sellers = {
            Seller(50, 40, 30, 5.0, 3.0, 4.0),
            Seller(60, 35, 45, 4.5, 3.5, 3.8),
            Seller(40, 50, 35, 5.2, 2.8, 4.2)};

        for (auto &buyer : serial_buyers)
        {
            // Reset buyer purchases
            buyer.roses_bought = buyer.tulips_bought = buyer.lilies_bought = 0;
            buyer.rose_budget = (buyer.id == 0) ? 25.0 : (buyer.id == 1) ? 20.0
                                                     : (buyer.id == 2)   ? 35.0
                                                     : (buyer.id == 3)   ? 15.0
                                                     : (buyer.id == 4)   ? 22.0
                                                     : (buyer.id == 5)   ? 30.0
                                                     : (buyer.id == 6)   ? 8.0
                                                     : (buyer.id == 7)   ? 28.0
                                                     : (buyer.id == 8)   ? 18.0
                                                                         : 24.0;
            buyer.tulip_budget = (buyer.id == 0) ? 15.0 : (buyer.id == 1) ? 18.0
                                                      : (buyer.id == 2)   ? 12.0
                                                      : (buyer.id == 3)   ? 20.0
                                                      : (buyer.id == 4)   ? 16.0
                                                      : (buyer.id == 5)   ? 8.0
                                                      : (buyer.id == 6)   ? 25.0
                                                      : (buyer.id == 7)   ? 14.0
                                                      : (buyer.id == 8)   ? 20.0
                                                                          : 18.0;
            buyer.lily_budget = (buyer.id == 0) ? 10.0 : (buyer.id == 1) ? 15.0
                                                     : (buyer.id == 2)   ? 20.0
                                                     : (buyer.id == 3)   ? 8.0
                                                     : (buyer.id == 4)   ? 25.0
                                                     : (buyer.id == 5)   ? 18.0
                                                     : (buyer.id == 6)   ? 12.0
                                                     : (buyer.id == 7)   ? 22.0
                                                     : (buyer.id == 8)   ? 6.0
                                                                         : 30.0;

            processBuyerSerial(buyer, serial_sellers);
        }
    }

    void processBuyerSerial(Buyer &buyer, std::vector<Seller> &sellers)
    {
        // Same logic as serial version
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

    void resetMarket()
    {
        // Reset sellers
        sellers.clear();
        sellers.push_back(Seller(50, 40, 30, 5.0, 3.0, 4.0));
        sellers.push_back(Seller(60, 35, 45, 4.5, 3.5, 3.8));
        sellers.push_back(Seller(40, 50, 35, 5.2, 2.8, 4.2));

        // Reset buyers
        for (auto &buyer : buyers)
        {
            buyer.roses_bought = buyer.tulips_bought = buyer.lilies_bought = 0;
            buyer.rose_budget = (buyer.id == 0) ? 25.0 : (buyer.id == 1) ? 20.0
                                                     : (buyer.id == 2)   ? 35.0
                                                     : (buyer.id == 3)   ? 15.0
                                                     : (buyer.id == 4)   ? 22.0
                                                     : (buyer.id == 5)   ? 30.0
                                                     : (buyer.id == 6)   ? 8.0
                                                     : (buyer.id == 7)   ? 28.0
                                                     : (buyer.id == 8)   ? 18.0
                                                                         : 24.0;
            buyer.tulip_budget = (buyer.id == 0) ? 15.0 : (buyer.id == 1) ? 18.0
                                                      : (buyer.id == 2)   ? 12.0
                                                      : (buyer.id == 3)   ? 20.0
                                                      : (buyer.id == 4)   ? 16.0
                                                      : (buyer.id == 5)   ? 8.0
                                                      : (buyer.id == 6)   ? 25.0
                                                      : (buyer.id == 7)   ? 14.0
                                                      : (buyer.id == 8)   ? 20.0
                                                                          : 18.0;
            buyer.lily_budget = (buyer.id == 0) ? 10.0 : (buyer.id == 1) ? 15.0
                                                     : (buyer.id == 2)   ? 20.0
                                                     : (buyer.id == 3)   ? 8.0
                                                     : (buyer.id == 4)   ? 25.0
                                                     : (buyer.id == 5)   ? 18.0
                                                     : (buyer.id == 6)   ? 12.0
                                                     : (buyer.id == 7)   ? 22.0
                                                     : (buyer.id == 8)   ? 6.0
                                                                         : 30.0;
        }
    }

    void printResults() const
    {
        std::cout << "\nOpenMP Buyer Results:" << std::endl;
        std::cout << "ID\tRoses(D/B/R)\tTulips(D/B/R)\tLilies(D/B/R)" << std::endl;
        for (const auto &buyer : buyers)
        {
            std::cout << buyer.id << "\t"
                      << buyer.rose_demand << "/" << buyer.roses_bought << "/" << buyer.remaining_rose_demand() << "\t\t"
                      << buyer.tulip_demand << "/" << buyer.tulips_bought << "/" << buyer.remaining_tulip_demand() << "\t\t"
                      << buyer.lily_demand << "/" << buyer.lilies_bought << "/" << buyer.remaining_lily_demand() << std::endl;
        }
    }

    void compareWithSerial() const
    {
        std::cout << "\n=== ACCURACY COMPARISON ===" << std::endl;
        std::cout << "Buyer\tSerial Remaining\tOpenMP Remaining\tMatch?" << std::endl;
        std::cout << "\t(R/T/L)\t\t(R/T/L)" << std::endl;

        bool perfect_match = true;
        for (int i = 0; i < buyers.size(); i++)
        {
            bool match = (serial_buyers[i].remaining_rose_demand() == buyers[i].remaining_rose_demand() &&
                          serial_buyers[i].remaining_tulip_demand() == buyers[i].remaining_tulip_demand() &&
                          serial_buyers[i].remaining_lily_demand() == buyers[i].remaining_lily_demand());

            if (!match)
                perfect_match = false;

            std::cout << buyers[i].id << "\t"
                      << serial_buyers[i].remaining_rose_demand() << "/"
                      << serial_buyers[i].remaining_tulip_demand() << "/"
                      << serial_buyers[i].remaining_lily_demand() << "\t\t\t"
                      << buyers[i].remaining_rose_demand() << "/"
                      << buyers[i].remaining_tulip_demand() << "/"
                      << buyers[i].remaining_lily_demand() << "\t\t"
                      << (match ? "YES" : "NO") << std::endl;
        }

        std::cout << "\nOverall Accuracy: " << (perfect_match ? "100%" : "< 100%") << std::endl;
        std::cout << "Number of threads used: " << omp_get_max_threads() << std::endl;
    }
};

int main()
{
    FlowerMarketOMP market;
    market.simulate();
    return 0;
}