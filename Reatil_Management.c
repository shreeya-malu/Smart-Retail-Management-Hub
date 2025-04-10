#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 100
#define MAX_COUNTERS 4
#define MAX_CART_ITEMS 10

//color codes using ANSI escape sequences
#define RESET   "\x1b[0m"    // Reset to default color
#define RED     "\x1b[31m"   // Red
#define GREEN   "\x1b[32m"   // Green
#define YELLOW  "\x1b[33m"   // Yellow
#define BLUE    "\x1b[34m"   // Blue
#define MAGENTA "\x1b[35m"   // Magenta
#define CYAN    "\x1b[36m"   // Cyan

//for bold and underline
#define BOLD    "\x1b[1m"    // Bold text
#define UNDERLINE "\x1b[4m"  // Underlined text
 
// Hash Table for Inventory
typedef struct {
    char productName[50];
    int quantity;
    char location[50];
    int isOccupied; // Flag to indicate if the slot is used
} HashTableEntry;

// Cart Item
typedef struct {
    char productName[50];
    int quantity;
} CartItem;

// Queue Node
typedef struct QueueNode {
    int customerID;
    CartItem cart[MAX_CART_ITEMS];
    int cartSize;
    int checkedOut; // New field to track checkout status
    struct QueueNode* next;
} QueueNode;

// Queue Structure
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

// Global Variables
HashTableEntry hashTable[HASH_TABLE_SIZE];
Queue counters[MAX_COUNTERS];

// Function Prototypes
int hashFunction(char* productName);
void addProduct(char* name, int quantity, char* location);
HashTableEntry* searchProduct(char* name);
void displayInventory();
void decrementStock(char* productName, int quantity);
void initializeQueues();
void enqueueCustomer(int counter, int customerID);
void addToCart(QueueNode* customer, char* productName, int quantity);
void billCustomer(int counter);
void dequeueCustomer(int counter);
void findBestCounter();
void displayQueueStatus();
int checkLocationConflict(char* location);
void managerMenu();
void customerMenu();

// Hash function
int hashFunction(char* productName) {
    int hash = 0;
    while (*productName)
        hash += *productName++;
    return hash % HASH_TABLE_SIZE;
}

// Add product to inventory
void addProduct(char* name, int quantity, char* location) {
    if (checkLocationConflict(location)) {
        printf(RED BOLD"\n\t\t\t\t\t\tError: Another product already exists at location %s.\n"RESET, location);
        return;
    }

    int index = hashFunction(name);
    while (hashTable[index].isOccupied) {
        index = (index + 1) % HASH_TABLE_SIZE;  // Linear probing
    }
    strcpy(hashTable[index].productName, name);
    hashTable[index].quantity = quantity;
    strcpy(hashTable[index].location, location);
    hashTable[index].isOccupied = 1;
    printf(CYAN BOLD"\n\t\t\t\t\t\tProduct %s added to inventory.\n"RESET, name);
}

// Check if location is already occupied by another product
int checkLocationConflict(char* location) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (hashTable[i].isOccupied && strcmp(hashTable[i].location, location) == 0) {
            return 1;  // Location conflict
        }
    }
    return 0;  // No conflict
}

// Search product in inventory
HashTableEntry* searchProduct(char* name) {
    int index = hashFunction(name);  // Get the hash index for the product name
    int originalIndex = index;  // Remember the original index to detect full circle (in case of collision)

    while (hashTable[index].isOccupied) {
        // Ensure case-insensitive comparison
        if (strcasecmp(hashTable[index].productName, name) == 0) {
            // Product found, return the corresponding entry
            return &hashTable[index];
        }

        index = (index + 1) % HASH_TABLE_SIZE;  // Linear probing
        if (index == originalIndex) {
            // We have looped through all possible slots
            break;
        }
    }

    return NULL;  // Product not found
}

// Display inventory
void displayInventory() {
    printf(GREEN"\t\t\t\t\t\tInventory:\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (hashTable[i].isOccupied) {
            printf("\t\t\t\t\t\tProduct: %s, Quantity: %d, Location: %s\n",
                   hashTable[i].productName, hashTable[i].quantity, hashTable[i].location);
            if (hashTable[i].quantity == 0)
                printf("\t\t\t\t\t\tProduct %s is out of stock! Restocking needed.\n"RESET, hashTable[i].productName);
        }
    }
}

// Decrement stock
void decrementStock(char* productName, int quantity) {
    HashTableEntry* product = searchProduct(productName);
    if (product) {
        if (product->quantity >= quantity) {
            product->quantity -= quantity;
            if (product->quantity == 0)
                printf(RED BOLD"\t\t\t\t\t\tProduct %s is now out of stock!\n", productName);
        } else {
            printf("\t\t\t\t\t\tNot enough stock for %s. Available: %d\n", product->productName, product->quantity);
        }
    } else {
        printf("\t\t\t\t\t\tProduct %s not found in inventory!\n"RESET, productName);
    }
}

// Initialize cashier queues
void initializeQueues() {
    for (int i = 0; i < MAX_COUNTERS; i++) {
        counters[i].front = counters[i].rear = NULL;
        counters[i].size = 0;
    }
}

// Enqueue customer to a counter
void enqueueCustomer(int counter, int customerID) {
    if (counter < 0 || counter >= MAX_COUNTERS) {
        printf(RED BOLD"\t\t\t\t\t\tInvalid counter number!\n"RESET);
        return;
    }

    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->customerID = customerID;
    newNode->cartSize = 0;
    newNode->checkedOut = 0;  // Initially not checked out
    newNode->next = NULL;

    if (counters[counter].rear == NULL) {
        counters[counter].front = counters[counter].rear = newNode;
    } else {
        counters[counter].rear->next = newNode;
        counters[counter].rear = newNode;
    }
    counters[counter].size++;
}

// Add item to cart
void addToCart(QueueNode* customer, char* productName, int quantity) {
    if (customer->cartSize >= MAX_CART_ITEMS) {
        printf(RED BOLD"\t\t\t\t\t\tCart is full! Cannot add more items.\n"RESET);
        return;
    }

    HashTableEntry* product = searchProduct(productName);
    if (product) {
        if (product->quantity >= quantity) {
            strcpy(customer->cart[customer->cartSize].productName, productName);
            customer->cart[customer->cartSize].quantity = quantity;
            customer->cartSize++;
            printf(CYAN BOLD"\t\t\t\t\t\tAdded %d of %s to cart.\n"RESET, quantity , productName);
        } else {
            printf(RED BOLD"\t\t\t\t\t\tNot enough stock for %s. Available: %d\n", product->productName, product->quantity);
            printf("\t\t\t\t\t\tPlease enter a smaller quantity or press 0 to cancel: "RESET);
            int newQuantity;
            scanf("%d", &newQuantity);
            if (newQuantity > 0 && newQuantity <= product->quantity) {
                strcpy(customer->cart[customer->cartSize].productName, productName);
                customer->cart[customer->cartSize].quantity = newQuantity;
                customer->cartSize++;
                printf(CYAN BOLD"\t\t\t\t\t\tAdded %d of %s to cart.\n"RESET, newQuantity , productName);
            }
        }
    } else {
        printf(RED BOLD"\t\t\t\t\t\tProduct not found in inventory.\n"RESET);
    }
}

// Find best counter (least number of customers)
void findBestCounter() {
    int minCounter = 0;
    for (int i = 1; i < MAX_COUNTERS; i++) {
        if (counters[i].size < counters[minCounter].size)
            minCounter = i;
    }
    printf(CYAN BOLD"\t\t\t\t\t\tThe best counter is Counter %d with %d customers.\n"RESET, minCounter + 1, counters[minCounter].size); // Adjusted to display from 1
}

// Bill customer and dequeue
void billCustomer(int counter) {
    if (counter < 0 || counter >= MAX_COUNTERS) {
        printf(RED BOLD"\t\t\t\t\t\tInvalid counter number!\n");
        return;
    }

    if (counters[counter].front == NULL) {
        printf("\t\t\t\t\t\tCounter %d is empty!\n"RESET, counter + 1); // Adjusted to display from 1
        return;
    }

    QueueNode* customer = counters[counter].front;
    printf(CYAN BOLD"\t\t\t\t\t\tBilling for customer %d:\n"RESET, customer->customerID);

    for (int i = 0; i < customer->cartSize; i++) {
        char* productName = customer->cart[i].productName;
        int quantity = customer->cart[i].quantity;
        decrementStock(productName, quantity);
        printf(CYAN BOLD"\t\t\t\t\t\t%s x %d billed.\n"RESET, productName, quantity);
    }

    // Mark customer as checked out
    customer->checkedOut = 1;

    // Dequeue the customer
    counters[counter].front = counters[counter].front->next;
    if (counters[counter].front == NULL)
        counters[counter].rear = NULL;

    free(customer);
    counters[counter].size--;
}

// Display queue status and customers who haven't checked out
void displayQueueStatus() {
    for (int i = 0; i < MAX_COUNTERS; i++) {
        printf(BLUE BOLD"\t\t\t\t\t\tCounter %d: %d customers in queue\n", i + 1, counters[i].size); // Adjusted to display from 1
        printf("\t\t\t\t\t\tCustomers at Counter %d who haven't checked out:\n", i + 1);
       
        QueueNode* temp = counters[i].front;
        while (temp != NULL) {
            if (!temp->checkedOut) {
                printf("\t\t\t\t\t\tCustomer %d is waiting with cart.\n"RESET, temp->customerID);
               
            }
            temp = temp->next;
        } 
    }
     printf("\n\t\t\t\t\t\t-------------------------------------");
}

// Manager Mode
void managerMenu() {
    int choice;
    char password[50];
    printf(BOLD"\n\t\t\t\t\t\tEnter password to access manager mode: ");
    scanf("\t\t\t\t\t\t\t%s", password);
    if (strcmp(password, "admin") == 0) {
        do {
            printf(GREEN"\n\n\t\t\t\t\t\t*********SELECT THE OPERATION*********\n");
            printf("\n\n\t\t\t\t\t\t1. Display Inventory\n");
            printf("\n\t\t\t\t\t\t2. Cashier - View Customers\n");
            printf("\n\t\t\t\t\t\t3. Exit Manager Mode\n");
            printf( "\n\n\t\t\t\t\t\t************************************\n" RESET);
            printf(RED BOLD"\n\t\t\t\t\t\tEnter your choice: "RESET);
            scanf("\n\t\t\t\t\t\t\t%d", &choice);

            switch (choice) {
                case 1:
                    displayInventory();
                    break;
                case 2:
                    displayQueueStatus();
                    break;
                case 3:
                    printf(RED BOLD"\n\t\t\t\t\t\tExiting Manager Mode...\n"RESET);
                    break;
                default:
                    printf(RED BOLD"Invalid choice! Please try again.\n"RESET);
            }
        } while (choice != 3);
    } else {
        printf(RED BOLD"\n\t\t\t\t\t\tIncorrect password! Access denied.\n"RESET);
    }
}

// Customer Menu
void customerMenu() {
    int customerID;
    printf(YELLOW"\t\t\t\t\t\t--------------------------------------"RESET);
    printf(YELLOW "\n \t\t\t\t\t\tEnter your Customer ID: "RESET);
    scanf("\t\t\t\t\t\t\t\t\t%d", &customerID);
    printf(YELLOW"\t\t\t\t\t\t--------------------------------------"RESET);

    int choice;
    QueueNode* customer = NULL;
    do {
        printf(MAGENTA"\n\n\t\t\t\t\t\t*********SELECT THE OPERATION*********\n");
        printf("\n\n \t\t\t\t\t\t\t1. Search Inventory\n");
        printf("\n\t\t\t\t\t\t\t2. Add to Cart\n");
        printf("\n\t\t\t\t\t\t\t3. Best Counter\n");
        printf("\n\t\t\t\t\t\t\t4. Checkout\n");
        printf("\n\t\t\t\t\t\t\t5. Exit Customer Mode\n");
        printf("\n\t\t\t\t\t\t************************************\n" RESET);
        printf(RED BOLD"\t\t\t\t\t\t\tEnter your choice: "RESET);
        scanf("\t\t\t\t\t\t\t%d", &choice);
        

        switch (choice) {
            case 1: {
             char productName[50];
    printf(BLUE BOLD"\n\t\t\t\t\t\tEnter product name to search: ");
    scanf("\t\t\t\t\t\t\t%s", productName);
    HashTableEntry* product = searchProduct(productName);
    if (product) {
        printf(CYAN BOLD"\t\t\t\t\t\tProduct found: %s, Quantity: %d, Location: %s\n"RESET, 
               product->productName, product->quantity, product->location);
    } else {
        printf(CYAN BOLD"\t\t\t\t\t\tProduct not found.\n"RESET);
    }
    break;
            }
            case 2: {
                char productName[50];
                int quantity;
                printf(BLUE BOLD"\n\t\t\t\t\t\tEnter product name to add to cart: "RESET);
                scanf("\t\t\t\t\t\t\t%s", productName);
                printf(BLUE BOLD"\n\t\t\t\t\t\t Enter quantity: "RESET);
                scanf("\t\t\t\t\t\t %d", &quantity);
                if (customer == NULL) {
                    int counter;
                    printf(BLUE"\n\t\t\t\t\t\tChoose a counter (1-4): "RESET);
                    scanf("%d", &counter);
                    counter--; // Adjust for 0-indexing
                    enqueueCustomer(counter, customerID);
                    customer = counters[counter].rear;
                }
                addToCart(customer, productName, quantity);
                break;
            }
            case 3:
                findBestCounter();
                break;
            case 4:
                printf(RED BOLD"\t\t\t\t\t\tProceeding to checkout...\n"RESET);
                for (int i = 0; i < MAX_COUNTERS; i++) {
                    if (counters[i].front && counters[i].front->customerID == customerID) {
                        billCustomer(i);
                        break;
                    }
                }
                break;
            case 5:
                printf(RED"\t\t\t\t\t\tExiting Customer Mode...\n"RESET);
                break;
            default:
                printf(RED"Invalid choice! Please try again.\n"RESET);
        }
    } while (choice != 5);
}

// Main menu
int main() {
        addProduct("laptop", 10, "A1");
        addProduct("phone", 20, "A2");
        addProduct("tablet", 15, "A3");
        addProduct("milk", 10, "B1");
        addProduct("bread", 20, "B2");
        addProduct("cheese", 15, "A3");
        addProduct("biscuits", 10, "C1");
        addProduct("coldrinks", 20, "C2");
        addProduct("oil", 15, "C3");
        addProduct("broom", 10, "D1");
        addProduct("bucket", 20, "D2");
        addProduct("utensils", 15, "D3");

    int mainChoice;
    initializeQueues();  // Initialize counters
    do {
        printf(GREEN "\n\t\t\t\t\t\t*********WELCOME TO DMart!*********\n\n" );
        printf("\n\t\t\t\t\t\t\t 1. Customer Mode\n");
        printf("\n\t\t\t\t\t\t\t 2. Manager Mode\n");
        printf("\n\t\t\t\t\t\t\t 3. Exit" RESET);
        printf(GREEN "\n\n\t\t\t\t\t\t************************************\n" RESET);
        printf( RED BOLD"\n\t\t\t\t\t\t Enter your choice: "RESET);
        scanf("\n\t\t\t\t\t\t\t\t\t\t\t\t%d", &mainChoice);

        switch (mainChoice) {
            case 1:
                customerMenu();
                break;
            case 2:
                managerMenu();
                break;
            case 3:
                printf(RED BOLD"\t\t\t\t\t\tExiting program.\n"RESET);
                break;
            default:
                printf(RED BOLD"Invalid choice! Please try again.\n"RESET);
        }
    } while (mainChoice != 3);

    return 0;
}
