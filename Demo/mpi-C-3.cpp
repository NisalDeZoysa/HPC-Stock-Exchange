#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define MAX_ROUNDS 10
#define NUM_SELLERS 3
#define NUM_BUYERS 23 // Changed to match serial version
#define NUM_FLOWER_TYPES 3
#define MAX_NAME_LEN 20

typedef struct
{
    char name[MAX_NAME_LEN];
    int inventory[NUM_FLOWER_TYPES];
    double prices[NUM_FLOWER_TYPES];
    int total_sold[NUM_FLOWER_TYPES];
    double total_revenue;
} Seller;

typedef struct
{
    char name[MAX_NAME_LEN];
    int desired[NUM_FLOWER_TYPES];
    double budget;
    double max_prices[NUM_FLOWER_TYPES];
    int purchased[NUM_FLOWER_TYPES];
    double spent;
} Buyer;

typedef struct
{
    int seller_id;
    int flower_type;
    int quantity;
    double price;
    int buyer_id;
} Transaction;

// Function to initialize sellers - using same data as serial version
void init_sellers(Seller sellers[])
{
    strcpy(sellers[0].name, "Alice");
    sellers[0].inventory[0] = 30;
    sellers[0].inventory[1] = 10;
    sellers[0].inventory[2] = 20;
    sellers[0].prices[0] = 6.0; // Changed to match serial version
    sellers[0].prices[1] = 5.5; // Changed to match serial version
    sellers[0].prices[2] = 7.0; // Changed to match serial version

    strcpy(sellers[1].name, "Bob");
    sellers[1].inventory[0] = 20;
    sellers[1].inventory[1] = 20;
    sellers[1].inventory[2] = 10;
    sellers[1].prices[0] = 5.5; // Changed to match serial version
    sellers[1].prices[1] = 5.2; // Changed to match serial version
    sellers[1].prices[2] = 6.5; // Changed to match serial version

    strcpy(sellers[2].name, "Charlie");
    sellers[2].inventory[0] = 10;
    sellers[2].inventory[1] = 5;
    sellers[2].inventory[2] = 10;
    sellers[2].prices[0] = 6.8; // Changed to match serial version
    sellers[2].prices[1] = 5.0; // Changed to match serial version
    sellers[2].prices[2] = 7.5; // Changed to match serial version

    for (int i = 0; i < NUM_SELLERS; i++)
    {
        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            sellers[i].total_sold[j] = 0;
        }
        sellers[i].total_revenue = 0.0;
    }
}

// Function to initialize buyers - using same data as serial version
void init_buyers(Buyer buyers[])
{
    // Buyer data matching serial version exactly
    char buyer_names[NUM_BUYERS][MAX_NAME_LEN] = {
        "Dan", "Eve", "Fay", "Ben", "Lia", "Joe", "Sue", "Amy", "Tim", "Sam",
        "Jill", "Zoe", "Max", "Ivy", "Leo", "Kim", "Tom", "Nina", "Ray", "Liv",
        "Oli", "Ken", "Ana"};

    int desired_flowers[NUM_BUYERS][NUM_FLOWER_TYPES] = {
        {10, 5, 2}, {5, 5, 0}, {15, 10, 5}, {10, 0, 5}, {2, 2, 2}, {5, 10, 5}, {5, 5, 5}, {1, 1, 1}, {4, 6, 3}, {7, 8, 4}, {3, 4, 5}, {6, 3, 7}, {5, 5, 5}, {8, 6, 4}, {9, 0, 2}, {3, 3, 3}, {6, 5, 3}, {4, 2, 6}, {3, 5, 4}, {5, 3, 2}, {6, 6, 6}, {2, 2, 2}, {7, 7, 1}};

    double budgets[NUM_BUYERS] = {
        500, 300, 1000, 350, 100, 400, 200, 50, 250, 600,
        200, 300, 250, 550, 350, 180, 400, 280, 300, 250,
        450, 100, 370};

    double max_prices_data[NUM_BUYERS][NUM_FLOWER_TYPES] = {
        {4.0, 4.0, 5.0}, {3.5, 3.5, 0.0}, {5.0, 4.5, 5.5}, {4.5, 0.0, 5.0}, {4.0, 4.0, 4.0}, {5.0, 5.0, 5.0}, {4.5, 4.5, 4.5}, {3.0, 3.0, 3.0}, {4.5, 4.5, 5.0}, {5.0, 5.0, 5.0}, {4.0, 4.5, 5.0}, {4.0, 5.0, 5.5}, {4.5, 4.5, 4.5}, {5.0, 5.0, 5.0}, {4.2, 0.0, 5.0}, {4.0, 4.0, 4.0}, {4.8, 4.8, 5.0}, {4.0, 4.0, 5.0}, {4.5, 4.5, 4.5}, {4.0, 4.0, 4.5}, {5.0, 5.0, 5.0}, {3.5, 3.5, 3.5}, {4.5, 4.5, 4.5}};

    for (int i = 0; i < NUM_BUYERS; i++)
    {
        strcpy(buyers[i].name, buyer_names[i]);

        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            buyers[i].desired[j] = desired_flowers[i][j];
            buyers[i].max_prices[j] = max_prices_data[i][j];
            buyers[i].purchased[j] = 0;
        }

        buyers[i].budget = budgets[i];
        buyers[i].spent = 0.0;
    }
}

// Function to adjust seller prices - simple market logic: prices always decrease
void adjust_prices(Seller sellers[])
{
    for (int i = 0; i < NUM_SELLERS; i++)
    {
        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            // Simple market: prices decrease each round
            sellers[i].prices[j] *= 0.95; // 5% decrease each round

            // Ensure minimum price
            if (sellers[i].prices[j] < 0.1)
            {
                sellers[i].prices[j] = 0.1;
            }
        }
    }
}

// Function to process transactions
int process_transactions(Seller sellers[], Buyer buyers[], Transaction transactions[], int num_transactions)
{
    int successful_transactions = 0;

    for (int i = 0; i < num_transactions; i++)
    {
        Transaction *t = &transactions[i];
        Seller *seller = &sellers[t->seller_id];
        Buyer *buyer = &buyers[t->buyer_id];

        // Check if seller has inventory and buyer can afford
        if (seller->inventory[t->flower_type] >= t->quantity &&
            buyer->budget >= t->price * t->quantity)
        {

            // Execute transaction
            seller->inventory[t->flower_type] -= t->quantity;
            seller->total_sold[t->flower_type] += t->quantity;
            seller->total_revenue += t->price * t->quantity;

            buyer->purchased[t->flower_type] += t->quantity;
            buyer->spent += t->price * t->quantity;
            buyer->budget -= t->price * t->quantity;

            successful_transactions++;
        }
    }

    return successful_transactions;
}

int main(int argc, char *argv[])
{
    int rank, size;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 4 || size > 5)
    {
        if (rank == 0)
        {
            printf("This program requires 4-5 MPI processes\n");
        }
        MPI_Finalize();
        return 1;
    }

    start_time = MPI_Wtime();

    Seller sellers[NUM_SELLERS];
    Buyer buyers[NUM_BUYERS];
    Transaction transactions[100];
    int demand_info[NUM_SELLERS][NUM_FLOWER_TYPES];

    // Initialize data on all processes
    init_sellers(sellers);
    init_buyers(buyers);

    if (rank == 0)
    {
        printf("=== FLOWER MARKET SIMULATION ===\n");
        printf("Sellers: %d, Buyers: %d, Rounds: %d\n", NUM_SELLERS, NUM_BUYERS, MAX_ROUNDS);
        printf("MPI Processes: %d\n\n", size);
    }

    // Main simulation loop
    for (int round = 0; round < MAX_ROUNDS; round++)
    {
        if (rank == 0)
        {
            printf("--- Round %d ---\n", round + 1);
        }

        // Reset demand info
        memset(demand_info, 0, sizeof(demand_info));

        // Process 0: Coordinate and manage sellers
        if (rank == 0)
        {
            printf("Current Seller Prices:\n");
            for (int i = 0; i < NUM_SELLERS; i++)
            {
                printf("%s: [%.2f, %.2f, %.2f] Inventory: [%d, %d, %d]\n",
                       sellers[i].name,
                       sellers[i].prices[0], sellers[i].prices[1], sellers[i].prices[2],
                       sellers[i].inventory[0], sellers[i].inventory[1], sellers[i].inventory[2]);
            }

            // Broadcast current prices to all processes
            MPI_Bcast(sellers, NUM_SELLERS * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);
        }
        else
        {
            // Receive current prices
            MPI_Bcast(sellers, NUM_SELLERS * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);
        }

        // Processes 1-(size-1): Handle buyer groups
        if (rank >= 1 && rank < size)
        {
            int num_buyer_processes = size - 1;
            int buyers_per_process = NUM_BUYERS / num_buyer_processes;
            int start_buyer = (rank - 1) * buyers_per_process;
            int end_buyer = (rank == num_buyer_processes) ? NUM_BUYERS : start_buyer + buyers_per_process;

            int local_transactions = 0;
            Transaction local_trans[50];

            // Each buyer process generates transactions
            for (int b = start_buyer; b < end_buyer; b++)
            {
                for (int seller_id = 0; seller_id < NUM_SELLERS; seller_id++)
                {
                    for (int flower_type = 0; flower_type < NUM_FLOWER_TYPES; flower_type++)
                    {
                        // Check if buyer wants this flower and can afford it
                        if (buyers[b].desired[flower_type] > buyers[b].purchased[flower_type] &&
                            sellers[seller_id].prices[flower_type] <= buyers[b].max_prices[flower_type] &&
                            buyers[b].budget >= sellers[seller_id].prices[flower_type])
                        {

                            int quantity = 1; // Buy one at a time

                            // Create transaction
                            local_trans[local_transactions].seller_id = seller_id;
                            local_trans[local_transactions].flower_type = flower_type;
                            local_trans[local_transactions].quantity = quantity;
                            local_trans[local_transactions].price = sellers[seller_id].prices[flower_type];
                            local_trans[local_transactions].buyer_id = b;
                            local_transactions++;

                            // Update demand info
                            demand_info[seller_id][flower_type] += quantity;

                            if (local_transactions >= 50)
                                break;
                        }
                    }
                    if (local_transactions >= 50)
                        break;
                }
                if (local_transactions >= 50)
                    break;
            }

            // Send transactions to process 0
            MPI_Send(&local_transactions, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            if (local_transactions > 0)
            {
                MPI_Send(local_trans, local_transactions * sizeof(Transaction), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
            }

            // Send demand info to process 0
            MPI_Send(demand_info, NUM_SELLERS * NUM_FLOWER_TYPES, MPI_INT, 0, 2, MPI_COMM_WORLD);
        }

        // Process 0: Collect and process all transactions
        if (rank == 0)
        {
            int total_transactions = 0;
            Transaction all_transactions[200];
            int total_demand[NUM_SELLERS][NUM_FLOWER_TYPES];
            memset(total_demand, 0, sizeof(total_demand));

            // Collect transactions from all buyer processes
            for (int p = 1; p < size; p++)
            {
                int local_count;
                MPI_Recv(&local_count, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (local_count > 0)
                {
                    MPI_Recv(&all_transactions[total_transactions], local_count * sizeof(Transaction),
                             MPI_BYTE, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    total_transactions += local_count;
                }

                // Collect demand info
                int proc_demand[NUM_SELLERS][NUM_FLOWER_TYPES];
                MPI_Recv(proc_demand, NUM_SELLERS * NUM_FLOWER_TYPES, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int i = 0; i < NUM_SELLERS; i++)
                {
                    for (int j = 0; j < NUM_FLOWER_TYPES; j++)
                    {
                        total_demand[i][j] += proc_demand[i][j];
                    }
                }
            }

            // Process transactions
            int successful = process_transactions(sellers, buyers, all_transactions, total_transactions);
            printf("Transactions attempted: %d, Successful: %d\n", total_transactions, successful);

            // Adjust prices - simple decrease each round
            adjust_prices(sellers);

            // Broadcast updated sellers and buyers data
            MPI_Bcast(sellers, NUM_SELLERS * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);
            MPI_Bcast(buyers, NUM_BUYERS * sizeof(Buyer), MPI_BYTE, 0, MPI_COMM_WORLD);
        }
        else
        {
            // Receive updated data
            MPI_Bcast(sellers, NUM_SELLERS * sizeof(Seller), MPI_BYTE, 0, MPI_COMM_WORLD);
            MPI_Bcast(buyers, NUM_BUYERS * sizeof(Buyer), MPI_BYTE, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        // Add 500ms delay after each round
        if (rank == 0)
        {
            usleep(500000); // 500ms = 500,000 microseconds
        }
        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0)
        {
            printf("\n");
        }
    }

    // Final results
    if (rank == 0)
    {
        printf("=== FINAL RESULTS ===\n");
        printf("\nSeller Performance:\n");
        for (int i = 0; i < NUM_SELLERS; i++)
        {
            printf("%s: Revenue: $%.2f, Sold: [%d, %d, %d], Remaining: [%d, %d, %d]\n",
                   sellers[i].name, sellers[i].total_revenue,
                   sellers[i].total_sold[0], sellers[i].total_sold[1], sellers[i].total_sold[2],
                   sellers[i].inventory[0], sellers[i].inventory[1], sellers[i].inventory[2]);
        }

        printf("\nBuyer Performance:\n");
        for (int i = 0; i < NUM_BUYERS; i++)
        {
            int still_needed[NUM_FLOWER_TYPES];
            for (int j = 0; j < NUM_FLOWER_TYPES; j++)
            {
                still_needed[j] = buyers[i].desired[j] - buyers[i].purchased[j];
                if (still_needed[j] < 0)
                    still_needed[j] = 0;
            }

            printf("%s: Budget: $%.2f, Spent: $%.2f, Purchased: [%d, %d, %d], Still needed: [%d, %d, %d]\n",
                   buyers[i].name, buyers[i].budget, buyers[i].spent,
                   buyers[i].purchased[0], buyers[i].purchased[1], buyers[i].purchased[2],
                   still_needed[0], still_needed[1], still_needed[2]);
        }

        end_time = MPI_Wtime();
        printf("\nTotal execution time: %.4f seconds\n", end_time - start_time);
    }

    MPI_Finalize();
    return 0;
}