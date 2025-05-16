#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <omp.h>
#include <algorithm>

class Seller {
public:
    std::string flower_type;
    int quantity;
    double price_per_flower;
    std::mutex lock;

    Seller(std::string type, int qty, double price)
        : flower_type(type), quantity(qty), price_per_flower(price) {}

    bool sell(int requested_quantity, double max_price, double& spent) {
        std::lock_guard<std::mutex> guard(lock);
        if (price_per_flower > max_price || quantity <= 0) return false;

        int quantity_to_sell = std::min(quantity, requested_quantity);
        quantity -= quantity_to_sell;
        spent = quantity_to_sell * price_per_flower;
        return true;
    }

    void adjust_price(int inventory_threshold) {
        std::lock_guard<std::mutex> guard(lock);
        if (quantity < inventory_threshold) {
            price_per_flower += 0.2;
        } else {
            price_per_flower = std::max(0.5, price_per_flower - 0.1);
        }
    }

    void display(int id) const {
        std::cout << "Seller " << id << ": " << flower_type
                  << " | Quantity: " << quantity
                  << " | Price: $" << price_per_flower << "\n";
    }
};

class Buyer {
public:
    std::string flower_type;
    int requested_quantity;
    double max_price;
    double spent = 0;
    bool fulfilled = false;

    Buyer(std::string type, int qty, double price)
        : flower_type(type), requested_quantity(qty), max_price(price) {}

    void attempt_purchase(std::vector<Seller>& sellers) {
        for (auto& seller : sellers) {
            if (seller.flower_type == flower_type) {
                double tmp_spent = 0;
                if (seller.sell(requested_quantity, max_price, tmp_spent)) {
                    spent = tmp_spent;
                    fulfilled = true;
                    break;
                }
            }
        }
    }

    void display(int id) const {
        std::cout << "Buyer " << id << ": ";
        if (fulfilled) {
            std::cout << "Fulfilled, Spent $" << spent << "\n";
        } else {
            std::cout << "Not Fulfilled\n";
        }
    }
};

int main() {
    std::vector<Seller> sellers = {
        Seller("rose", 100, 2.5),
        Seller("lily", 50, 3.0),
        Seller("tulip", 75, 1.8),
        Seller("rose", 40, 2.0)
    };

    std::vector<Buyer> buyer_templates = {
        Buyer("rose", 20, 3.0),
        Buyer("lily", 30, 3.2),
        Buyer("tulip", 50, 2.0),
        Buyer("rose", 60, 2.3),
        Buyer("rose", 10, 2.6),
        Buyer("tulip", 30, 2.1),
        Buyer("lily", 20, 3.5)
    };

    int ticks = 3;
    double total_revenue = 0;
    double start = omp_get_wtime();

    for (int tick = 1; tick <= ticks; tick++) {
        std::cout << "\n=== Market Tick " << tick << " ===\n";
        std::vector<Buyer> buyers = buyer_templates;

        #pragma omp parallel for
        for (int i = 0; i < buyers.size(); i++) {
            buyers[i].attempt_purchase(sellers);
        }

        double tick_revenue = 0;
        #pragma omp parallel for reduction(+:tick_revenue)
        for (int i = 0; i < buyers.size(); i++) {
            tick_revenue += buyers[i].spent;
        }

        total_revenue += tick_revenue;
        std::cout << "Tick Revenue: $" << tick_revenue << "\n";

        #pragma omp parallel for
        for (int i = 0; i < sellers.size(); i++) {
            sellers[i].adjust_price(30);
        }

        for (int i = 0; i < buyers.size(); i++) {
            buyers[i].display(i);
        }
    }

    double end = omp_get_wtime();

    std::cout << "\n=== Final Seller Inventory ===\n";
    for (int i = 0; i < sellers.size(); i++) {
        sellers[i].display(i);
    }

    std::cout << "\nTotal Revenue: $" << total_revenue << "\n";
    std::cout << "Elapsed Time: " << (end - start) << " seconds\n";
    return 0;
}
