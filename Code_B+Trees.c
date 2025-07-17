#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_KEYS 3  
#define MAX_PARKING_SPACES 50  

// Structure for parking spaces
typedef struct {
    int spaceID;
    int isOccupied;
    int totalUsageCount;
    int totalRevenue;
} ParkingSpace;

ParkingSpace parkingSpaces[MAX_PARKING_SPACES];

// Structure for a vehicle
typedef struct {
    char vehicleNumber[15];
    char ownerName[50];
    time_t arrivalTime;
    time_t departureTime;
    double totalHours;
    int totalCount;   // New field to track total entries
    int membership;
    int parkingSpace;
    int amountPaid;
    int isParked; // 1 if parked, 0 if exited
} Vehicle;

// B+ Tree Node
typedef struct BPTreeNode {
    Vehicle *keys[MAX_KEYS];
    struct BPTreeNode *children[MAX_KEYS + 1];
    int numKeys;
    int isLeaf;
    struct BPTreeNode *next;
} BPTreeNode;

BPTreeNode *root = NULL;  
#define VEHICLE_FILE "vehicles.txt"
#define PARKING_FILE "parking_spaces.txt"

void initializeParkingSpaces() {
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        parkingSpaces[i].spaceID = i + 1;
        parkingSpaces[i].isOccupied = 0;
        parkingSpaces[i].totalUsageCount = 0;
        parkingSpaces[i].totalRevenue = 0;
    }
}

// Collects all vehicles from the B+ Tree into an array
void collectVehicles(BPTreeNode *node, Vehicle **vehicles, int *count) {
    if (!node) return;

    // Add all vehicles from the current node
    for (int i = 0; i < node->numKeys; i++) {
        vehicles[(*count)++] = node->keys[i];
    }

    // If not a leaf, continue collecting from child nodes
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            collectVehicles(node->children[i], vehicles, count);
        }
    }
}

// Function to copy all vehicles from the B+ Tree
Vehicle **copyVehicles(int *count) {
    Vehicle **vehicleArray = (Vehicle **)malloc(100 * sizeof(Vehicle *));
    *count = 0;
    collectVehicles(root, vehicleArray, count);
    return vehicleArray;
}


// Function to create a new B+ Tree node
BPTreeNode *createNode(int isLeaf) {
    BPTreeNode *node = (BPTreeNode *)malloc(sizeof(BPTreeNode));
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->next = NULL;
    for (int i = 0; i <= MAX_KEYS; i++) {
        node->children[i] = NULL;
    }
    return node;

} 
BPTreeNode *findLeaf(BPTreeNode *node, char *vehicleNumber) {
    while (!node->isLeaf) {
        int i = 0;
        while (i < node->numKeys && strcmp(vehicleNumber, node->keys[i]->vehicleNumber) > 0) {
            i++;
        }
        node = node->children[i];
    }
    return node;
}
Vehicle* searchVehicle(BPTreeNode *node, const char *vehicleNumber) {
    if (!node) return NULL;

    int i = 0;
    while (i < node->numKeys && strcmp(vehicleNumber, node->keys[i]->vehicleNumber) > 0) {
        i++;
    }

    if (node->isLeaf) {
        if (i < node->numKeys && strcmp(vehicleNumber, node->keys[i]->vehicleNumber) == 0) {
            return node->keys[i];
        }
        return NULL;
    }

    return searchVehicle(node->children[i], vehicleNumber);
}

BPTreeNode *insertInternal(BPTreeNode *node, Vehicle *vehicle, BPTreeNode *child) {
    if (!node) return NULL;

    if (searchVehicle(node, vehicle->vehicleNumber)) {
        return node; // Already exists in internal path, skip
    }

    if (node->numKeys < MAX_KEYS) {
        int i = node->numKeys - 1;
        while (i >= 0 && strcmp(vehicle->vehicleNumber, node->keys[i]->vehicleNumber) < 0) {
            node->keys[i + 1] = node->keys[i];
            node->children[i + 2] = node->children[i + 1];
            i--;
        }
        node->keys[i + 1] = vehicle;
        node->children[i + 2] = child;
        node->numKeys++;
        return node;
    }

    Vehicle *tempKeys[MAX_KEYS + 1];
    BPTreeNode *tempChildren[MAX_KEYS + 2];

    for (int i = 0; i < MAX_KEYS; i++) {
        tempKeys[i] = node->keys[i];
        tempChildren[i] = node->children[i];
    }
    tempChildren[MAX_KEYS] = node->children[MAX_KEYS];

    int i = MAX_KEYS - 1;
    while (i >= 0 && strcmp(vehicle->vehicleNumber, tempKeys[i]->vehicleNumber) < 0) {
        tempKeys[i + 1] = tempKeys[i];
        tempChildren[i + 2] = tempChildren[i + 1];
        i--;
    }
    tempKeys[i + 1] = vehicle;
    tempChildren[i + 2] = child;

    int mid = (MAX_KEYS + 1) / 2;
    Vehicle *promoteKey = tempKeys[mid];

    BPTreeNode *newInternal = createNode(0);
    node->numKeys = 0;

    for (int i = 0; i < mid; i++) {
        node->keys[i] = tempKeys[i];
        node->children[i] = tempChildren[i];
        node->numKeys++;
    }
    node->children[mid] = tempChildren[mid];

    for (int i = mid + 1, j = 0; i <= MAX_KEYS; i++, j++) {
        newInternal->keys[j] = tempKeys[i];
        newInternal->children[j] = tempChildren[i];
        newInternal->numKeys++;
    }
    newInternal->children[newInternal->numKeys] = tempChildren[MAX_KEYS + 1];

    if (node == root) {
        BPTreeNode *newRoot = createNode(0);
        newRoot->keys[0] = promoteKey;
        newRoot->children[0] = node;
        newRoot->children[1] = newInternal;
        newRoot->numKeys = 1;
        return newRoot;
    }

    return insertInternal(root, promoteKey, newInternal);
}

BPTreeNode *insert(BPTreeNode *root, Vehicle *vehicle) {
    if (!vehicle || searchVehicle(root, vehicle->vehicleNumber)) {
        printf("Duplicate or invalid vehicle found. Skipping insertion: %s\n", vehicle ? vehicle->vehicleNumber : "NULL");
        return root;
    }

    if (!root) {
        root = createNode(1);
        root->keys[0] = vehicle;
        root->numKeys = 1;
        return root;
    }

    BPTreeNode *leaf = findLeaf(root, vehicle->vehicleNumber);

    // Double-check in the exact leaf before inserting
    for (int j = 0; j < leaf->numKeys; j++) {
        if (strcmp(leaf->keys[j]->vehicleNumber, vehicle->vehicleNumber) == 0) {
            printf("Duplicate found in leaf node. Skipping: %s\n", vehicle->vehicleNumber);
            return root;
        }
    }

    // Insert in sorted order
    int i = leaf->numKeys - 1;
    while (i >= 0 && strcmp(vehicle->vehicleNumber, leaf->keys[i]->vehicleNumber) < 0) {
        leaf->keys[i + 1] = leaf->keys[i];
        i--;
    }
    leaf->keys[i + 1] = vehicle;
    leaf->numKeys++;

    if (leaf->numKeys <= MAX_KEYS) return root;

    // Split leaf node
    int mid = (MAX_KEYS + 1) / 2;
    BPTreeNode *newLeaf = createNode(1);

    for (int i = mid, j = 0; i < leaf->numKeys; i++, j++) {
        newLeaf->keys[j] = leaf->keys[i];
        newLeaf->numKeys++;
        leaf->keys[i] = NULL;
    }

    leaf->numKeys = mid;
    newLeaf->next = leaf->next;
    leaf->next = newLeaf;

    Vehicle *promoteKey = newLeaf->keys[0];

    if (root == leaf) {
        BPTreeNode *newRoot = createNode(0);
        newRoot->keys[0] = promoteKey;
        newRoot->children[0] = leaf;
        newRoot->children[1] = newLeaf;
        newRoot->numKeys = 1;
        return newRoot;
    }

    return insertInternal(root, promoteKey, newLeaf);
}


// Function to save parking spaces to a file
// Function to save parking spaces to a file
void saveParkingSpaces() {
    FILE *file = fopen(PARKING_FILE, "w");
    if (!file) {
        printf("Error opening parking file.\n");
        return;
    }
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        fprintf(file, "%d %d %d %d\n", parkingSpaces[i].spaceID, parkingSpaces[i].isOccupied,
                parkingSpaces[i].totalUsageCount, parkingSpaces[i].totalRevenue);
    }
    fclose(file);
}


void loadParkingSpaces() {
    FILE *file = fopen(PARKING_FILE, "r");
    if (!file) {
        printf("Parking file not found. Initializing default spaces.\n");
        initializeParkingSpaces();  // Only initialize if file doesn't exist
        saveParkingSpaces();  // Save the initialized state to file
        return;
    }

    int i = 0;
    while (i < MAX_PARKING_SPACES && fscanf(file, "%d %d %d %d", 
           &parkingSpaces[i].spaceID, &parkingSpaces[i].isOccupied,
           &parkingSpaces[i].totalUsageCount, &parkingSpaces[i].totalRevenue) == 4) {
        i++;
    }
    fclose(file);
}

#define MAX_VEHICLES 1000  // Set based on your expected max entries
#define VEHICLE_NUM_LEN 20 // Adjust based on your format

#define MAX_VEHICLES 1000

int isExactDuplicate(Vehicle *v, Vehicle *savedList[], int count) {
    for (int i = 0; i < count; i++) {
        Vehicle *s = savedList[i];
        if (strcmp(v->vehicleNumber, s->vehicleNumber) == 0 &&
            strcmp(v->ownerName, s->ownerName) == 0 &&
            v->arrivalTime == s->arrivalTime &&
            v->departureTime == s->departureTime &&
            v->totalHours == s->totalHours &&
            v->totalCount == s->totalCount &&
            v->membership == s->membership &&
            v->parkingSpace == s->parkingSpace &&
            v->amountPaid == s->amountPaid &&
            v->isParked == s->isParked) {
            return 1; // Found an exact duplicate
        }
    }
    return 0;
}

void saveVehicles(BPTreeNode *node, FILE *file) {
    if (!node) return;

    // Static variables to track saved vehicles across recursive calls
    static Vehicle *savedVehicles[MAX_VEHICLES];
    static int savedCount = 0;

    for (int i = 0; i < node->numKeys; i++) {
        Vehicle *v = node->keys[i];

        if (!isExactDuplicate(v, savedVehicles, savedCount)) {
            fprintf(file, "%s %s %ld %ld %.2f %d %d %d %d %d\n",
                    v->vehicleNumber, v->ownerName, v->arrivalTime, v->departureTime, 
                    v->totalHours, v->totalCount, v->membership, v->parkingSpace, 
                    v->amountPaid, v->isParked);

            savedVehicles[savedCount++] = v; // Store pointer to prevent duplicate
        }
    }

    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++)
            saveVehicles(node->children[i], file);
    }

    // Optional: Reset static variables after full traversal
    if (node->isLeaf && node->next == NULL) {
        savedCount = 0;
    }
}


void loadVehicles() {
    FILE *file = fopen(VEHICLE_FILE, "r");
    if (!file) {
        printf("Vehicle file not found. Starting fresh.\n");
        return;
    }

    while (!feof(file)) {
        Vehicle *v = (Vehicle *)malloc(sizeof(Vehicle));
        if (!v) {
            printf("Memory allocation failed for vehicle.\n");
            break;
        }

        int result = fscanf(file, "%14s %49s %ld %ld %lf %d %d %d %d %d",
                            v->vehicleNumber, v->ownerName, &v->arrivalTime, &v->departureTime, 
                            &v->totalHours, &v->totalCount, &v->membership, &v->parkingSpace, 
                            &v->amountPaid, &v->isParked);

        if (result == EOF) {
            free(v);
            break;
        }

        if (result == 10) {  // Ensure all fields were read
            root = insert(root, v);
        } else {
            printf("Error: Data corruption in vehicle file. Skipping entry.\n");
            free(v);
        }
    }

    fclose(file);
}

// Function to search for a vehicle
Vehicle *search(BPTreeNode *node, char *vehicleNumber) {
    if (!node) return NULL;
    int i = 0;
    while (i < node->numKeys && strcmp(vehicleNumber, node->keys[i]->vehicleNumber) > 0)
        i++;

    if (i < node->numKeys && strcmp(vehicleNumber, node->keys[i]->vehicleNumber) == 0)
        return node->keys[i];

    if (node->isLeaf)
        return NULL;

    return search(node->children[i], vehicleNumber);
}


int assignParkingSpace(int membership) {
    int start, end;

    if (membership == 2) {  
        start = 0; end = 9;   
    } else if (membership == 1) {  
        start = 10; end = 19;  
    } else {  
        start = 20; end = 49;  
    }

    for (int i = start; i <= end; i++) {
        if (!parkingSpaces[i].isOccupied) {
            parkingSpaces[i].isOccupied = 1;  
            saveParkingSpaces();  
            return parkingSpaces[i].spaceID;
        }
    }

    return -1;  // No space available
}

// Function to register a vehicle
void registerVehicle(char *vehicleNumber, char *ownerName) {
    Vehicle *existingVehicle = search(root, vehicleNumber);

    if (existingVehicle) {
        if (existingVehicle->isParked) {
            printf("Error: Vehicle is already parked.\n");
            return;
        }

        existingVehicle->arrivalTime = time(NULL);
        existingVehicle->isParked = 1;
        existingVehicle->totalCount++; // *Increment total entry count*
        existingVehicle->parkingSpace = assignParkingSpace(existingVehicle->membership);

        printf("Vehicle re-entered successfully. Assigned Parking Space: %d\n", existingVehicle->parkingSpace);

        FILE *file = fopen(VEHICLE_FILE, "w");
        if (file) {
            saveVehicles(root, file);
            fclose(file);
        }
        saveParkingSpaces();
        return;
    }

    // New vehicle registration
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    strcpy(vehicle->vehicleNumber, vehicleNumber);
    strcpy(vehicle->ownerName, ownerName);
    vehicle->totalHours = 0;
    vehicle->totalCount = 1; // *First entry*
    vehicle->membership = 0;
    vehicle->amountPaid = 0;
    vehicle->arrivalTime = time(NULL);
    vehicle->isParked = 1;

    vehicle->parkingSpace = assignParkingSpace(0);
    if (vehicle->parkingSpace == -1) {
        printf("No parking space available!\n");
        free(vehicle);
        return;
    }

    root = insert(root, vehicle);
    printf("Vehicle registered successfully with Parking Space %d.\n", vehicle->parkingSpace);

    FILE *file = fopen(VEHICLE_FILE, "w");
    if (file) {
        saveVehicles(root, file);
        fclose(file);
    }
    saveParkingSpaces();
}

// Function to calculate parking fee
int calculateFee(double hours, int membership) {
    int roundedHours = (int)(hours + 0.5);  
    int fee = 100;
    if (roundedHours > 3) fee += (roundedHours - 3) * 50;
    if (membership > 0) fee *= 0.9;  
    return fee;
}

// Function to process vehicle exit
void exitVehicle(char *vehicleNumber) {
    Vehicle *vehicle = search(root, vehicleNumber);
    if (!vehicle) {
        printf("Error: Vehicle not found!\n");
        return;
    }

    if (!vehicle->isParked) {
        printf("Error: Vehicle has already exited!\n");
        return;
    }

    vehicle->departureTime = time(NULL);
    double parkedHours = difftime(vehicle->departureTime, vehicle->arrivalTime) / 3600.0;
    vehicle->totalHours += parkedHours;
    int fee = calculateFee(parkedHours, vehicle->membership);
    vehicle->amountPaid += fee;

    int spaceIndex = vehicle->parkingSpace - 1;
    parkingSpaces[spaceIndex].isOccupied = 0;
    parkingSpaces[spaceIndex].totalUsageCount++;
    parkingSpaces[spaceIndex].totalRevenue += fee;

    if (vehicle->totalHours >= 200)
        vehicle->membership = 2;  
    else if (vehicle->totalHours >= 100)
        vehicle->membership = 1;  

    vehicle->isParked = 0;
    printf("Vehicle exited. Parked for %.2f hours. Total amount paid: Rs %d\n", parkedHours, vehicle->amountPaid);
    FILE *file = fopen(VEHICLE_FILE, "w");
    if (file) {
        saveVehicles(root, file);
        fclose(file);
    }
    saveParkingSpaces();
}

// Function to display all vehicles
void displayVehicles(BPTreeNode *node) {
    if (!node) return;

    if (node->isLeaf) {
        for (int i = 0; i < node->numKeys; i++) {
            Vehicle *v = node->keys[i];
            printf("Vehicle Number: %s | Owner Name: %s | Total Hours: %.2f | Total Entries: %d | Membership: %d | Parking Space: %d | Total Amount Paid: Rs %d\n",
                v->vehicleNumber, v->ownerName, v->totalHours, v->totalCount, v->membership, v->parkingSpace, v->amountPaid);
        }
    } else {
        for (int i = 0; i <= node->numKeys; i++) {
            displayVehicles(node->children[i]);
        }
    }
}

// Function to display all parking spaces
void displayParkingSpaces() {
    printf("Parking Space Details:\n");
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        printf("Space ID: %d | Occupied: %d | Total Usage Count: %d | Total Revenue: Rs %d\n",
               parkingSpaces[i].spaceID, parkingSpaces[i].isOccupied,
               parkingSpaces[i].totalUsageCount, parkingSpaces[i].totalRevenue);
    }
}

// Count vehicles only in leaf nodes
int countVehiclesInLeaves(BPTreeNode *node) {
    if (!node) return 0;
    int total = 0;

    if (node->isLeaf) {
        total += node->numKeys;
    } else {
        for (int i = 0; i <= node->numKeys; i++) {
            total += countVehiclesInLeaves(node->children[i]);
        }
    }

    return total;
}

// Collect vehicles only from leaf nodes
void collectVehiclesFromLeaves(BPTreeNode *node, Vehicle **arr, int *count) {
    if (!node) return;

    if (node->isLeaf) {
        for (int i = 0; i < node->numKeys; i++) {
            arr[*count] = node->keys[i];
            (*count)++;
        }
    } else {
        for (int i = 0; i <= node->numKeys; i++) {
            collectVehiclesFromLeaves(node->children[i], arr, count);
        }
    }
}

// Copy vehicle pointers from leaf nodes only
Vehicle **copyVehiclesFromLeaves(int *count) {
    *count = countVehiclesInLeaves(root);
    if (*count == 0) return NULL;

    Vehicle **arr = (Vehicle **)malloc(sizeof(Vehicle *) * (*count));
    if (!arr) return NULL;

    int index = 0;
    collectVehiclesFromLeaves(root, arr, &index);
    return arr;
}


// Compare by total number of times a vehicle has parked
int compareByTotalParkings(const void *a, const void *b) {
    Vehicle *v1 = *(Vehicle **)a;
    Vehicle *v2 = *(Vehicle **)b;
    return v2->totalCount - v1->totalCount; // Descending order
}

// Compare by total amount paid by a vehicle
int compareByAmountPaid(const void *a, const void *b) {
    Vehicle *v1 = *(Vehicle **)a;
    Vehicle *v2 = *(Vehicle **)b;
    return v2->amountPaid - v1->amountPaid; // Descending order
}

// Compare by total usage count of parking spaces
int compareByUsageCount(const void *a, const void *b) {
    ParkingSpace *p1 = (ParkingSpace *)a;
    ParkingSpace *p2 = (ParkingSpace *)b;
    return p2->totalUsageCount - p1->totalUsageCount; // Descending order
}

// Compare by total revenue generated by a parking space
int compareByRevenue(const void *a, const void *b) {
    ParkingSpace *p1 = (ParkingSpace *)a;
    ParkingSpace *p2 = (ParkingSpace *)b;
    return p2->totalRevenue - p1->totalRevenue; // Descending order
}

void sortByTotalParkings() {
    int count = 0;
    Vehicle **vehicles = copyVehiclesFromLeaves(&count);  // only from leaf nodes

    if (!vehicles) {
        printf("Memory allocation failed or no vehicles to sort!\n");
        return;
    }

    qsort(vehicles, count, sizeof(Vehicle *), compareByTotalParkings);

    printf("\nVehicles Sorted by Total Parkings:\n");
    for (int i = 0; i < count; i++) {
        printf("Vehicle: %s | Total Parkings: %d\n", vehicles[i]->vehicleNumber, vehicles[i]->totalCount);
    }

    free(vehicles);
}


void sortByAmountPaid() {
    int count = 0;
    Vehicle **vehicles = copyVehiclesFromLeaves(&count);  // only from leaf nodes

    if (!vehicles) {
        printf("Memory allocation failed or no vehicles to sort!\n");
        return;
    }

    qsort(vehicles, count, sizeof(Vehicle *), compareByAmountPaid);

    printf("\nVehicles Sorted by Amount Paid:\n");
    for (int i = 0; i < count; i++) {
        printf("Vehicle: %s | Amount Paid: Rs %d\n", vehicles[i]->vehicleNumber, vehicles[i]->amountPaid);
    }

    free(vehicles);  // Free memory after use
}


// Function to sort and display parking spaces by usage count
void sortByUsageCount() {
    ParkingSpace temp[MAX_PARKING_SPACES];
    memcpy(temp, parkingSpaces, sizeof(parkingSpaces));

    qsort(temp, MAX_PARKING_SPACES, sizeof(ParkingSpace), compareByUsageCount);
    printf("\nParking Spaces Sorted by Usage Count:\n");
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        printf("Space ID: %d | Total Usage: %d\n", temp[i].spaceID, temp[i].totalUsageCount);
    }
}

// Function to sort and display parking spaces by revenue
void sortByRevenue() {
    ParkingSpace temp[MAX_PARKING_SPACES];
    memcpy(temp, parkingSpaces, sizeof(parkingSpaces));

    qsort(temp, MAX_PARKING_SPACES, sizeof(ParkingSpace), compareByRevenue);
    printf("\nParking Spaces Sorted by Revenue:\n");
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        printf("Space ID: %d | Total Revenue: Rs %d\n", temp[i].spaceID, temp[i].totalRevenue);
    }
}



int main() {
    initializeParkingSpaces();
    loadParkingSpaces();
    loadVehicles();

    int choice;
    char vehicleNumber[15], ownerName[50];

    while (1) {
        printf("\n1. Register Vehicle\n2. Vehicle Exit\n3. Display Vehicles\n");
        printf("4. Display Parking Spaces\n5. Sort by Total Parkings\n6. Sort by Amount Paid\n7. Sort by Parking Space Usage\n8. Sort by Parking Space Revenue\n9. Exit\nEnter choice: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            printf("Enter Vehicle Number: ");
            scanf("%s", vehicleNumber);
            getchar();
            printf("Enter Owner Name: ");
            fgets(ownerName, 50, stdin);
            ownerName[strcspn(ownerName, "\n")] = 0;
            registerVehicle(vehicleNumber, ownerName);
        }
        else if (choice == 2) {
            printf("Enter Vehicle Number: ");
            scanf("%s", vehicleNumber);
            exitVehicle(vehicleNumber);
        }
        else if (choice == 3) {
            displayVehicles(root);
        }
        else if (choice == 4) {
            displayParkingSpaces();
        }
        else if (choice == 5) {
            sortByTotalParkings();
        }
        else if (choice == 6) {
            sortByAmountPaid();
        }
        else if (choice == 7) {
            sortByUsageCount();
        }
        else if (choice == 8) {
            sortByRevenue();
        }
        else if (choice == 9) {
            return 0;
        }
        else {
            printf("Invalid choice.\n");
        }
    }
}
