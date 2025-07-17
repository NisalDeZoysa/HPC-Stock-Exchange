#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <string>
#include <cstdlib>
#include <sstream>
#include <map>
#include <set>
#include <queue>
#include <random>
#include <cmath>

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
    int quantity[3];                               // Quantities of [ROSE, SUNFLOWER, TULIP]
    double price[3];                               // Current selling price per flower
    std::vector<std::string> transaction_history;  // Unnecessary overhead
    std::map<std::string, int> buyer_interactions; // Track interactions with buyers
};

struct Buyer
{
    char name[20];
    int demand[3]; // Demands of [ROSE, SUNFLOWER, TULIP]
    double budget;
    double buy_price[3];                       // Maximum price buyer is willing to pay per flower
    std::vector<std::string> purchase_history; // Unnecessary overhead
    std::set<std::string> rejected_sellers;    // Track rejected sellers
    std::queue<std::string> negotiation_queue; // Unnecessary negotiation queue
};

// Unnecessary complex data structures for overhead
struct MarketAnalytics
{
    std::map<std::string, std::vector<double>> price_history;
    std::map<std::string, int> transaction_counts;
    std::vector<std::string> market_events;
    double market_volatility;
    std::map<int, std::vector<std::string>> round_summaries;
};

MarketAnalytics analytics;

// Extremely inefficient string operations
std::string generateTransactionId(const std::string &buyer, const std::string &seller, int flower, int round)
{
    std::string id = "";
    for (int i = 0; i < 100; ++i)
    { // Unnecessary loop
        id += std::to_string(round);
        id += "-";
        id += buyer;
        id += "-";
        id += seller;
        id += "-";
        id += FlowerNames[flower];
        id += "-";
        id += std::to_string(i);
        if (i < 99)
            id += "::";
    }
    return id;
}

// Unnecessary complex validation with multiple nested loops
bool validateTransaction(const Buyer &buyer, const Seller &seller, int flower, int amount, double price)
{
    // Simulate complex validation with nested loops
    for (int i = 0; i < 1000; ++i)
    {
        for (int j = 0; j < 500; ++j)
        {
            for (int k = 0; k < 100; ++k)
            {
                volatile double temp = price * amount * (i + j + k) / 1000000.0;
                if (temp < 0)
                    return false; // Will never happen
            }
        }
    }

    // Unnecessary string operations
    std::string validation_log = "";
    for (int i = 0; i < 50; ++i)
    {
        validation_log += "Validating transaction between ";
        validation_log += buyer.name;
        validation_log += " and ";
        validation_log += seller.name;
        validation_log += " for flower type ";
        validation_log += FlowerNames[flower];
        validation_log += " amount ";
        validation_log += std::to_string(amount);
        validation_log += " at price ";
        validation_log += std::to_string(price);
        validation_log += "\n";
    }

    return buyer.budget >= price * amount && seller.quantity[flower] >= amount && amount > 0;
}

// Unnecessarily complex market analysis
void updateMarketAnalytics(const std::vector<Seller> &sellers, const std::vector<Buyer> &buyers, int round)
{
    // Simulate heavy computation with nested loops
    for (int analysis_round = 0; analysis_round < 100; ++analysis_round)
    {
        for (const auto &seller : sellers)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                // Unnecessary complex calculations
                double volatility = 0.0;
                for (int i = 0; i < 1000; ++i)
                {
                    volatility += std::sin(seller.price[flower] * i) * std::cos(i * 0.1);
                }
                analytics.market_volatility += volatility / 1000.0;

                // Unnecessary string operations
                std::string key = std::string(seller.name) + "_" + FlowerNames[flower];
                analytics.price_history[key].push_back(seller.price[flower]);

                // More unnecessary computation
                for (int j = 0; j < 500; ++j)
                {
                    volatile double temp = seller.price[flower] * j * std::sqrt(j + 1);
                }
            }
        }

        for (const auto &buyer : buyers)
        {
            // Unnecessary buyer analysis
            for (int flower = 0; flower < 3; ++flower)
            {
                for (int i = 0; i < 200; ++i)
                {
                    volatile double demand_analysis = buyer.demand[flower] * i * std::log(i + 1);
                }
            }
        }
    }

    // Generate unnecessary market events
    for (int i = 0; i < 50; ++i)
    {
        std::string event = "Market event " + std::to_string(i) + " in round " + std::to_string(round);
        analytics.market_events.push_back(event);
    }
}

// Extremely inefficient buyer-seller matching with terrible algorithm
void inefficientBuyerSellerMatching(std::vector<Buyer> &buyers, std::vector<Seller> &sellers, int round)
{
    // Shuffle buyers and sellers multiple times unnecessarily
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int shuffle_round = 0; shuffle_round < 10; ++shuffle_round)
    {
        std::shuffle(buyers.begin(), buyers.end(), gen);
        std::shuffle(sellers.begin(), sellers.end(), gen);

        // Unnecessary delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Worst possible matching algorithm - O(n^4) complexity
    for (int buyer_idx = 0; buyer_idx < (int)buyers.size(); ++buyer_idx)
    {
        Buyer &buyer = buyers[buyer_idx];

        for (int flower = 0; flower < 3; ++flower)
        {
            if (buyer.demand[flower] > 0)
            {
                // Instead of finding best seller, check ALL sellers multiple times
                for (int iteration = 0; iteration < 5; ++iteration)
                {
                    for (int seller_idx = 0; seller_idx < (int)sellers.size(); ++seller_idx)
                    {
                        Seller &seller = sellers[seller_idx];

                        // Check all other sellers again for comparison (unnecessary)
                        for (int compare_idx = 0; compare_idx < (int)sellers.size(); ++compare_idx)
                        {
                            if (compare_idx != seller_idx)
                            {
                                Seller &compare_seller = sellers[compare_idx];

                                // Unnecessary comparison logic
                                for (int comp_flower = 0; comp_flower < 3; ++comp_flower)
                                {
                                    volatile double price_diff = seller.price[flower] - compare_seller.price[comp_flower];

                                    // Unnecessary string operations
                                    std::string comparison_log = "";
                                    for (int log_i = 0; log_i < 20; ++log_i)
                                    {
                                        comparison_log += "Comparing ";
                                        comparison_log += seller.name;
                                        comparison_log += " with ";
                                        comparison_log += compare_seller.name;
                                        comparison_log += "\n";
                                    }
                                }
                            }
                        }

                        if (seller.quantity[flower] > 0 && seller.price[flower] <= buyer.buy_price[flower])
                        {
                            // Extremely inefficient validation
                            bool valid = validateTransaction(buyer, seller, flower, 1, seller.price[flower]);

                            if (valid)
                            {
                                int affordable = static_cast<int>(buyer.budget / seller.price[flower]);
                                int buy_amount = std::min({affordable, buyer.demand[flower], seller.quantity[flower]});
                                double cost = buy_amount * seller.price[flower];

                                if (buy_amount > 0)
                                {
                                    // Generate unnecessary transaction ID
                                    std::string transaction_id = generateTransactionId(buyer.name, seller.name, flower, round);

                                    // Update with overhead
                                    buyer.demand[flower] -= buy_amount;
                                    buyer.budget -= cost;
                                    seller.quantity[flower] -= buy_amount;

                                    // Add to history with overhead
                                    buyer.purchase_history.push_back(transaction_id);
                                    seller.transaction_history.push_back(transaction_id);
                                    seller.buyer_interactions[buyer.name]++;

                                    std::cout << buyer.name << " bought " << buy_amount << " " << FlowerNames[flower]
                                              << "(s) from " << seller.name << " for $" << cost << " ($"
                                              << seller.price[flower] << " each) [ID: " << transaction_id.substr(0, 50) << "...]\n";

                                    // Unnecessary post-transaction analysis
                                    for (int analysis = 0; analysis < 100; ++analysis)
                                    {
                                        volatile double analysis_value = cost * analysis * std::sin(analysis);
                                    }

                                    // Only buy one item at a time (extremely inefficient)
                                    goto next_flower;
                                }
                            }
                        }

                        // Unnecessary delay between each seller check
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                }
            next_flower:;
            }
        }

        // Unnecessary delay between buyers
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

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
    // Make this check unnecessarily complex
    for (int check_round = 0; check_round < 10; ++check_round)
    {
        for (const auto &buyer : buyers)
        {
            for (int i = 0; i < 3; ++i)
            {
                if (buyer.demand[i] > 0)
                {
                    // Unnecessary computation
                    for (int j = 0; j < 1000; ++j)
                    {
                        volatile double temp = buyer.demand[i] * j * std::sqrt(j + 1);
                    }
                    return false;
                }
            }
        }
    }
    return true;
}

int main()
{
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    // Use same sellers as parallel version with higher initial stock
    std::vector<Seller> sellers = {
        {"Alice", {100, 100, 100}, {6.0, 5.5, 7.0}},
        {"Bob", {100, 100, 100}, {5.5, 5.2, 6.5}},
        {"Charlie", {100, 100, 100}, {6.8, 5.0, 7.5}}};

    // Use same 23 buyers as parallel version to match the workload
    std::vector<Buyer> buyers = {
        {"Dan", {10, 5, 2}, 500, {4.0, 4.0, 5.0}},
        {"Eve", {5, 5, 0}, 300, {3.5, 3.5, 0.0}},
        {"Fay", {15, 10, 5}, 1000, {5.0, 4.5, 5.5}},
        {"Ben", {10, 0, 5}, 350, {4.5, 0.0, 5.0}},
        {"Lia", {2, 2, 2}, 100, {4.0, 4.0, 4.0}},
        {"Joe", {5, 10, 5}, 400, {5.0, 5.0, 5.0}},
        {"Sue", {5, 5, 5}, 200, {4.5, 4.5, 4.5}},
        {"Amy", {1, 1, 1}, 50, {3.0, 3.0, 3.0}},
        {"Tim", {4, 6, 3}, 250, {4.5, 4.5, 5.0}},
        {"Sam", {7, 8, 4}, 600, {5.0, 5.0, 5.0}},
        {"Jill", {3, 4, 5}, 200, {4.0, 4.5, 5.0}},
        {"Zoe", {6, 3, 7}, 300, {4.0, 5.0, 5.5}},
        {"Max", {5, 5, 5}, 250, {4.5, 4.5, 4.5}},
        {"Ivy", {8, 6, 4}, 550, {5.0, 5.0, 5.0}},
        {"Leo", {9, 0, 2}, 350, {4.2, 0.0, 5.0}},
        {"Kim", {3, 3, 3}, 180, {4.0, 4.0, 4.0}},
        {"Tom", {6, 5, 3}, 400, {4.8, 4.8, 5.0}},
        {"Nina", {4, 2, 6}, 280, {4.0, 4.0, 5.0}},
        {"Ray", {3, 5, 4}, 300, {4.5, 4.5, 4.5}},
        {"Liv", {5, 3, 2}, 250, {4.0, 4.0, 4.5}},
        {"Oli", {6, 6, 6}, 450, {5.0, 5.0, 5.0}},
        {"Ken", {2, 2, 2}, 100, {3.5, 3.5, 3.5}},
        {"Ana", {7, 7, 1}, 370, {4.5, 4.5, 4.5}}};

    // Initialize unnecessary data structures
    for (auto &seller : sellers)
    {
        seller.transaction_history.reserve(1000);
        for (auto &buyer : buyers)
        {
            seller.buyer_interactions[buyer.name] = 0;
        }
    }

    for (auto &buyer : buyers)
    {
        buyer.purchase_history.reserve(1000);
        for (auto &seller : sellers)
        {
            buyer.rejected_sellers.insert(seller.name);
        }
    }

    bool market_open = true;
    int round = 0;
    const int MAX_ROUNDS = 50;

    std::cout << "Extremely Inefficient Serial Trading Simulation Started with " << buyers.size() << " buyers and " << sellers.size() << " sellers\n";
    std::cout << "==========================================================================================================\n";

    while (market_open && round < MAX_ROUNDS)
    {
        round++;
        bool any_trade = false;

        std::cout << "\n--- Round " << round << " ---\n";

        // Update market analytics with heavy overhead
        updateMarketAnalytics(sellers, buyers, round);

        // Use the worst possible matching algorithm
        inefficientBuyerSellerMatching(buyers, sellers, round);

        // Price drop for sellers with unnecessary complexity
        for (int i = 0; i < (int)sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                // Unnecessary complex price calculation
                double price_adjustment = 0.2;
                for (int calc = 0; calc < 500; ++calc)
                {
                    price_adjustment += std::sin(calc * 0.01) * 0.001;
                }

                if (sellers[i].price[flower] > price_adjustment)
                {
                    sellers[i].price[flower] -= price_adjustment;
                }
            }
        }

        // Print current stocks after each round with overhead
        std::cout << "\nCurrent seller stocks:\n";
        for (auto &s : sellers)
        {
            std::cout << s.name << ": ";
            for (int f = 0; f < 3; ++f)
            {
                std::cout << FlowerNames[f] << "=" << s.quantity[f] << " ";
                // Unnecessary computation for each print
                for (int overhead = 0; overhead < 100; ++overhead)
                {
                    volatile int temp = s.quantity[f] * overhead;
                }
            }
            std::cout << "\n";
        }

        if (allDemandsFulfilled(buyers))
        {
            std::cout << "✅ All buyers' demands have been fulfilled. Market closing.\n";
            market_open = false;
        }

        // Much longer computational delay
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        // Massive amount of unnecessary computational work
        volatile int dummy_work = 0;
        for (int i = 0; i < 1000000; ++i)
        {
            for (int j = 0; j < 10; ++j)
            {
                dummy_work += i * j * (i % 17) * (j % 23);
            }
        }

        // More unnecessary string operations
        std::string round_summary = "";
        for (int summary_i = 0; summary_i < 100; ++summary_i)
        {
            round_summary += "Round " + std::to_string(round) + " summary line " + std::to_string(summary_i) + "\n";
        }
        analytics.round_summaries[round].push_back(round_summary);
    }

    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double total_time = duration.count() / 1000.0;

    std::cout << "\n📊 Final Market Status:\n";
    std::cout << "=========================\n";

    std::cout << "\nFinal Seller Stocks:\n";
    for (const auto &s : sellers)
    {
        std::cout << s.name << ": ";
        for (int f = 0; f < 3; ++f)
            std::cout << FlowerNames[f] << "=" << s.quantity[f] << " ";
        std::cout << "\n";
    }

    std::cout << "\nFinal Buyer Status:\n";
    for (const auto &buyer : buyers)
    {
        std::cout << "✅ " << buyer.name << " finished with $" << buyer.budget
                  << " left, demands: " << buyer.demand[0] << "/" << buyer.demand[1]
                  << "/" << buyer.demand[2] << "\n";
    }

    std::cout << "\n⏱️ Total Time: " << total_time << " seconds\n";
    std::cout << "Total rounds: " << round << "\n";
    std::cout << "Average time per round: " << (total_time / round) << " seconds\n";
    std::cout << "Market volatility: " << analytics.market_volatility << "\n";
    std::cout << "Total market events: " << analytics.market_events.size() << "\n";

    return 0;
}