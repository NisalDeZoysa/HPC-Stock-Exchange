#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

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
        buyers[i].money = 50.0 + (rand() % 100); // $50-$149
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
            shops[i].prices[j] = 5.0 + (rand() % 15); // $5-$19
            shops[i].inventory[j] = 50 + (rand() % 50); // 50-99 flowers
        }
    }
}

void simulate_market_hybrid(Buyer *buyers, Shop *shops, int rank, int size) {
    // Calculate buyers per process
    int buyers_per_proc = NUM_BUYERS / size;
    int start_buyer = rank * buyers_per_proc;
    int end_buyer = (rank == size - 1) ? NUM_BUYERS : start_buyer + buyers_per_proc;
    
    for (int step = 0; step < SIMULATION_STEPS; step++) {
        // Process buyers in parallel using OpenMP within each MPI process
        #pragma omp parallel for schedule(static)
        for (int i = start_buyer; i < end_buyer; i++) {
            if (buyers[i].money > 10.0) { // Only shop if has enough money
                // Use deterministic random based on buyer id and step
                unsigned int seed = 42 + i * 1000 + step;
                int shop_id = rand_r(&seed) % NUM_SHOPS;
                seed = 42 + i * 1000 + step + 1;
                int flower_type = rand_r(&seed) % NUM_FLOWER_TYPES;
                
                buyers[i].shop_visits++;
                
                // Critical section for shop access
                #pragma omp critical
                {
                    if (shops[shop_id].inventory[flower_type] > 0 && 
                        buyers[i].money >= shops[shop_id].prices[flower_type]) {
                        
                        // Make purchase
                        buyers[i].money -= shops[shop_id].prices[flower_type];
                        buyers[i].flowers[flower_type]++;
                        buyers[i].total_purchases++;
                        shops[shop_id].inventory[flower_type]--;
                        shops[shop_id].sales_count++;
                    }
                }
            }
        }
        
        // Synchronize shop states across all processes
        for (int shop_idx = 0; shop_idx < NUM_SHOPS; shop_idx++) {
            // Reduce inventory changes
            for (int flower_idx = 0; flower_idx < NUM_FLOWER_TYPES; flower_idx++) {
                int local_inventory = shops[shop_idx].inventory[flower_idx];
                int global_inventory;
                MPI_Allreduce(&local_inventory, &global_inventory, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
                shops[shop_idx].inventory[flower_idx] = global_inventory;
            }
            
            // Reduce sales count
            int local_sales = shops[shop_idx].sales_count;
            int global_sales;
            MPI_Allreduce(&local_sales, &global_sales, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            shops[shop_idx].sales_count = global_sales;
        }
        
        // Restock shops periodically
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

void gather_all_buyers(Buyer *buyers, int rank, int size) {
    int buyers_per_proc = NUM_BUYERS / size;
    int start_buyer = rank * buyers_per_proc;
    int end_buyer = (rank == size - 1) ? NUM_BUYERS : start_buyer + buyers_per_proc;
    int local_count = end_buyer - start_buyer;
    
    // Create arrays for MPI communication
    double *all_money = malloc(NUM_BUYERS * sizeof(double));
    int *all_purchases = malloc(NUM_BUYERS * sizeof(int));
    int *all_visits = malloc(NUM_BUYERS * sizeof(int));
    int *all_flowers = malloc(NUM_BUYERS * NUM_FLOWER_TYPES * sizeof(int));
    
    // Prepare local data
    double *local_money = malloc(local_count * sizeof(double));
    int *local_purchases = malloc(local_count * sizeof(int));
    int *local_visits = malloc(local_count * sizeof(int));
    int *local_flowers = malloc(local_count * NUM_FLOWER_TYPES * sizeof(int));
    
    for (int i = 0; i < local_count; i++) {
        local_money[i] = buyers[start_buyer + i].money;
        local_purchases[i] = buyers[start_buyer + i].total_purchases;
        local_visits[i] = buyers[start_buyer + i].shop_visits;
        for (int j = 0; j < NUM_FLOWER_TYPES; j++) {
            local_flowers[i * NUM_FLOWER_TYPES + j] = buyers[start_buyer + i].flowers[j];
        }
    }
    
    // Calculate displacements for MPI_Allgatherv
    int *recvcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) {
        int proc_start = i * buyers_per_proc;
        int proc_end = (i == size - 1) ? NUM_BUYERS : proc_start + buyers_per_proc;
        recvcounts[i] = proc_end - proc_start;
        displs[i] = proc_start;
    }
    
    // Gather all buyer data
    MPI_Allgatherv(local_money, local_count, MPI_DOUBLE, 
                   all_money, recvcounts, displs, MPI_DOUBLE, MPI_COMM_WORLD);
    MPI_Allgatherv(local_purchases, local_count, MPI_INT, 
                   all_purchases, recvcounts, displs, MPI_INT, MPI_COMM_WORLD);
    MPI_Allgatherv(local_visits, local_count, MPI_INT, 
                   all_visits, recvcounts, displs, MPI_INT, MPI_COMM_WORLD);
    
    // Adjust recvcounts and displs for flowers array
    for (int i = 0; i < size; i++) {
        recvcounts[i] *= NUM_FLOWER_TYPES;
        displs[i] *= NUM_FLOWER_TYPES;
    }
    
    MPI_Allgatherv(local_flowers, local_count * NUM_FLOWER_TYPES, MPI_INT, 
                   all_flowers, recvcounts, displs, MPI_INT, MPI_COMM_WORLD);
    
    // Update all buyer data on all processes
    for (int i = 0; i < NUM_BUYERS; i++) {
        buyers[i].money = all_money[i];
        buyers[i].total_purchases = all_purchases[i];
        buyers[i].shop_visits = all_visits[i];
        for (int j = 0; j < NUM_FLOWER_TYPES; j++) {
            buyers[i].flowers[j] = all_flowers[i * NUM_FLOWER_TYPES + j];
        }
    }
    
    // Cleanup
    free(all_money);
    free(all_purchases);
    free(all_visits);
    free(all_flowers);
    free(local_money);
    free(local_purchases);
    free(local_visits);
    free(local_flowers);
    free(recvcounts);
    free(displs);
}

double compare_buyer_states(const char* serial_file, const char* hybrid_file) {
    FILE *serial = fopen(serial_file, "r");
    FILE *hybrid = fopen(hybrid_file, "r");
    
    if (!serial || !hybrid) {
        printf("Error opening comparison files\n");
        if (serial) fclose(serial);
        if (hybrid) fclose(hybrid);
        return 0.0;
    }
    
    int matches = 0, total = 0;
    char serial_line[256], hybrid_line[256];
    
    while (fgets(serial_line, sizeof(serial_line), serial) && 
           fgets(hybrid_line, sizeof(hybrid_line), hybrid)) {
        total++;
        if (strcmp(serial_line, hybrid_line) == 0) {
            matches++;
        }
    }
    
    fclose(serial);
    fclose(hybrid);
    
    return (total > 0) ? (double)matches / total * 100.0 : 0.0;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Ensure we have enough buyers for all processes
    if (NUM_BUYERS % size != 0 && rank == 0) {
        printf("Warning: NUM_BUYERS (%d) not evenly divisible by processes (%d)\n", 
               NUM_BUYERS, size);
    }
    
    srand(42 + rank); // Deterministic but different seed per process
    
    Buyer *buyers = malloc(NUM_BUYERS * sizeof(Buyer));
    Shop *shops = malloc(NUM_SHOPS * sizeof(Shop));
    
    double start_time = MPI_Wtime();
    
    // Initialize on all processes with same seed for consistency
    srand(42);
    init_buyers(buyers);
    init_shops(shops);
    
    // Run hybrid simulation
    simulate_market_hybrid(buyers, shops, rank, size);
    
    // Gather all buyer states to all processes
    gather_all_buyers(buyers, rank, size);
    
    double end_time = MPI_Wtime();
    
    if (rank == 0) {
        print_results(buyers, shops, "Hybrid MPI+OpenMP");
        printf("Execution time: %.4f seconds\n", end_time - start_time);
        printf("Number of MPI processes: %d\n", size);
        printf("Number of OpenMP threads per process: %d\n", omp_get_max_threads());
        
        save_buyer_states(buyers, "hybrid_results.csv");
        
        // Compare with serial results if available
        double accuracy = compare_buyer_states("serial_results.csv", "hybrid_results.csv");
        printf("Accuracy compared to serial version: %.2f%%\n", accuracy);
    }
    
    free(buyers);
    free(shops);
    
    MPI_Finalize();
    return 0;
}