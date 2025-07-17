#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_ROUNDS 10
#define NUM_SELLERS 3
#define NUM_BUYERS 20
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

// Function to initialize sellers
void init_sellers(Seller sellers[])
{
    strcpy(sellers[0].name, "Alice");
    sellers[0].inventory[0] = 30;
    sellers[0].inventory[1] = 10;
    sellers[0].inventory[2] = 20;
    sellers[0].prices[0] = 2.0;
    sellers[0].prices[1] = 3.0;
    sellers[0].prices[2] = 4.0;

    strcpy(sellers[1].name, "Bob");
    sellers[1].inventory[0] = 20;
    sellers[1].inventory[1] = 20;
    sellers[1].inventory[2] = 10;
    sellers[1].prices[0] = 2.5;
    sellers[1].prices[1] = 2.8;
    sellers[1].prices[2] = 3.5;

    strcpy(sellers[2].name, "Charlie");
    sellers[2].inventory[0] = 10;
    sellers[2].inventory[1] = 5;
    sellers[2].inventory[2] = 10;
    sellers[2].prices[0] = 1.8;
    sellers[2].prices[1] = 2.5;
    sellers[2].prices[2] = 4.2;

    for (int i = 0; i < NUM_SELLERS; i++)
    {
        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            sellers[i].total_sold[j] = 0;
        }
        sellers[i].total_revenue = 0.0;
    }
}

// Function to initialize buyers
void init_buyers(Buyer buyers[])
{
    char buyer_names[NUM_BUYERS][MAX_NAME_LEN] = {
        "Dan", "Eva", "Frank", "Grace", "Henry", "Ivy", "Jack", "Kate",
        "Leo", "Mia", "Noah", "Olivia", "Paul", "Quinn", "Ruby", "Sam",
        "Tina", "Uma", "Victor", "Wendy"};

    srand(time(NULL));

    for (int i = 0; i < NUM_BUYERS; i++)
    {
        strcpy(buyers[i].name, buyer_names[i]);

        // Random desired quantities (1-10 for each flower type)
        buyers[i].desired[0] = (rand() % 10) + 1;
        buyers[i].desired[1] = (rand() % 8) + 1;
        buyers[i].desired[2] = (rand() % 6) + 1;

        // Random budget (200-800)
        buyers[i].budget = (rand() % 600) + 200;

        // Random max prices (2.0-6.0)
        buyers[i].max_prices[0] = 2.0 + (rand() % 400) / 100.0;
        buyers[i].max_prices[1] = 2.5 + (rand() % 350) / 100.0;
        buyers[i].max_prices[2] = 3.0 + (rand() % 300) / 100.0;

        // Initialize purchased and spent
        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            buyers[i].purchased[j] = 0;
        }
        buyers[i].spent = 0.0;
    }
}

// Function to adjust seller prices based on demand
void adjust_prices(Seller sellers[], int demand_info[][NUM_FLOWER_TYPES])
{
    for (int i = 0; i < NUM_SELLERS; i++)
    {
        for (int j = 0; j < NUM_FLOWER_TYPES; j++)
        {
            int total_demand = demand_info[i][j];
            int supply = sellers[i].inventory[j];

            if (total_demand > supply)
            {
                // High demand, increase price slightly
                sellers[i].prices[j] *= 1.05;
            }
            else if (total_demand < supply / 2)
            {
                // Low demand, decrease price
                sellers[i].prices[j] *= 0.90;
            }

            // Ensure minimum price
            if (sellers[i].prices[j] < 0.5)
            {
                sellers[i].prices[j] = 0.5;
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

        // Processes 1-4: Handle buyer groups
        if (rank >= 1 && rank <= 4)
        {
            int buyers_per_process = NUM_BUYERS / 4;
            int start_buyer = (rank - 1) * buyers_per_process;
            int end_buyer = (rank == 4) ? NUM_BUYERS : start_buyer + buyers_per_process;

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
            for (int p = 1; p <= 4; p++)
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

            // Adjust prices based on demand
            adjust_prices(sellers, total_demand);

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

        printf("\nTop 10 Buyer Performance:\n");
        for (int i = 0; i < 10; i++)
        {
            printf("%s: Spent: $%.2f, Bought: [%d, %d, %d], Remaining Budget: $%.2f\n",
                   buyers[i].name, buyers[i].spent,
                   buyers[i].purchased[0], buyers[i].purchased[1], buyers[i].purchased[2],
                   buyers[i].budget);
        }

        end_time = MPI_Wtime();
        printf("\nTotal execution time: %.4f seconds\n", end_time - start_time);
    }

    MPI_Finalize();
    return 0;
}