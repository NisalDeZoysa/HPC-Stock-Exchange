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
#include <random>
#include <atomic>

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
    std::atomic<int> quantity[3];
    double price[3];
    std::string timestamp;
    int original_quantity[3];
    std::atomic<double> revenue;
    std::atomic<int> trades_count;
    omp_lock_t lock;

    Seller() : revenue(0.0), trades_count(0)
    {
        omp_init_lock(&lock);
    }

    ~Seller()
    {
        omp_destroy_lock(&lock);
    }

    // Copy constructor
    Seller(const Seller &other) : revenue(other.revenue.load()), trades_count(other.trades_count.load())
    {
        strcpy(name, other.name);
        for (int i = 0; i < 3; ++i)
        {
            quantity[i].store(other.quantity[i].load());
            price[i] = other.price[i];
            original_quantity[i] = other.original_quantity[i];
        }
        timestamp = other.timestamp;
        omp_init_lock(&lock);
    }

    // Assignment operator
    Seller &operator=(const Seller &other)
    {
        if (this != &other)
        {
            strcpy(name, other.name);
            for (int i = 0; i < 3; ++i)
            {
                quantity[i].store(other.quantity[i].load());
                price[i] = other.price[i];
                original_quantity[i] = other.original_quantity[i];
            }
            timestamp = other.timestamp;
            revenue.store(other.revenue.load());
            trades_count.store(other.trades_count.load());
        }
        return *this;
    }

    // Helper method to add revenue atomically
    void addRevenue(double amount)
    {
        double expected = revenue.load();
        while (!revenue.compare_exchange_weak(expected, expected + amount))
        {
            // Keep trying until successful
        }
    }
};

struct Buyer
{
    char name[20];
    std::atomic<int> demand[3];
    int original_demand[3];
    std::atomic<double> budget;
    double original_budget;
    double buy_price[3];
    std::string timestamp;
    int priority;
    std::atomic<double> spent;
    std::atomic<int> purchases_count;
    omp_lock_t lock;

    Buyer() : spent(0.0), purchases_count(0)
    {
        omp_init_lock(&lock);
    }

    ~Buyer()
    {
        omp_destroy_lock(&lock);
    }

    // Copy constructor
    Buyer(const Buyer &other) : spent(other.spent.load()), purchases_count(other.purchases_count.load())
    {
        strcpy(name, other.name);
        for (int i = 0; i < 3; ++i)
        {
            demand[i].store(other.demand[i].load());
            original_demand[i] = other.original_demand[i];
            buy_price[i] = other.buy_price[i];
        }
        budget.store(other.budget.load());
        original_budget = other.original_budget;
        timestamp = other.timestamp;
        priority = other.priority;
        omp_init_lock(&lock);
    }

    // Assignment operator
    Buyer &operator=(const Buyer &other)
    {
        if (this != &other)
        {
            strcpy(name, other.name);
            for (int i = 0; i < 3; ++i)
            {
                demand[i].store(other.demand[i].load());
                original_demand[i] = other.original_demand[i];
                buy_price[i] = other.buy_price[i];
            }
            budget.store(other.budget.load());
            original_budget = other.original_budget;
            timestamp = other.timestamp;
            priority = other.priority;
            spent.store(other.spent.load());
            purchases_count.store(other.purchases_count.load());
        }
        return *this;
    }

    // Helper method to subtract from budget atomically
    bool subtractBudget(double amount)
    {
        double expected = budget.load();
        while (expected >= amount)
        {
            if (budget.compare_exchange_weak(expected, expected - amount))
            {
                return true;
            }
        }
        return false;
    }

    // Helper method to add to spent atomically
    void addSpent(double amount)
    {
        double expected = spent.load();
        while (!spent.compare_exchange_weak(expected, expected + amount))
        {
            // Keep trying until successful
        }
    }
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
    int thread_id;
};

class FlowerMarket
{
private:
    std::vector<Seller> sellers;
    std::vector<Buyer> buyers;
    std::vector<TradeRecord> trade_history;
    std::mutex trade_mutex;
    std::mutex print_mutex;
    std::mutex history_mutex;
    std::atomic<int> total_trades;
    std::atomic<double> total_volume;

    // Parallel processing statistics
    std::atomic<int> parallel_operations;
    std::atomic<int> concurrent_trades;

    // Helper method to add to total volume atomically
    void addToTotalVolume(double amount)
    {
        double expected = total_volume.load();
        while (!total_volume.compare_exchange_weak(expected, expected + amount))
        {
            // Keep trying until successful
        }
    }

public:
    FlowerMarket() : total_trades(0), total_volume(0.0), parallel_operations(0), concurrent_trades(0) {}

    void initializeMarket()
    {
        std::cout << "🔧 Initializing market with " << omp_get_max_threads() << " threads available\n";

        // Initialize sellers
        sellers.resize(5);
        std::vector<std::string> seller_names = {"Alice", "Bob", "Charlie", "Diana", "Edward"};

#pragma omp parallel for
        for (int i = 0; i < sellers.size(); ++i)
        {
            strcpy(sellers[i].name, seller_names[i].c_str());

            // Random but balanced initial quantities
            std::random_device rd;
            std::mt19937 gen(rd() + i);
            std::uniform_int_distribution<> qty_dist(15, 40);
            std::uniform_real_distribution<> price_dist(4.0, 8.0);

            for (int j = 0; j < 3; ++j)
            {
                int qty = qty_dist(gen);
                sellers[i].quantity[j].store(qty);
                sellers[i].original_quantity[j] = qty;
                sellers[i].price[j] = price_dist(gen);
            }

            sellers[i].timestamp = getCurrentTimestamp();
            sellers[i].revenue.store(0.0);
            sellers[i].trades_count.store(0);
        }

        // Initialize buyers
        buyers.resize(8);
        std::vector<std::string> buyer_names = {"Dan", "Eve", "Fay", "Grace", "Henry", "Ivy", "Jack", "Kate"};

#pragma omp parallel for
        for (int i = 0; i < buyers.size(); ++i)
        {
            strcpy(buyers[i].name, buyer_names[i].c_str());

            std::random_device rd;
            std::mt19937 gen(rd() + i + 100);
            std::uniform_int_distribution<> demand_dist(5, 20);
            std::uniform_real_distribution<> budget_dist(200, 800);
            std::uniform_real_distribution<> price_dist(3.0, 7.0);
            std::uniform_int_distribution<> priority_dist(1, 5);

            for (int j = 0; j < 3; ++j)
            {
                int demand = demand_dist(gen);
                buyers[i].demand[j].store(demand);
                buyers[i].original_demand[j] = demand;
                buyers[i].buy_price[j] = price_dist(gen);
            }

            double budget = budget_dist(gen);
            buyers[i].budget.store(budget);
            buyers[i].original_budget = budget;
            buyers[i].priority = priority_dist(gen);
            buyers[i].timestamp = getCurrentTimestamp();
            buyers[i].spent.store(0.0);
            buyers[i].purchases_count.store(0);
        }
    }

    void printStatus()
    {
        std::lock_guard<std::mutex> lock(print_mutex);

        std::cout << "\n"
                  << std::string(70, '=') << "\n";
        std::cout << "CURRENT MARKET STATUS (Thread Analysis)\n";
        std::cout << "Parallel Operations: " << parallel_operations.load() << "\n";
        std::cout << "Concurrent Trades: " << concurrent_trades.load() << "\n";
        std::cout << std::string(70, '=') << "\n";

        std::cout << "\n📦 SELLER INVENTORY:\n";

        // Parallel aggregation of seller statistics
        std::vector<double> seller_revenues(sellers.size());
        std::vector<int> seller_trade_counts(sellers.size());

#pragma omp parallel for
        for (int i = 0; i < sellers.size(); ++i)
        {
            seller_revenues[i] = sellers[i].revenue.load();
            seller_trade_counts[i] = sellers[i].trades_count.load();
        }

        for (int i = 0; i < sellers.size(); ++i)
        {
            std::cout << "🏪 " << sellers[i].name << " (Revenue: $" << std::fixed
                      << std::setprecision(2) << seller_revenues[i]
                      << ", Trades: " << seller_trade_counts[i] << ")\n";

            for (int j = 0; j < 3; ++j)
            {
                std::cout << "   " << FlowerNames[j] << ": " << sellers[i].quantity[j].load()
                          << "/" << sellers[i].original_quantity[j]
                          << " @ $" << std::fixed << std::setprecision(2) << sellers[i].price[j] << "\n";
            }
        }

        std::cout << "\n🛒 BUYER DEMANDS:\n";

        // Parallel aggregation of buyer statistics
        std::vector<double> buyer_spent(buyers.size());
        std::vector<int> buyer_purchases(buyers.size());

#pragma omp parallel for
        for (int i = 0; i < buyers.size(); ++i)
        {
            buyer_spent[i] = buyers[i].spent.load();
            buyer_purchases[i] = buyers[i].purchases_count.load();
        }

        for (int i = 0; i < buyers.size(); ++i)
        {
            std::cout << "👤 " << buyers[i].name << " (Priority: " << buyers[i].priority
                      << ", Spent: $" << std::fixed << std::setprecision(2) << buyer_spent[i]
                      << ", Purchases: " << buyer_purchases[i] << ")\n";

            for (int j = 0; j < 3; ++j)
            {
                if (buyers[i].original_demand[j] > 0)
                {
                    std::cout << "   " << FlowerNames[j] << ": " << buyers[i].demand[j].load()
                              << "/" << buyers[i].original_demand[j]
                              << " (max $" << buyers[i].buy_price[j] << ")\n";
                }
            }
            std::cout << "   Budget: $" << std::fixed << std::setprecision(2)
                      << buyers[i].budget.load() << "/$" << buyers[i].original_budget << "\n";
        }
        std::cout << std::endl;
    }

    bool allDemandsFulfilled()
    {
        bool all_fulfilled = true;

#pragma omp parallel for reduction(&& : all_fulfilled)
        for (int i = 0; i < buyers.size(); ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (buyers[i].demand[j].load() > 0)
                {
                    all_fulfilled = false;
                }
            }
        }

        return all_fulfilled;
    }

    bool conductTradingRound()
    {
        std::atomic<bool> any_trade(false);
        parallel_operations.fetch_add(1);

        std::cout << "🔄 Conducting parallel trading round on " << omp_get_max_threads() << " threads\n";

        // Sort buyers by priority (higher priority first) - parallel sort
        std::vector<int> buyer_indices(buyers.size());
        std::iota(buyer_indices.begin(), buyer_indices.end(), 0);

// Use parallel sort for better performance
#pragma omp parallel
        {
#pragma omp single
            {
                std::sort(buyer_indices.begin(), buyer_indices.end(),
                          [this](int a, int b)
                          {
                              return buyers[a].priority > buyers[b].priority;
                          });
            }
        }

// Process each flower type in parallel
#pragma omp parallel for
        for (int flower = 0; flower < 3; ++flower)
        {
            // Create list of interested buyers for this flower type
            std::vector<int> interested_buyers;

            for (int buyer_idx : buyer_indices)
            {
                if (buyers[buyer_idx].demand[flower].load() > 0)
                {
                    interested_buyers.push_back(buyer_idx);
                }
            }

            if (interested_buyers.empty())
                continue;

            // Process sellers for this flower type
            for (int seller_idx = 0; seller_idx < sellers.size(); ++seller_idx)
            {
                if (sellers[seller_idx].quantity[flower].load() <= 0)
                    continue;

                // Find eligible buyers (can afford and want this flower)
                std::vector<int> eligible_buyers;
                for (int buyer_idx : interested_buyers)
                {
                    if (buyers[buyer_idx].buy_price[flower] >= sellers[seller_idx].price[flower] &&
                        buyers[buyer_idx].budget.load() >= sellers[seller_idx].price[flower])
                    {
                        eligible_buyers.push_back(buyer_idx);
                    }
                }

                if (eligible_buyers.empty())
                    continue;

// Parallel trade execution for eligible buyers
#pragma omp parallel for
                for (int i = 0; i < eligible_buyers.size(); ++i)
                {
                    int buyer_idx = eligible_buyers[i];

                    // Calculate trade quantity (limit to prevent overselling)
                    int max_quantity = std::min({buyers[buyer_idx].demand[flower].load(),
                                                 sellers[seller_idx].quantity[flower].load() / (int)eligible_buyers.size() + 1,
                                                 static_cast<int>(buyers[buyer_idx].budget.load() / sellers[seller_idx].price[flower])});

                    if (max_quantity > 0)
                    {
                        if (executeTrade(buyer_idx, seller_idx, flower, max_quantity))
                        {
                            any_trade = true;
                        }
                    }
                }
            }
        }

        return any_trade.load();
    }

    bool executeTrade(int buyer_idx, int seller_idx, int flower, int quantity)
    {
        // Use seller and buyer locks for thread safety
        omp_set_lock(&buyers[buyer_idx].lock);
        omp_set_lock(&sellers[seller_idx].lock);

        Buyer &buyer = buyers[buyer_idx];
        Seller &seller = sellers[seller_idx];

        // Double-check availability and affordability
        int buyer_demand = buyer.demand[flower].load();
        int seller_stock = seller.quantity[flower].load();
        double buyer_budget = buyer.budget.load();

        if (buyer_demand <= 0 || seller_stock <= 0 || quantity <= 0)
        {
            omp_unset_lock(&sellers[seller_idx].lock);
            omp_unset_lock(&buyers[buyer_idx].lock);
            return false;
        }

        // Calculate actual trade quantity
        int affordable = static_cast<int>(buyer_budget / seller.price[flower]);
        int actual_quantity = std::min({affordable, buyer_demand, seller_stock, quantity});

        if (actual_quantity <= 0)
        {
            omp_unset_lock(&sellers[seller_idx].lock);
            omp_unset_lock(&buyers[buyer_idx].lock);
            return false;
        }

        double cost = actual_quantity * seller.price[flower];

        // Execute atomic updates - using helper methods for doubles
        buyer.demand[flower].fetch_sub(actual_quantity);
        if (!buyer.subtractBudget(cost))
        {
            // Budget insufficient, rollback
            buyer.demand[flower].fetch_add(actual_quantity);
            omp_unset_lock(&sellers[seller_idx].lock);
            omp_unset_lock(&buyers[buyer_idx].lock);
            return false;
        }
        buyer.addSpent(cost);
        buyer.purchases_count.fetch_add(1);

        seller.quantity[flower].fetch_sub(actual_quantity);
        seller.addRevenue(cost);
        seller.trades_count.fetch_add(1);

        // Update global statistics
        total_trades.fetch_add(1);
        addToTotalVolume(cost);
        concurrent_trades.fetch_add(1);

        // Record trade
        TradeRecord record = {
            buyer.name,
            seller.name,
            flower,
            actual_quantity,
            seller.price[flower],
            cost,
            getCurrentTimestamp(),
            omp_get_thread_num()};

        {
            std::lock_guard<std::mutex> lock(history_mutex);
            trade_history.push_back(record);
        }

        // Print trade info with thread information
        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "💰 [T" << omp_get_thread_num() << "] " << buyer.name
                      << " bought " << actual_quantity << " " << FlowerNames[flower]
                      << "(s) from " << seller.name << " for $" << std::fixed
                      << std::setprecision(2) << cost << " ($" << seller.price[flower] << " each)\n";
        }

        omp_unset_lock(&sellers[seller_idx].lock);
        omp_unset_lock(&buyers[buyer_idx].lock);

        return true;
    }

    void dropPrices()
    {
        std::cout << "📉 Parallel price adjustment across all sellers...\n";

#pragma omp parallel for collapse(2)
        for (int i = 0; i < sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                if (sellers[i].price[flower] > 0.3)
                {
                    sellers[i].price[flower] = std::max(0.3, sellers[i].price[flower] - 0.25);
                }
            }
        }

        parallel_operations.fetch_add(1);
    }

    void runMarket()
    {
        bool market_open = true;
        int round = 0;

        std::cout << "🌸 PARALLEL FLOWER MARKET OPENING 🌸\n";
        std::cout << "Market has " << sellers.size() << " sellers and " << buyers.size() << " buyers\n";
        std::cout << "Running on " << omp_get_max_threads() << " threads\n";

        while (market_open)
        {
            round++;
            std::cout << "\n--- ROUND " << round << " ---\n";

            bool any_trade = conductTradingRound();

            if (!any_trade)
            {
                dropPrices();

                // Parallel market analysis
                analyzeMarketConditions();
            }

            if (round % 2 == 0)
            {
                printStatus();
            }

            if (allDemandsFulfilled())
            {
                std::cout << "✅ All buyers' demands fulfilled! Market closing.\n";
                market_open = false;
            }

            // Enhanced exit condition
            if (round > 30)
            {
                std::cout << "⏰ Market timeout after 30 rounds.\n";
                market_open = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        printFinalReport();
    }

    void analyzeMarketConditions()
    {
        std::cout << "🔍 Parallel market analysis...\n";

        // Parallel calculation of market metrics
        std::vector<double> avg_prices(3, 0.0);
        std::vector<int> total_supply(3, 0);
        std::vector<int> total_demand(3, 0);

#pragma omp parallel for
        for (int flower = 0; flower < 3; ++flower)
        {
            double price_sum = 0.0;
            int supply_sum = 0;
            int demand_sum = 0;
            int seller_count = 0;

            for (int i = 0; i < sellers.size(); ++i)
            {
                if (sellers[i].quantity[flower].load() > 0)
                {
                    price_sum += sellers[i].price[flower];
                    seller_count++;
                }
                supply_sum += sellers[i].quantity[flower].load();
            }

            for (int i = 0; i < buyers.size(); ++i)
            {
                demand_sum += buyers[i].demand[flower].load();
            }

            avg_prices[flower] = seller_count > 0 ? price_sum / seller_count : 0.0;
            total_supply[flower] = supply_sum;
            total_demand[flower] = demand_sum;
        }

        // Print analysis
        std::cout << "📊 Market Conditions:\n";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "   " << FlowerNames[i] << ": Avg Price $" << std::fixed
                      << std::setprecision(2) << avg_prices[i] << ", Supply " << total_supply[i]
                      << ", Demand " << total_demand[i] << "\n";
        }
    }

    void printFinalReport()
    {
        std::cout << "\n"
                  << std::string(70, '=') << "\n";
        std::cout << "FINAL PARALLEL MARKET REPORT\n";
        std::cout << std::string(70, '=') << "\n";

        printStatus();

        std::cout << "\n📊 PARALLEL PROCESSING STATISTICS:\n";
        std::cout << "Total Parallel Operations: " << parallel_operations.load() << "\n";
        std::cout << "Peak Concurrent Trades: " << concurrent_trades.load() << "\n";
        std::cout << "Threads Used: " << omp_get_max_threads() << "\n";

        std::cout << "\n💰 TRADE SUMMARY:\n";
        std::cout << "Total Trades: " << total_trades.load() << "\n";
        std::cout << "Total Market Volume: $" << std::fixed << std::setprecision(2) << total_volume.load() << "\n";

        // Parallel efficiency calculation
        std::vector<double> seller_efficiency(sellers.size());
        std::vector<double> buyer_efficiency(buyers.size());

#pragma omp parallel for
        for (int i = 0; i < sellers.size(); ++i)
        {
            int total_original = 0;
            int total_sold = 0;
            for (int j = 0; j < 3; ++j)
            {
                total_original += sellers[i].original_quantity[j];
                total_sold += (sellers[i].original_quantity[j] - sellers[i].quantity[j].load());
            }
            seller_efficiency[i] = total_original > 0 ? (double)total_sold / total_original * 100 : 0.0;
        }

#pragma omp parallel for
        for (int i = 0; i < buyers.size(); ++i)
        {
            int total_original = 0;
            int total_bought = 0;
            for (int j = 0; j < 3; ++j)
            {
                total_original += buyers[i].original_demand[j];
                total_bought += (buyers[i].original_demand[j] - buyers[i].demand[j].load());
            }
            buyer_efficiency[i] = total_original > 0 ? (double)total_bought / total_original * 100 : 0.0;
        }

        std::cout << "\n📈 PARALLEL EFFICIENCY ANALYSIS:\n";
        std::cout << "Seller Performance:\n";
        for (int i = 0; i < sellers.size(); ++i)
        {
            std::cout << "• " << sellers[i].name << ": " << std::fixed << std::setprecision(1)
                      << seller_efficiency[i] << "% sold, $" << std::setprecision(2)
                      << sellers[i].revenue.load() << " revenue\n";
        }

        std::cout << "\nBuyer Performance:\n";
        for (int i = 0; i < buyers.size(); ++i)
        {
            std::cout << "• " << buyers[i].name << ": " << std::fixed << std::setprecision(1)
                      << buyer_efficiency[i] << "% fulfilled, $" << std::setprecision(2)
                      << buyers[i].spent.load() << " spent\n";
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

    void printMarketSummary()
    {
        std::cout << "\n🌺 PARALLEL MARKET SUMMARY 🌺\n";
        std::cout << "OpenMP Threads: " << omp_get_max_threads() << "\n";
        std::cout << "Sellers: " << sellers.size() << "\n";
        std::cout << "Buyers: " << buyers.size() << "\n";
        std::cout << "Flower Types: 3 (Rose, Sunflower, Tulip)\n";

        // Parallel calculation of totals
        std::vector<int> total_supply(3, 0);
        std::vector<int> total_demand(3, 0);

#pragma omp parallel for
        for (int flower = 0; flower < 3; ++flower)
        {
            int supply = 0, demand = 0;

            for (int i = 0; i < sellers.size(); ++i)
            {
                supply += sellers[i].original_quantity[flower];
            }

            for (int i = 0; i < buyers.size(); ++i)
            {
                demand += buyers[i].original_demand[flower];
            }

            total_supply[flower] = supply;
            total_demand[flower] = demand;
        }

        std::cout << "\nInitial Supply vs Demand (calculated in parallel):\n";
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
    // Set OpenMP thread count
    omp_set_num_threads(std::min(8, omp_get_max_threads()));

    std::cout << "🚀 Starting Parallel Flower Market Exchange\n";
    std::cout << "Available CPU cores: " << omp_get_max_threads() << "\n";
    std::cout << "Using " << omp_get_num_threads() << " threads\n";

    FlowerMarket market;

    // Initialize with generated data
    market.initializeMarket();

    // Print initial market summary
    market.printMarketSummary();

    // Run the parallel market simulation
    market.runMarket();

    return 0;
}