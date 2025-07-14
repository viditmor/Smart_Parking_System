#include <stdio.h>
#include <string.h>
#include <time.h>

struct Vehicle
{
    char vehicle_number[20]; 
    char owner_name[100];
    time_t arrivaltime; //Takes realtime when car enters
    time_t departtime;  //Takes realtime when car exit
    char membership[10];
    float totalparking_hours; //Total lifetime parking hours
    int parkingID;
    float Revenue; //Total lifetime revenue  of that vehicle
    int no_of_Parking; //Total lifetime parkings done 
};

struct ParkingSpace
{
    int spaceID; //1-50
    int isOccupied; // 0 or 1
    float revenueGenerated; //Total revenue on that parking space 
    int occupancy; //Total number of times that space occupied till now 
};

int vehicleCount = 0;  // Global declaration of total vehicles in the database
struct ParkingSpace parkingSpaces[50];
struct Vehicle vehicles[100];

void saveDataToFile()
{
    FILE *file = fopen("data.txt", "w");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return;
    }
    printf("File opened successfully for saving data.\n");


    // Save vehicle data
    fprintf(file, "%d\n", vehicleCount);
    for (int i = 0; i < vehicleCount; i++)
    {
        fprintf(file, "%s %s %ld %f %d %f %d\n", vehicles[i].vehicle_number, vehicles[i].owner_name, vehicles[i].arrivaltime, vehicles[i].totalparking_hours, vehicles[i].parkingID, vehicles[i].Revenue, vehicles[i].no_of_Parking);
    }

    // Save parking space data
    for (int i = 0; i < 50; i++)
    {
        fprintf(file, "%d %d %f %d\n", parkingSpaces[i].spaceID, parkingSpaces[i].isOccupied, parkingSpaces[i].revenueGenerated, parkingSpaces[i].occupancy);
    }

    fclose(file);
}

void loadDataFromFile()
{
    FILE *file = fopen("data.txt", "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return;
    }
    
    printf("Loading data from file\n");

    // Loading vehicle data
    fscanf(file, "%d", &vehicleCount);
    for (int i = 0; i < vehicleCount; i++)
    {
        fscanf(file, "%s %s %ld %ld %f %d %f %d", vehicles[i].vehicle_number, vehicles[i].owner_name, &vehicles[i].arrivaltime,&vehicles[i].departtime, &vehicles[i].totalparking_hours, &vehicles[i].parkingID, &vehicles[i].Revenue, &vehicles[i].no_of_Parking);
    }

    // Load parking space data
    for (int i = 0; i < 50; i++)
    {
        fscanf(file, "%d %d %f %d", &parkingSpaces[i].spaceID, &parkingSpaces[i].isOccupied, &parkingSpaces[i].revenueGenerated, &parkingSpaces[i].occupancy);
    }

    fclose(file);  // Close the file
    
}





void initializeParkingSpaces(struct ParkingSpace parkingSpaces[])
{
    for (int i = 0; i < 50; i++)
    {
        parkingSpaces[i].spaceID = i + 1;
        parkingSpaces[i].isOccupied = 0;
        parkingSpaces[i].revenueGenerated = 0;
        parkingSpaces[i].occupancy = 0;
    }
}

int findVehicle(char vehiclenumber[])
{
    for (int i = 0; i < vehicleCount; i++)
    {
        if (strcmp(vehicles[i].vehicle_number, vehiclenumber) == 0)
        {
            return i;
        }
    }
    return -1;
}

int findParkingSpace(float totalparkinghours)
{
    int start;

    if (totalparkinghours >= 200.00)
    {
        start = 0;
    }
    else if ((totalparkinghours >= 100.00) && (totalparkinghours < 200.00))
    {
        start = 10;
    }
    else
    {
        start = 20;
    }

    for (int i = start; i < 50; i++)
    {
        if (parkingSpaces[i].isOccupied == 0)
        {
            return i;
        }
    }
    return -1;
}

void parkVehicle(char vehicleNumber[], char ownerName[], float totalparkinghours, int vehicleIndex)
{
    time_t currentTime;
    time(&currentTime); // Input of arrival time

    // This if block is for vehicles that are already registered
    if (vehicleIndex != -1)
    {
        int parkingslot = findParkingSpace(totalparkinghours); // Returning the parking slot
        if (parkingslot == -1)
        {
            printf("No parking space available.\n");
        }
        else
        {
            vehicles[vehicleIndex].arrivaltime = currentTime;
            vehicles[vehicleIndex].parkingID = parkingslot;
            parkingSpaces[parkingslot].isOccupied = 1;
            parkingSpaces[parkingslot].occupancy+=1;  //increment in occupancy
            vehicles[vehicleIndex].no_of_Parking+=1;  //increment in no of parking
            printf("Your allotted parking slot is %d\n", parkingslot + 1);
        }
    }
    // New vehicle registration
    else
    {
        strcpy(vehicles[vehicleCount].vehicle_number, vehicleNumber); // Copy vehicle number
        strcpy(vehicles[vehicleCount].owner_name, ownerName);         // Copy owner name
        vehicles[vehicleCount].arrivaltime = currentTime;
        vehicles[vehicleCount].totalparking_hours = 0;
        int parkingslot = findParkingSpace(totalparkinghours);

        if (parkingslot != -1)
        {
            vehicles[vehicleCount].parkingID = parkingslot;
            parkingSpaces[parkingslot].isOccupied = 1;
            parkingSpaces[parkingslot].occupancy+=1; //increment in occupancy
            vehicles[vehicleCount].no_of_Parking+=1;//increment in no of parking

            printf("Your allotted parking slot is %d\n", parkingslot + 1);
            vehicleCount = vehicleCount + 1;
        }
        else
        {
            printf("No parking space available.\n");
        }
    }
}

float calculatepayment(float parkedhours, float totalparkinghours)
{
    float payment = 100.00;
    if (totalparkinghours < 100)
    {
        if (parkedhours <= 3)
        {
            return payment;
        }
        else
        {
            return payment + (parkedhours - 3) * 50;
        }
    }
    else
    {
        if (parkedhours <= 3)
        {
            return payment * 0.9;
        }
        else
        {
            return (payment + (parkedhours - 3) * 50) * 0.9;
        }
    }
}

void exitVehicle(char vehicleNumber[])
{
    time_t currentTime;
    time(&currentTime);

    for (int i = 0; i < vehicleCount; i++)
    {
        if (strcmp(vehicles[i].vehicle_number, vehicleNumber) == 0)
        {
            vehicles[i].departtime = currentTime;
            float parkedhours = (difftime(vehicles[i].departtime, vehicles[i].arrivaltime) / 3600.00); //Converting seconds to hours 
            vehicles[i].totalparking_hours += parkedhours; // Increment in total lifetime parking hours in vehicle's database

            float payment = calculatepayment(parkedhours, vehicles[i].totalparking_hours);
            vehicles[i].Revenue += payment;  // Increment in total lifetime revenue in vehicle's database
            printf("Your car has exited. Your payment: %.2f\n", payment);

            int parkId = vehicles[i].parkingID;
            parkingSpaces[parkId].isOccupied = 0; //After exiting of vehicle returning 0 to that space
            parkingSpaces[parkId].revenueGenerated += payment; // Increment in total lifetime revenue in parking space database

            saveDataToFile(); 
        }
    }
}

// Partition function for quicksort for number of parkings
int partition_no_of_Parking(struct Vehicle vehicles[], int low, int high) 
{
    int pivot = vehicles[high].no_of_Parking;  // Choosing the last element as pivot
    int i = low - 1;

    for (int j = low; j < high; j++) 
    {
        if (vehicles[j].no_of_Parking > pivot) // Sorting in descending order
        {  
            i++;
            struct Vehicle temp = vehicles[i];
            vehicles[i] = vehicles[j];
            vehicles[j] = temp;
        }
    }

    struct Vehicle temp = vehicles[i + 1];
    vehicles[i + 1] = vehicles[high];
    vehicles[high] = temp;
    return i + 1;
}

// Quicksort function for number of parkings
void quicksort_no_of_Parking(struct Vehicle vehicles[], int low, int high) 
{
    if (low < high) 
    {
        int pi = partition_no_of_Parking(vehicles, low, high);

        quicksort_no_of_Parking(vehicles, low, pi - 1);
        quicksort_no_of_Parking(vehicles, pi + 1, high);
    }
}

// Partition function for quicksort for amount paid 
int partition_amount_paid(struct Vehicle vehicles[], int low, int high) 
{
    int pivot = vehicles[high].Revenue;  // Choosing the last element as pivot
    int i = low - 1;

    for (int j = low; j < high; j++) 
    {
        if (vehicles[j].Revenue > pivot) // Sorting in descending order
        {  
            i++;
            struct Vehicle temp = vehicles[i];
            vehicles[i] = vehicles[j];
            vehicles[j] = temp;
        }
    }

    struct Vehicle temp = vehicles[i + 1];
    vehicles[i + 1] = vehicles[high];
    vehicles[high] = temp;
    return i + 1;
}

// Quicksort function for amount paid
void quicksort_amount_paid(struct Vehicle vehicles[], int low, int high) 
{
    if (low < high)
    {
        int pi = partition_amount_paid(vehicles, low, high);

        quicksort_amount_paid(vehicles, low, pi - 1);
        quicksort_amount_paid(vehicles, pi + 1, high);
    }
}


// Partition function for quicksort (for occupncy)
int partition_occupancy(struct ParkingSpace parkingSpaces[], int low, int high) 
{
    int pivot = parkingSpaces[high].occupancy;
    int i = low - 1;

    for (int j = low; j < high; j++) 
    {
        if (parkingSpaces[j].occupancy > pivot) 
        {  
            i++;
            struct ParkingSpace temp = parkingSpaces[i];
            parkingSpaces[i] = parkingSpaces[j];
            parkingSpaces[j] = temp;
        }
    }

    struct ParkingSpace temp = parkingSpaces[i + 1];
    parkingSpaces[i + 1] = parkingSpaces[high];
    parkingSpaces[high] = temp;
    return i + 1;
}

// Quicksort function (for occupncy)
void quicksort_occupancy(struct ParkingSpace parkingSpaces[], int low, int high) 
{
    if (low < high) 
    {
        int pi = partition_occupancy(parkingSpaces, low, high);

        
        quicksort_occupancy(parkingSpaces, low, pi - 1);
        quicksort_occupancy(parkingSpaces, pi + 1, high);
    }
}

// Partition function for quicksort (for parking spaces which generated maximum revenue)
int partition_revenue(struct ParkingSpace parkingSpaces[], int low, int high) 
{
    int pivot = parkingSpaces[high].revenueGenerated;
    int i = low - 1;

    for (int j = low; j < high; j++) 
    {
        if (parkingSpaces[j].revenueGenerated > pivot) 
        {  
            i++;
            struct ParkingSpace temp = parkingSpaces[i];
            parkingSpaces[i] = parkingSpaces[j];
            parkingSpaces[j] = temp;
        }
    }

    struct ParkingSpace temp = parkingSpaces[i + 1];
    parkingSpaces[i + 1] = parkingSpaces[high];
    parkingSpaces[high] = temp;
    return i + 1;
}

// Quicksort function (for parking spaces which generated maximum revenue)
void quicksort_revenue(struct ParkingSpace parkingSpaces[], int low, int high) 
{
    if (low < high) 
    {
        int pi = partition_revenue(parkingSpaces, low, high);

        
        quicksort_revenue(parkingSpaces, low, pi - 1);
        quicksort_revenue(parkingSpaces, pi + 1, high);
    }
}


int main()
{
    loadDataFromFile();
    initializeParkingSpaces(parkingSpaces);

    int choice;
    char vehicleNumber[20]; //input vehicle number 
    char ownerName[100];
    float totalparkinghours;

    do
    {
        printf("\n1. Park Vehicle\n2. Exit Vehicle\n3. Sorting based on Number of Parking done \n4. Sorting based on Revenue\n5.Sorting based on Occupancy\n6.sorting of parking spaces which generated maximum revenue.\n7.exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            printf("Enter Vehicle Number: ");
            scanf("%s", vehicleNumber);

            int vehicleIndex = findVehicle(vehicleNumber);
            if (vehicleIndex != -1)
            {
                totalparkinghours = vehicles[vehicleIndex].totalparking_hours;
                printf("Vehicle already registered.\n");
                strcpy(ownerName, vehicles[vehicleIndex].owner_name);
            }
            else
            {
                printf("Enter Owner Name: ");
                scanf("%s", ownerName);
                totalparkinghours = 0;
            }

            parkVehicle(vehicleNumber, ownerName, totalparkinghours, vehicleIndex);
            break;

        
        case 2:
            printf("Enter Vehicle Number to Exit: ");
            scanf("%s", vehicleNumber);
            exitVehicle(vehicleNumber);
            break;
            
        
        case 3:
        printf("sorting based on number of parking done\n");
        quicksort_no_of_Parking(vehicles,0,vehicleCount-1);
         for (int  i = 0; i < vehicleCount; i++)
            {
                printf("%s %d\n",vehicles[i].vehicle_number,vehicles[i].no_of_Parking);
            }
        break;
            
        case 4:
        printf("sorting based on revenue generated");
        quicksort_amount_paid(vehicles,0,vehicleCount-1);
        for(int i=0;i<vehicleCount;i++)
        {
            printf("%f %s\n",vehicles[i].Revenue,vehicles[i].vehicle_number);
        }
        break;

        case 5:
         printf("Sorting based on Occupancy\n");
         quicksort_occupancy(parkingSpaces, 0, 49);  // Sorting spaces
        for (int i = 0; i < 50; i++)
         {
            printf("Parking Space %d: Occupied %d times\n", parkingSpaces[i].spaceID, parkingSpaces[i].occupancy);
         }
         break;

        case 6:
        printf("sorting of parking spaces which generated maximum revenue. ");
        quicksort_revenue(parkingSpaces,0,49);
        for (int i = 0; i < 50; i++)
         {
            printf("Parking Space %d: RevenueGenerated %.2f \n", parkingSpaces[i].spaceID, parkingSpaces[i].revenueGenerated);
         }
        break; 



        case 7:
            printf("Exiting the program\n");
            break;

        default:
            printf("Invalid choice\n");
        }
    } while (choice != 7);

    return 0;
}
