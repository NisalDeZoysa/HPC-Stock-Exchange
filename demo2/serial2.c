#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NUM_BUYERS 1000
#define NUM_SHOPS 10
#define NUM_FLOWER_TYPES 5
#define SIMULATION_STEPS 100

typedef struct {
    int id;
    double money;
    int flowers[NUM_FLOWER_TYPES];
    int total_purchases;
    int shop_visits;
} Buyer;

typedef struct {
    int id;
    double prices[NUM_FLOWER_TYPES];
    int inventory[NUM_FLOWER_TYPES];
    int sales_count;
} Shop;

void init_buyers(Buyer *buyers) {
    for (int i = 0; i < NUM_BUYERS; i++) {
        buyers[i].id = i;
        buyers[i].money = 50.0 + (rand() % 100);
        buyers[i].total_purchases = 0;
        buyers[i].shop_visits = 0;
        for (int j = 0; j < NUM_FLOWER_TYPES; j++) {
            buyers[i].flowers[j] = 0;
        }
    }
}

void init_shops(Shop *shops) {
    for (int i = 0; i < NUM_SHOPS; i++) {
        shops[i].id = i;
        shops[i].sales_count = 0;
        for (int j = 0; j < NUM_FLOWER_TYPES; j++) {
            shops[i].prices[j] = 5.0 + (rand() % 15);
            shops[i].inventory[j] = 50 + (rand() % 50);
        }
    }
}

void simulate_market(Buyer *buyers, Shop *shops) {
    for (int step = 0; step < SIMULATION_STEPS; step++) {
        for (int i = 0; i < NUM_BUYERS; i++) {
            if (buyers[i].money > 10.0) {
                int shop_id = rand() % NUM_SHOPS;
                int flower_type = rand() % NUM_FLOWER_TYPES;
                
                buyers[i].shop_visits++;
                
                if (shops[shop_id].inventory[flower_type] > 0 && 
                    buyers[i].money >= shops[shop_id].prices[flower_type]) {
                    
                    buyers[i].money -= shops[shop_id].prices[flower_type];
                    buyers[i].flowers[flower_type]++;
                    buyers[i].total_purchases++;
                    shops[shop_id].inventory[flower_type]--;
                    shops[shop_id].sales_count++;
                }
            }
        }
        
        if (step % 20 == 0) {
            for (int i = 0; i < NUM_SHOPS; i++) {
                for (int j = 0; j < NUM_FLOWER_TYPES; j++) {
                    shops[i].inventory[j] += 10;
                }
            }
        }
    }
}

void print_results(Buyer *buyers, Shop *shops, const char* version) {
    printf("\n=== %s Results ===\n", version);
    
    double total_money = 0, total_purchases = 0, total_visits = 0;
    for (int i = 0; i < NUM_BUYERS; i++) {
        total_money += buyers[i].money;
        total_purchases += buyers[i].total_purchases;
        total_visits += buyers[i].shop_visits;
    }
    
    printf("Average buyer money: $%.2f\n", total_money / NUM_BUYERS);
    printf("Average purchases per buyer: %.2f\n", total_purchases / NUM_BUYERS);
    printf("Average shop visits per buyer: %.2f\n", total_visits / NUM_BUYERS);
    
    int total_sales = 0;
    for (int i = 0; i < NUM_SHOPS; i++) {
        total_sales += shops[i].sales_count;
    }
    printf("Total market sales: %d\n", total_sales);
}

void save_buyer_states(Buyer *buyers, const char* filename) {
    FILE *file = fopen(filename, "w");
    if (file) {
        for (int i = 0; i < NUM_BUYERS; i++) {
            fprintf(file, "%d,%.2f,%d,%d\n", buyers[i].id, buyers[i].money, 
                   buyers[i].total_purchases, buyers[i].shop_visits);
        }
        fclose(file);
    }
}

int main() {
    srand(42);
    
    Buyer *buyers = malloc(NUM_BUYERS * sizeof(Buyer));
    Shop *shops = malloc(NUM_SHOPS * sizeof(Shop));
    
    clock_t start = clock();
    
    init_buyers(buyers);
    init_shops(shops);
    simulate_market(buyers, shops);
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    print_results(buyers, shops, "Serial");
    printf("Execution time: %.4f seconds\n", time_taken);
    
    save_buyer_states(buyers, "serial_results.csv");
    
    free(buyers);
    free(shops);
    
    return 0;
}