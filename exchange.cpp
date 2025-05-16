#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <omp.h>
#include <algorithm>
#include <random>

struct Seller {
    std::string flower_type;
    int quantity;
    double price_per_flower;
    std::mutex lock;
};

struct Buyer {
    std::string flower_type;
    int requested_quantity;
    double max_price_per_flower;
    double spent = 0;
    bool fulfilled = false;
};

void process_order(Buyer& buyer, std::vector<Seller>& sellers) {
    for (auto& seller : sellers) {
        if (seller.flower_type == buyer.flower_type &&
            seller.price_per_flower <= buyer.max_price_per_flower) {
            
            std::lock_guard<std::mutex> guard(seller.lock);

            int qty_to_buy = std::min(buyer.requested_quantity, seller.quantity);
            if (qty_to_buy > 0) {
                buyer.spent = qty_to_buy * seller.price_per_flower;
                seller.quantity -= qty_to_buy;
                buyer.fulfilled = true;
                break;
            }
        }
    }
}

void adjust_prices(std::vector<Seller>& sellers, int inventory_threshold) {
    #pragma omp parallel for
    for (int i = 0; i < sellers.size(); i++) {
        std::lock_guard<std::mutex> guard(sellers[i].lock);
        if (sellers[i].quantity < inventory_threshold) {
            sellers[i].price_per_flower += 0.2;  // increase price
        } else {
            sellers[i].price_per_flower = std::max(0.5, sellers[i].price_per_flower - 0.1);  // discount
        }
    }
}

int main() {
    std::vector<Seller> sellers = {
        {"rose", 100, 2.5}, {"lily", 50, 3.0}, {"tulip", 75, 1.8}, {"rose", 40, 2.0}
    };

    std::vector<Buyer> buyers_template = {
        {"rose", 20, 3.0}, {"lily", 30, 3.2}, {"tulip", 50, 2.0},
        {"rose", 60, 2.3}, {"rose", 10, 2.6}, {"tulip", 30, 2.1}, {"lily", 20, 3.5}
    };

    int total_ticks = 3;
    double total_revenue = 0;
    double start_time = omp_get_wtime();

    for (int tick = 0; tick < total_ticks; tick++) {
        std::cout << "\n=== Market Tick " << tick + 1 << " ===\n";

        // Reset buyer states for each tick
        std::vector<Buyer> buyers = buyers_template;
        #pragma omp parallel for
        for (int i = 0; i < buyers.size(); i++) {
            process_order(buyers[i], sellers);
        }

        // Revenue calculation
        double tick_revenue = 0;
        #pragma omp parallel for reduction(+:tick_revenue)
        for (int i = 0; i < buyers.size(); i++) {
            tick_revenue += buyers[i].spent;
        }

        total_revenue += tick_revenue;
        std::cout << "Tick Revenue: $" << tick_revenue << "\n";

        adjust_prices(sellers, 30); // dynamic pricing

        // Display buyer results
        for (int i = 0; i < buyers.size(); i++) {
            std::cout << "Buyer " << i << ": ";
            if (buyers[i].fulfilled) {
                std::cout << "Fulfilled " << buyers[i].requested_quantity
                          << " " << buyers[i].flower_type << " for $"
                          << buyers[i].spent << "\n";
            } else {
                std::cout << "Not Fulfilled\n";
            }
        }
    }

    double end_time = omp_get_wtime();
    std::cout << "\n=== Final Seller Inventory ===\n";
    for (size_t i = 0; i < sellers.size(); i++) {
        std::cout << "Seller " << i << ": " << sellers[i].flower_type << " - "
                  << sellers[i].quantity << " left @ $" << sellers[i].price_per_flower << "\n";
    }

    std::cout << "\nTotal Revenue Over " << total_ticks << " Ticks: $" << total_revenue << "\n";
    std::cout << "Total Elapsed Time: " << (end_time - start_time) << " seconds\n";

    return 0;
}
