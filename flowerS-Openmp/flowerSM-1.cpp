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
#include <iomanip>
#include <mutex>
#include <numeric>

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
    std::string timestamp;
    int original_quantity[3]; // Track original stock for analytics
};

struct Buyer
{
    char name[20];
    int demand[3];
    int original_demand[3]; // Track original demand
    double budget;
    double original_budget;
    double buy_price[3];
    std::string timestamp;
    int priority; // Higher number = higher priority
};

struct TradeRecord
{
    std::string buyer_name;
    std::string seller_name;
    int flower_type;
    int quantity;
    double price_per_unit;
    double total_cost;
    std::string timestamp;
};

class FlowerMarket
{
private:
    std::vector<Seller> sellers;
    std::vector<Buyer> buyers;
    std::vector<TradeRecord> trade_history;
    std::mutex trade_mutex;
    std::mutex print_mutex;

public:
    // Initialize with hardcoded data
    void initializeMarket()
    {
        // Initialize sellers with timestamps
        sellers = {
            {"Alice", {30, 10, 20}, {6.0, 5.5, 7.0}, "2024-01-15 09:00:00", {30, 10, 20}},
            {"Bob", {20, 20, 10}, {5.5, 5.2, 6.5}, "2024-01-15 09:30:00", {20, 20, 10}},
            {"Charlie", {10, 5, 10}, {6.8, 5.0, 7.5}, "2024-01-15 10:00:00", {10, 5, 10}}};

        // Initialize buyers with timestamps and priorities
        buyers = {
            {"Dan", {10, 5, 2}, {10, 5, 2}, 500, 500, {4.0, 4.0, 5.0}, "2024-01-15 08:00:00", 3},
            {"Eve", {5, 5, 0}, {5, 5, 0}, 300, 300, {3.5, 3.5, 0.0}, "2024-01-15 08:30:00", 1},
            {"Fay", {15, 10, 5}, {15, 10, 5}, 1000, 1000, {5.0, 4.5, 5.5}, "2024-01-15 09:00:00", 2}};
    }

    void printStatus()
    {
        std::lock_guard<std::mutex> lock(print_mutex);

        std::cout << "\n"
                  << std::string(60, '=') << "\n";
        std::cout << "CURRENT MARKET STATUS\n";
        std::cout << std::string(60, '=') << "\n";

        std::cout << "\n📦 SELLER INVENTORY:\n";
        for (const auto &seller : sellers)
        {
            std::cout << "🏪 " << seller.name << " (Added: " << seller.timestamp << ")\n";
            for (int i = 0; i < 3; ++i)
            {
                std::cout << "   " << FlowerNames[i] << ": " << seller.quantity[i]
                          << "/" << seller.original_quantity[i]
                          << " @ $" << std::fixed << std::setprecision(2) << seller.price[i] << "\n";
            }
        }

        std::cout << "\n🛒 BUYER DEMANDS:\n";
        for (const auto &buyer : buyers)
        {
            std::cout << "👤 " << buyer.name << " (Priority: " << buyer.priority
                      << ", Joined: " << buyer.timestamp << ")\n";
            for (int i = 0; i < 3; ++i)
            {
                if (buyer.original_demand[i] > 0)
                {
                    std::cout << "   " << FlowerNames[i] << ": " << buyer.demand[i]
                              << "/" << buyer.original_demand[i]
                              << " (max $" << buyer.buy_price[i] << ")\n";
                }
            }
            std::cout << "   Budget: $" << std::fixed << std::setprecision(2)
                      << buyer.budget << "/$" << buyer.original_budget << "\n";
        }
        std::cout << std::endl;
    }

    bool allDemandsFulfilled()
    {
        for (const auto &buyer : buyers)
            for (int i = 0; i < 3; ++i)
                if (buyer.demand[i] > 0)
                    return false;
        return true;
    }

    // Improved trading system with priority and fairness
    bool conductTradingRound()
    {
        bool any_trade = false;

        // Sort buyers by priority (higher priority first)
        std::vector<int> buyer_indices(buyers.size());
        std::iota(buyer_indices.begin(), buyer_indices.end(), 0);

        std::sort(buyer_indices.begin(), buyer_indices.end(),
                  [this](int a, int b)
                  {
                      return buyers[a].priority > buyers[b].priority;
                  });

        // Process each flower type
        for (int flower = 0; flower < 3; ++flower)
        {
            // Create a list of buyers who want this flower
            std::vector<std::pair<int, int>> interested_buyers; // (buyer_index, demand)

            for (int buyer_idx : buyer_indices)
            {
                if (buyers[buyer_idx].demand[flower] > 0)
                {
                    interested_buyers.push_back({buyer_idx, buyers[buyer_idx].demand[flower]});
                }
            }

            if (interested_buyers.empty())
                continue;

            // Process each seller for this flower type
            for (int seller_idx = 0; seller_idx < sellers.size(); ++seller_idx)
            {
                Seller &seller = sellers[seller_idx];
                if (seller.quantity[flower] <= 0)
                    continue;

                // Find buyers who can afford this seller's price
                std::vector<std::pair<int, int>> eligible_buyers;
                for (auto &buyer_pair : interested_buyers)
                {
                    int buyer_idx = buyer_pair.first;
                    Buyer &buyer = buyers[buyer_idx];

                    if (buyer.buy_price[flower] >= seller.price[flower] &&
                        buyer.budget >= seller.price[flower])
                    {
                        eligible_buyers.push_back(buyer_pair);
                    }
                }

                if (eligible_buyers.empty())
                    continue;

                // Allocation strategy: Priority-based with partial fulfillment
                int total_demand = 0;
                for (auto &buyer_pair : eligible_buyers)
                {
                    total_demand += buyer_pair.second;
                }

                if (total_demand <= seller.quantity[flower])
                {
                    // Enough stock for everyone
                    for (auto &buyer_pair : eligible_buyers)
                    {
                        int buyer_idx = buyer_pair.first;
                        int demand = buyer_pair.second;

                        if (executeTrade(buyer_idx, seller_idx, flower, demand))
                        {
                            any_trade = true;
                        }
                    }
                }
                else
                {
                    // Not enough stock - allocate proportionally with priority weighting
                    int remaining_stock = seller.quantity[flower];

                    // Priority-weighted allocation
                    for (auto &buyer_pair : eligible_buyers)
                    {
                        int buyer_idx = buyer_pair.first;
                        int demand = buyer_pair.second;

                        // Calculate allocation based on priority and remaining stock
                        int allocation = std::min(demand, remaining_stock);

                        if (allocation > 0)
                        {
                            if (executeTrade(buyer_idx, seller_idx, flower, allocation))
                            {
                                any_trade = true;
                                remaining_stock -= allocation;
                            }
                        }

                        if (remaining_stock <= 0)
                            break;
                    }
                }
            }
        }

        return any_trade;
    }

    bool executeTrade(int buyer_idx, int seller_idx, int flower, int quantity)
    {
        std::lock_guard<std::mutex> lock(trade_mutex);

        Buyer &buyer = buyers[buyer_idx];
        Seller &seller = sellers[seller_idx];

        // Double-check availability (thread safety)
        if (buyer.demand[flower] <= 0 || seller.quantity[flower] <= 0 || quantity <= 0)
            return false;

        // Check affordability
        int affordable = static_cast<int>(buyer.budget / seller.price[flower]);
        int actual_quantity = std::min({affordable, buyer.demand[flower], seller.quantity[flower], quantity});

        if (actual_quantity <= 0)
            return false;

        double cost = actual_quantity * seller.price[flower];

        // Execute trade
        buyer.demand[flower] -= actual_quantity;
        buyer.budget -= cost;
        seller.quantity[flower] -= actual_quantity;

        // Record trade
        TradeRecord record = {
            buyer.name,
            seller.name,
            flower,
            actual_quantity,
            seller.price[flower],
            cost,
            getCurrentTimestamp()};
        trade_history.push_back(record);

        // Print trade info
        std::cout << "💰 " << buyer.name << " bought " << actual_quantity << " "
                  << FlowerNames[flower] << "(s) from " << seller.name
                  << " for $" << std::fixed << std::setprecision(2) << cost
                  << " ($" << seller.price[flower] << " each)\n";

        return true;
    }

    void dropPrices()
    {
#pragma omp parallel for collapse(2)
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                if (sellers[i].price[flower] > 0.2)
                {
                    sellers[i].price[flower] = std::max(0.2, sellers[i].price[flower] - 0.2);
                }
            }
        }
    }

    void runMarket()
    {
        bool market_open = true;
        int round = 0;

        std::cout << "🌸 FLOWER MARKET OPENING 🌸\n";
        std::cout << "Market has " << sellers.size() << " sellers and " << buyers.size() << " buyers\n";

        while (market_open)
        {
            round++;
            std::cout << "\n--- ROUND " << round << " ---\n";

            bool any_trade = conductTradingRound();

            if (!any_trade)
            {
                std::cout << "📉 No trades this round. Dropping prices...\n";
                dropPrices();
            }

            if (round % 3 == 0)
            {
                printStatus();
            }

            if (allDemandsFulfilled())
            {
                std::cout << "✅ All buyers' demands fulfilled! Market closing.\n";
                market_open = false;
            }

            // Prevent infinite loops
            if (round > 50)
            {
                std::cout << "⏰ Market timeout after 50 rounds.\n";
                market_open = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        printFinalReport();
    }

    void printFinalReport()
    {
        std::cout << "\n"
                  << std::string(60, '=') << "\n";
        std::cout << "FINAL MARKET REPORT\n";
        std::cout << std::string(60, '=') << "\n";

        printStatus();

        std::cout << "\n📊 TRADE HISTORY:\n";
        double total_revenue = 0;
        for (const auto &trade : trade_history)
        {
            std::cout << "• " << trade.buyer_name << " ← " << trade.seller_name
                      << ": " << trade.quantity << " " << FlowerNames[trade.flower_type]
                      << " @ $" << std::fixed << std::setprecision(2) << trade.price_per_unit
                      << " = $" << trade.total_cost << "\n";
            total_revenue += trade.total_cost;
        }

        std::cout << "\n💰 Total Market Volume: $" << std::fixed << std::setprecision(2) << total_revenue << "\n";
        std::cout << "🏪 Total Trades: " << trade_history.size() << "\n";

        // Market efficiency analysis
        std::cout << "\n📈 MARKET EFFICIENCY ANALYSIS:\n";
        for (int i = 0; i < buyers.size(); ++i)
        {
            double fulfillment_rate = 0;
            int total_original_demand = 0;
            int total_fulfilled = 0;

            for (int flower = 0; flower < 3; ++flower)
            {
                total_original_demand += buyers[i].original_demand[flower];
                total_fulfilled += (buyers[i].original_demand[flower] - buyers[i].demand[flower]);
            }

            if (total_original_demand > 0)
            {
                fulfillment_rate = (double)total_fulfilled / total_original_demand * 100;
            }

            std::cout << "• " << buyers[i].name << ": " << std::fixed << std::setprecision(1)
                      << fulfillment_rate << "% demand fulfilled, $"
                      << std::setprecision(2) << (buyers[i].original_budget - buyers[i].budget)
                      << " spent\n";
        }
    }

    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Additional utility functions
    void printMarketSummary()
    {
        std::cout << "\n🌺 MARKET SUMMARY 🌺\n";
        std::cout << "Sellers: " << sellers.size() << "\n";
        std::cout << "Buyers: " << buyers.size() << "\n";
        std::cout << "Flower Types: " << 3 << " (Rose, Sunflower, Tulip)\n";

        // Calculate total initial supply and demand
        int total_supply[3] = {0, 0, 0};
        int total_demand[3] = {0, 0, 0};

        for (const auto &seller : sellers)
        {
            for (int i = 0; i < 3; ++i)
            {
                total_supply[i] += seller.original_quantity[i];
            }
        }

        for (const auto &buyer : buyers)
        {
            for (int i = 0; i < 3; ++i)
            {
                total_demand[i] += buyer.original_demand[i];
            }
        }

        std::cout << "\nInitial Supply vs Demand:\n";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "• " << FlowerNames[i] << ": Supply " << total_supply[i]
                      << " vs Demand " << total_demand[i];
            if (total_supply[i] < total_demand[i])
            {
                std::cout << " (SHORTAGE: " << (total_demand[i] - total_supply[i]) << ")";
            }
            else if (total_supply[i] > total_demand[i])
            {
                std::cout << " (SURPLUS: " << (total_supply[i] - total_demand[i]) << ")";
            }
            else
            {
                std::cout << " (BALANCED)";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }
};

int main()
{
    FlowerMarket market;

    // Initialize with hardcoded data
    market.initializeMarket();

    // Print initial market summary
    market.printMarketSummary();

    // Run the market simulation
    market.runMarket();

    return 0;
}
