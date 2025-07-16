#include <iostream>
#include <vector>
#include <omp.h>
#include <mpi.h>
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
    int original_quantity[3];
    std::atomic<double> revenue;
    std::atomic<int> trades_count;
    omp_lock_t lock;
    int process_id; // Which MPI process owns this seller

    Seller() : revenue(0.0), trades_count(0), process_id(0)
    {
        omp_init_lock(&lock);
    }

    ~Seller()
    {
        omp_destroy_lock(&lock);
    }

    Seller(const Seller &other) : revenue(other.revenue.load()), trades_count(other.trades_count.load()), process_id(other.process_id)
    {
        strcpy(name, other.name);
        for (int i = 0; i < 3; ++i)
        {
            quantity[i].store(other.quantity[i].load());
            price[i] = other.price[i];
            original_quantity[i] = other.original_quantity[i];
        }
        omp_init_lock(&lock);
    }

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
            revenue.store(other.revenue.load());
            trades_count.store(other.trades_count.load());
            process_id = other.process_id;
        }
        return *this;
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
    int priority;
    std::atomic<double> spent;
    std::atomic<int> purchases_count;
    omp_lock_t lock;
    int process_id; // Which MPI process owns this buyer

    Buyer() : spent(0.0), purchases_count(0), process_id(0)
    {
        omp_init_lock(&lock);
    }

    ~Buyer()
    {
        omp_destroy_lock(&lock);
    }

    Buyer(const Buyer &other) : spent(other.spent.load()), purchases_count(other.purchases_count.load()), process_id(other.process_id)
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
        priority = other.priority;
        omp_init_lock(&lock);
    }

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
            priority = other.priority;
            spent.store(other.spent.load());
            purchases_count.store(other.purchases_count.load());
            process_id = other.process_id;
        }
        return *this;
    }
};

struct TradeRecord
{
    char buyer_name[20];
    char seller_name[20];
    int flower_type;
    int quantity;
    double price_per_unit;
    double total_cost;
    int thread_id;
    int process_id;
};

// MPI message structures
struct MarketUpdate
{
    int seller_quantities[3];
    double seller_prices[3];
    int buyer_demands[3];
    double buyer_budget;
    int process_id;
};

class HybridFlowerMarket
{
private:
    std::vector<Seller> local_sellers;
    std::vector<Buyer> local_buyers;
    std::vector<Seller> global_sellers; // All sellers across all processes
    std::vector<Buyer> global_buyers;   // All buyers across all processes
    std::vector<TradeRecord> trade_history;
    std::mutex trade_mutex;
    std::mutex print_mutex;
    std::atomic<int> total_trades;
    std::atomic<double> total_volume;

    // MPI variables
    int mpi_rank;
    int mpi_size;

public:
    HybridFlowerMarket() : total_trades(0), total_volume(0.0), mpi_rank(0), mpi_size(1) {}

    void initializeMPI(int argc, char **argv)
    {
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        if (mpi_rank == 0)
        {
            std::cout << "Hybrid MPI+OpenMP Flower Market\n";
            std::cout << "MPI Processes: " << mpi_size << "\n";
            std::cout << "OpenMP Threads per process: " << omp_get_max_threads() << "\n";
        }
    }

    void initializeMarket()
    {
        // Each process gets a subset of sellers and buyers
        int sellers_per_process = 5 / mpi_size + (mpi_rank < (5 % mpi_size) ? 1 : 0);
        int buyers_per_process = 8 / mpi_size + (mpi_rank < (8 % mpi_size) ? 1 : 0);

        local_sellers.resize(sellers_per_process);
        local_buyers.resize(buyers_per_process);

        // Initialize local sellers with OpenMP
        std::vector<std::string> seller_names = {"Alice", "Bob", "Charlie", "Diana", "Edward"};

#pragma omp parallel for
        for (int i = 0; i < sellers_per_process; ++i)
        {
            int global_seller_id = mpi_rank * (5 / mpi_size) + i;
            if (global_seller_id < seller_names.size())
            {
                strcpy(local_sellers[i].name, seller_names[global_seller_id].c_str());
                local_sellers[i].process_id = mpi_rank;

                std::random_device rd;
                std::mt19937 gen(rd() + global_seller_id);
                std::uniform_int_distribution<> qty_dist(15, 40);
                std::uniform_real_distribution<> price_dist(4.0, 8.0);

                for (int j = 0; j < 3; ++j)
                {
                    int qty = qty_dist(gen);
                    local_sellers[i].quantity[j].store(qty);
                    local_sellers[i].original_quantity[j] = qty;
                    local_sellers[i].price[j] = price_dist(gen);
                }

                local_sellers[i].revenue.store(0.0);
                local_sellers[i].trades_count.store(0);
            }
        }

        // Initialize local buyers with OpenMP
        std::vector<std::string> buyer_names = {"Dan", "Eve", "Fay", "Grace", "Henry", "Ivy", "Jack", "Kate"};

#pragma omp parallel for
        for (int i = 0; i < buyers_per_process; ++i)
        {
            int global_buyer_id = mpi_rank * (8 / mpi_size) + i;
            if (global_buyer_id < buyer_names.size())
            {
                strcpy(local_buyers[i].name, buyer_names[global_buyer_id].c_str());
                local_buyers[i].process_id = mpi_rank;

                std::random_device rd;
                std::mt19937 gen(rd() + global_buyer_id + 100);
                std::uniform_int_distribution<> demand_dist(5, 20);
                std::uniform_real_distribution<> budget_dist(200, 800);
                std::uniform_real_distribution<> price_dist(3.0, 7.0);
                std::uniform_int_distribution<> priority_dist(1, 5);

                for (int j = 0; j < 3; ++j)
                {
                    int demand = demand_dist(gen);
                    local_buyers[i].demand[j].store(demand);
                    local_buyers[i].original_demand[j] = demand;
                    local_buyers[i].buy_price[j] = price_dist(gen);
                }

                double budget = budget_dist(gen);
                local_buyers[i].budget.store(budget);
                local_buyers[i].original_budget = budget;
                local_buyers[i].priority = priority_dist(gen);
                local_buyers[i].spent.store(0.0);
                local_buyers[i].purchases_count.store(0);
            }
        }

        // Synchronize all processes
        MPI_Barrier(MPI_COMM_WORLD);

        if (mpi_rank == 0)
        {
            std::cout << "Market initialized across " << mpi_size << " processes\n";
            std::cout << "Local sellers: " << local_sellers.size() << ", Local buyers: " << local_buyers.size() << "\n";
        }
    }

    void shareMarketData()
    {
        // Gather all market data from all processes
        global_sellers.clear();
        global_buyers.clear();

        // Collect seller data
        for (int proc = 0; proc < mpi_size; ++proc)
        {
            if (proc == mpi_rank)
            {
                // Send local sellers to all other processes
                for (const auto &seller : local_sellers)
                {
                    // Create a simple data structure to send
                    MarketUpdate update;
                    for (int i = 0; i < 3; ++i)
                    {
                        update.seller_quantities[i] = seller.quantity[i].load();
                        update.seller_prices[i] = seller.price[i];
                    }
                    update.process_id = mpi_rank;

                    MPI_Bcast(&update, sizeof(MarketUpdate), MPI_BYTE, proc, MPI_COMM_WORLD);

                    // Also broadcast seller name
                    char name_buf[20];
                    strcpy(name_buf, seller.name);
                    MPI_Bcast(name_buf, 20, MPI_CHAR, proc, MPI_COMM_WORLD);
                }
            }
            else
            {
                // Receive sellers from process 'proc'
                int sellers_from_proc = (proc < (5 % mpi_size)) ? (5 / mpi_size + 1) : (5 / mpi_size);

                for (int i = 0; i < sellers_from_proc; ++i)
                {
                    MarketUpdate update;
                    MPI_Bcast(&update, sizeof(MarketUpdate), MPI_BYTE, proc, MPI_COMM_WORLD);

                    char name_buf[20];
                    MPI_Bcast(name_buf, 20, MPI_CHAR, proc, MPI_COMM_WORLD);

                    // Create seller object
                    Seller remote_seller;
                    strcpy(remote_seller.name, name_buf);
                    remote_seller.process_id = proc;
                    for (int j = 0; j < 3; ++j)
                    {
                        remote_seller.quantity[j].store(update.seller_quantities[j]);
                        remote_seller.price[j] = update.seller_prices[j];
                    }

                    global_sellers.push_back(remote_seller);
                }
            }
        }

        // Add local sellers to global list
        for (const auto &seller : local_sellers)
        {
            global_sellers.push_back(seller);
        }

        // Similar process for buyers (simplified)
        for (const auto &buyer : local_buyers)
        {
            global_buyers.push_back(buyer);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    void printStatus()
    {
        if (mpi_rank == 0)
        {
            std::lock_guard<std::mutex> lock(print_mutex);

            std::cout << "\n"
                      << std::string(70, '=') << "\n";
            std::cout << "HYBRID MARKET STATUS (MPI Rank " << mpi_rank << ")\n";
            std::cout << "Processes: " << mpi_size << ", Threads: " << omp_get_max_threads() << "\n";
            std::cout << std::string(70, '=') << "\n";

            std::cout << "\nLOCAL SELLERS:\n";
            for (const auto &seller : local_sellers)
            {
                std::cout << " " << seller.name << " (Process " << seller.process_id
                          << ", Revenue: $" << std::fixed << std::setprecision(2) << seller.revenue.load() << ")\n";

                for (int j = 0; j < 3; ++j)
                {
                    std::cout << "   " << FlowerNames[j] << ": " << seller.quantity[j].load()
                              << "/" << seller.original_quantity[j]
                              << " @ $" << seller.price[j] << "\n";
                }
            }

            std::cout << "\nLOCAL BUYERS:\n";
            for (const auto &buyer : local_buyers)
            {
                std::cout << " " << buyer.name << " (Process " << buyer.process_id
                          << ", Priority: " << buyer.priority << ")\n";

                for (int j = 0; j < 3; ++j)
                {
                    if (buyer.original_demand[j] > 0)
                    {
                        std::cout << "   " << FlowerNames[j] << ": " << buyer.demand[j].load()
                                  << "/" << buyer.original_demand[j] << "\n";
                    }
                }
                std::cout << "   Budget: $" << std::fixed << std::setprecision(2)
                          << buyer.budget.load() << "/$" << buyer.original_budget << "\n";
            }
        }
    }

    bool conductTradingRound()
    {
        std::atomic<bool> any_trade(false);

        // Share market data across all processes
        shareMarketData();

        if (mpi_rank == 0)
        {
            std::cout << "Conducting hybrid trading round...\n";
        }

// Each process handles its local buyers trying to buy from any seller
#pragma omp parallel for
        for (int buyer_idx = 0; buyer_idx < local_buyers.size(); ++buyer_idx)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                if (local_buyers[buyer_idx].demand[flower].load() <= 0)
                    continue;

                // Try to buy from any seller (local or remote)
                for (int seller_idx = 0; seller_idx < global_sellers.size(); ++seller_idx)
                {
                    if (global_sellers[seller_idx].quantity[flower].load() <= 0)
                        continue;

                    // Check if buyer can afford this seller's price
                    if (local_buyers[buyer_idx].buy_price[flower] >= global_sellers[seller_idx].price[flower] &&
                        local_buyers[buyer_idx].budget.load() >= global_sellers[seller_idx].price[flower])
                    {

                        // If seller is remote, we need MPI communication
                        if (global_sellers[seller_idx].process_id != mpi_rank)
                        {
                            // Send trade request to remote process
                            if (requestRemoteTrade(buyer_idx, seller_idx, flower))
                            {
                                any_trade = true;
                                break;
                            }
                        }
                        else
                        {
                            // Local trade
                            if (executeLocalTrade(buyer_idx, seller_idx, flower))
                            {
                                any_trade = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Synchronize all processes
        MPI_Barrier(MPI_COMM_WORLD);

        // Reduce any_trade across all processes
        bool global_any_trade = false;
        MPI_Allreduce(&any_trade, &global_any_trade, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);

        return global_any_trade;
    }

    bool requestRemoteTrade(int buyer_idx, int seller_idx, int flower)
    {
        // Simplified remote trade request
        // In a real implementation, you would send detailed trade requests
        // For now, we'll just simulate some remote communication

        int target_process = global_sellers[seller_idx].process_id;

        // Send trade request (simplified)
        struct TradeRequest
        {
            int flower_type;
            int quantity;
            double max_price;
            int buyer_process;
        } request;

        request.flower_type = flower;
        request.quantity = std::min(local_buyers[buyer_idx].demand[flower].load(), 5);
        request.max_price = local_buyers[buyer_idx].buy_price[flower];
        request.buyer_process = mpi_rank;

        // For simplicity, we'll just return false (no remote trades in this basic version)
        return false;
    }

    bool executeLocalTrade(int buyer_idx, int seller_idx, int flower)
    {
        // Find the corresponding local seller
        int local_seller_idx = -1;
        for (int i = 0; i < local_sellers.size(); ++i)
        {
            if (strcmp(local_sellers[i].name, global_sellers[seller_idx].name) == 0)
            {
                local_seller_idx = i;
                break;
            }
        }

        if (local_seller_idx == -1)
            return false;

        // Execute trade with thread safety
        omp_set_lock(&local_buyers[buyer_idx].lock);
        omp_set_lock(&local_sellers[local_seller_idx].lock);

        int buyer_demand = local_buyers[buyer_idx].demand[flower].load();
        int seller_stock = local_sellers[local_seller_idx].quantity[flower].load();
        double buyer_budget = local_buyers[buyer_idx].budget.load();

        if (buyer_demand <= 0 || seller_stock <= 0)
        {
            omp_unset_lock(&local_sellers[local_seller_idx].lock);
            omp_unset_lock(&local_buyers[buyer_idx].lock);
            return false;
        }

        int affordable = static_cast<int>(buyer_budget / local_sellers[local_seller_idx].price[flower]);
        int actual_quantity = std::min({affordable, buyer_demand, seller_stock, 3});

        if (actual_quantity <= 0)
        {
            omp_unset_lock(&local_sellers[local_seller_idx].lock);
            omp_unset_lock(&local_buyers[buyer_idx].lock);
            return false;
        }

        double cost = actual_quantity * local_sellers[local_seller_idx].price[flower];

        // Update buyer
        local_buyers[buyer_idx].demand[flower].fetch_sub(actual_quantity);
        local_buyers[buyer_idx].budget.fetch_sub(cost);
        local_buyers[buyer_idx].spent.fetch_add(cost);
        local_buyers[buyer_idx].purchases_count.fetch_add(1);

        // Update seller
        local_sellers[local_seller_idx].quantity[flower].fetch_sub(actual_quantity);
        local_sellers[local_seller_idx].revenue.fetch_add(cost);
        local_sellers[local_seller_idx].trades_count.fetch_add(1);

        // Update global statistics
        total_trades.fetch_add(1);
        total_volume.fetch_add(cost);

        // Record trade
        TradeRecord record;
        strcpy(record.buyer_name, local_buyers[buyer_idx].name);
        strcpy(record.seller_name, local_sellers[local_seller_idx].name);
        record.flower_type = flower;
        record.quantity = actual_quantity;
        record.price_per_unit = local_sellers[local_seller_idx].price[flower];
        record.total_cost = cost;
        record.thread_id = omp_get_thread_num();
        record.process_id = mpi_rank;

        trade_history.push_back(record);

        // Print trade info
        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "[P" << mpi_rank << ":T" << omp_get_thread_num() << "] "
                      << local_buyers[buyer_idx].name << " bought " << actual_quantity
                      << " " << FlowerNames[flower] << "(s) from " << local_sellers[local_seller_idx].name
                      << " for $" << std::fixed << std::setprecision(2) << cost << "\n";
        }

        omp_unset_lock(&local_sellers[local_seller_idx].lock);
        omp_unset_lock(&local_buyers[buyer_idx].lock);

        return true;
    }

    void dropPrices()
    {
        if (mpi_rank == 0)
        {
            std::cout << "Hybrid price adjustment across all processes...\n";
        }

// Each process adjusts its local sellers' prices
#pragma omp parallel for collapse(2)
        for (int i = 0; i < local_sellers.size(); ++i)
        {
            for (int flower = 0; flower < 3; ++flower)
            {
                if (local_sellers[i].price[flower] > 0.3)
                {
                    local_sellers[i].price[flower] = std::max(0.3, local_sellers[i].price[flower] - 0.25);
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    bool allDemandsFulfilled()
    {
        bool local_fulfilled = true;

#pragma omp parallel for reduction(&& : local_fulfilled)
        for (int i = 0; i < local_buyers.size(); ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (local_buyers[i].demand[j].load() > 0)
                {
                    local_fulfilled = false;
                }
            }
        }

        // Check across all processes
        bool global_fulfilled = true;
        MPI_Allreduce(&local_fulfilled, &global_fulfilled, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);

        return global_fulfilled;
    }

    void runMarket()
    {
        bool market_open = true;
        int round = 0;

        if (mpi_rank == 0)
        {
            std::cout << "\nHYBRID FLOWER MARKET OPENING\n";
            std::cout << "Running on " << mpi_size << " MPI processes\n";
            std::cout << "Each process using " << omp_get_max_threads() << " OpenMP threads\n";
        }

        while (market_open && round < 15)
        {
            round++;

            if (mpi_rank == 0)
            {
                std::cout << "\n--- ROUND " << round << " ---\n";
            }

            bool any_trade = conductTradingRound();

            if (!any_trade)
            {
                dropPrices();
            }

            if (round % 3 == 0)
            {
                printStatus();
            }

            if (allDemandsFulfilled())
            {
                if (mpi_rank == 0)
                {
                    std::cout << "All demands fulfilled! Market closing.\n";
                }
                market_open = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        printFinalReport();
    }

    void printFinalReport()
    {
        if (mpi_rank == 0)
        {
            std::cout << "\n"
                      << std::string(70, '=') << "\n";
            std::cout << "FINAL HYBRID MARKET REPORT\n";
            std::cout << std::string(70, '=') << "\n";

            std::cout << "MPI Processes: " << mpi_size << "\n";
            std::cout << "OpenMP Threads per process: " << omp_get_max_threads() << "\n";
            std::cout << "Total Trades: " << total_trades.load() << "\n";
            std::cout << "Total Volume: $" << std::fixed << std::setprecision(2) << total_volume.load() << "\n";
        }

        // Each process reports its local statistics
        MPI_Barrier(MPI_COMM_WORLD);

        for (int proc = 0; proc < mpi_size; ++proc)
        {
            if (proc == mpi_rank)
            {
                std::cout << "\nProcess " << mpi_rank << " Local Results:\n";
                std::cout << "Local Sellers: " << local_sellers.size() << "\n";
                std::cout << "Local Buyers: " << local_buyers.size() << "\n";
                std::cout << "Local Trades: " << trade_history.size() << "\n";

                double local_revenue = 0.0;
#pragma omp parallel for reduction(+ : local_revenue)
                for (int i = 0; i < local_sellers.size(); ++i)
                {
                    local_revenue += local_sellers[i].revenue.load();
                }

                std::cout << "Local Revenue: $" << std::fixed << std::setprecision(2) << local_revenue << "\n";
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    void finalizeMPI()
    {
        MPI_Finalize();
    }
};

int main(int argc, char **argv)
{
    // Set OpenMP thread count
    omp_set_num_threads(std::min(4, omp_get_max_threads()));

    HybridFlowerMarket market;

    // Initialize MPI
    market.initializeMPI(argc, argv);

    // Initialize market
    market.initializeMarket();

    // Run the hybrid market simulation
    market.runMarket();

    // Finalize MPI
    market.finalizeMPI();

    return 0;
}