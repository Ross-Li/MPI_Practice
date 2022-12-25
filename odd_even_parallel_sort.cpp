//TODO: Implement if (return of function != error code) {following operations} method
//TODO: Try to use sending to do the most precise timing in debug function. Send the rank 0 t1 to other processes.
//TODO: Try to ONLY declare the elements[] and sorted_elements[] variable in rank 0
//TODO: Try to make use of dynamic memory allocation???
//TODO: Try to make use of non-blocking routines???

#include <mpi.h>        
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <chrono>           // For timing purposes
#include <bits/stdc++.h>    // For swap function
#include <array>            // For array operations

// DEBUG is for printing out some general information as the program is execuated.
// #define DEBUG

// MORE_DEBUG is for printing out debug functions that prints out every element in the 
// according debugged array. DO NOT enable this if the input size is BIG!!!
// #define MORE_DEBUG

// Debug function to print time, rank and info
void debug(
    std::chrono::high_resolution_clock::time_point starttime, 
    int rank, 
    char *format, 
    ...
    ) {
    va_list args;
    va_start(args, format);

    std::chrono::high_resolution_clock::time_point now;
    std::chrono::duration<double> time_span;

    now = std::chrono::high_resolution_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::milliseconds>(now - starttime);

    printf("Rank Time: %8.6f | Rank: %2d | ", time_span, rank);
    vprintf(format, args);

    va_end(args);
}


void even_odd(int num_elements, int elements[]) {
    for (int i = 0; i < num_elements; i += 2) {
        if (elements[i] > elements[i + 1])
            std::swap(elements[i], elements[i + 1]);
    }
}

void odd_even ( int rank, 
                int rank_prev, 
                int rank_next, 
                int num_elements, 
                int elements[], 
                int &prev_buffer, 
                int &next_buffer, 
                MPI_Status &status) {
    // Odd-even sort within the current process 
    for (int i = 1; i < num_elements - 1; i += 2) {
        if (elements[i] > elements[i + 1])
            std::swap(elements[i], elements[i + 1]);
    }

    // Odd-even sort between the current process and neighbouring processes
        // Send the first element of current process to the previous process's next_buffer,
        // and receive the first element of the next process to the current process's 
        // next_buffer.
    MPI_Sendrecv(&elements[0], 1, MPI_INT, rank_prev, 0,
                 &next_buffer, 1, MPI_INT, rank_next, 0,
                 MPI_COMM_WORLD, &status);     

        // Send the last element of the current process to the next process's prev_buffer,
        // and receive the last element of the previous process to the current process's
        // prev_buffer.
    MPI_Sendrecv(&elements[num_elements-1], 1, MPI_INT, rank_next, 0,
                 &prev_buffer, 1, MPI_INT, rank_prev, 0,
                 MPI_COMM_WORLD, &status);             
        
    if (prev_buffer > elements[0]) {
        elements[0] = prev_buffer;
    }        

    if (next_buffer < elements[num_elements-1]) {
        elements[num_elements-1] = next_buffer;
    }
}

int main (int argc, char **argv) {
    MPI_Init(&argc, &argv); 

    // Initialize and get current process rank
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  

    // Initialize and get the number of total processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get previous rank. For rank 0, the previosu rank is MPI_PROC_NULL
    int rank_prev = rank - 1;
    if (rank_prev < 0) {
        rank_prev = MPI_PROC_NULL;
    }

    // Get next rank. For the last rank, the next rank is MPI_PROC_NULL
    int rank_next = rank + 1;
    if (rank_next > world_size - 1) {
        rank_next = MPI_PROC_NULL;
    }
    
    // Declare number of total elements to be sorted
    int num_elements;     
    // Get and convert command line argument to num_elements
    num_elements = atoi(argv[1]); 

    // Declare variable to store all unsorted elements
    int elements[num_elements]; 

    // Declare variable to store all sorted elements
    int sorted_elements[num_elements]; 

    // Read inputs from file (only in master process)
    if (rank == 0) { 
        // input is a ifstream
        std::ifstream input(argv[2]);
        int input_element;
        int input_iter = 0;
        while (input >> input_element) {
            elements[input_iter] = input_element;
            input_iter++;
        }
        std::cout << "Actual number of elements:" << input_iter << std::endl;
    }

    // Get number of elements allocated to each process
    int num_my_elements = num_elements / world_size;
    
    // Declare variable to store elements of current process 
    int my_elements[num_my_elements]; 

    // Buffer area to store the number sent from the previous and next process
    // prev_buffer and next_buffer are initialized to INT_MIN and INT_MAX to avoid
    // error on the first odd-even pass
    int prev_buffer = INT_MIN;
    int next_buffer = INT_MAX;

    // Try to collect error message in this status
    MPI_Status status;

    if (rank == 0) {
        std::cout << "Student ID: " << "119020026" << std::endl;
        std::cout << "Name: " << "李沛霖" << std::endl;
        std::cout << "Assignment 1" << std::endl;
        std::cout << "Input Size: " << num_elements << std::endl;
        std::cout << "Number of Processes: " << world_size << std::endl;   
        std::cout << "Number of elements per process: " << num_my_elements << std::endl;
    }

    // Start recording time (only in master process)
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;
    std::chrono::duration<double> time_span;
     
    t1 = std::chrono::high_resolution_clock::now();
    
    // Distribute elements to each process (only in master process)
    #ifdef DEBUG
    if (rank == 0) {
        debug(t1, 0, "Start scattering... \n");
    }
    #endif
    
    MPI_Scatter(
        elements,           // sendbuf
        num_my_elements,    // senfcount
        MPI_INT,            
        my_elements,      
        num_my_elements, 
        MPI_INT, 
        0, 
        MPI_COMM_WORLD
    ); 
    
    // Print out what elements each process received
    #ifdef MORE_DEBUG
    for (int i = 0; i < num_my_elements; i++) {
        debug(t1, rank, "Received scattered element: %d \n", my_elements[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    #endif

    // Create a barrier, make sure that all processes enter the following sorting 
    // stage AFTER scattering (receving their share of data).
    MPI_Barrier(MPI_COMM_WORLD);

    #ifdef DEBUG
    if (rank == 0)
        debug(t1, 0, "Scattering complete! \n");
    #endif

    // Actual Sorting Loop
    for (int pass = 0; pass < (num_elements/2); pass++) {
        even_odd(num_my_elements, my_elements);

        // Create a barrier, make sure to start odd-even phase ONLY after all 
        // even-odd phase processes complete.
        MPI_Barrier(MPI_COMM_WORLD);
        
        odd_even(
            rank, 
            rank_prev, 
            rank_next, 
            num_my_elements, 
            my_elements, 
            prev_buffer,
            next_buffer,
            status
        );
        
        // Create a barrier, make sure to start the next loop of even-odd phase 
        // ONLY after all odd-even phase processes complete.
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Create a barrier. Make sure that all sorting complete before printing out
    // the sorted results
    MPI_Barrier(MPI_COMM_WORLD);

    #ifdef DEBUG
    if (rank == 0)    
        debug(t1, rank, "Sorting complete!");
    #endif

    // Print out the sorted array in each process
    #ifdef MORE_DEBUG
    for (int i = 0; i < num_my_elements; i++) {
        debug(t1, rank, "%d th element after sorting: %d \n", i+1, my_elements[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    #endif

    // Create a barrier, make sure all processes are complete with the preceding 
    // operations BEFORE gathering.
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Collect sorted result from each process (only in master process)
    #ifdef DEBUG
    if (rank == 0)
        debug(t1, 0, "Start gathering... \n");
    #endif

    MPI_Gather(
        my_elements, 
        num_my_elements, 
        MPI_INT, 
        sorted_elements, 
        num_my_elements, 
        MPI_INT, 
        0, 
        MPI_COMM_WORLD
    ); 
    
    MPI_Barrier(MPI_COMM_WORLD);

    // Print out what sorted elements rank 0 have actually gathered
    #ifdef MORE_DEBUG
    if (rank == 0) {    
        for (int i = 0; i < num_elements; i++) {
            debug(t1, rank, "%d th number in rank 0 after sorting: %d \n", i+1, sorted_elements[i]);
        }
    }
    #endif

    #ifdef DEBUG
    if (rank == 0) {
        debug(t1, 0, "Gathering complete! \n");
    }
    #endif 

    /* TODO END */

    // Record and output time and info (only in master process)
    if (rank == 0){ 
        t2 = std::chrono::high_resolution_clock::now();  
        time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        std::cout << "Run Time: " << time_span.count() << " seconds" << std::endl;
    }

    // Print out the first 20 elements of the sorted array (only in master process)
    if (rank == 0) {
        std::cout << "First 20 elements of the sorted array: " << std::endl;
        for (int i = 0; i < 20; i++) {
            std::cout << i+1 << " th element: " << sorted_elements[i] << std::endl;
        }
    }

    // Store the sorted array in XXXpar.out file (only in master process) 
    if (rank == 0) {      
        std::string file_string(argv[2]);
        file_string = file_string.substr(0, file_string.size()-3);
        file_string.append("par.out");
        // `output` is a ofstream object. argv[2] is the 3rd inputted parameter of this program. 
        std::ofstream output(file_string, std::ios_base::out);
        for (int i = 0; i < num_elements; i++) {
            output << sorted_elements[i] << std::endl;
        }
        std::cout << "Stored sorted array in generated output file: " << file_string << std::endl;
    }
    
    MPI_Finalize();
    
    return 0;
}


