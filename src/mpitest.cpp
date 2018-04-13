#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <stack>
#include <queue>
#include <chrono>
#include <algorithm>
#include <mpi.h>
#include <omp.h>
#include <thread>

void master(int world_size, int argc, char** argv) {
    MPI_Status status;
    char data[5];
    for (int i = 0; i < 7; ++i) {
        std::cout << "[master] Sleeping for 3 secs" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "[master] Waiting..." << std::endl;
        MPI_Recv(data, 5, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        std::cout << "[master] Received data from " << status.MPI_SOURCE << " with tag " << status.MPI_TAG << std::endl;
    }
}

void slave(int world_rank) {
    char data[5] = "test";
    int sleep = 15 + world_rank * 3;
    std::cout << "[slave] Sending 3x some data to master with tag 1..." << std::endl;
    MPI_Send(data, 5, MPI_BYTE, 0, 100, MPI_COMM_WORLD);
    MPI_Send(data, 5, MPI_BYTE, 0, 100, MPI_COMM_WORLD);
    MPI_Send(data, 5, MPI_BYTE, 0, 100, MPI_COMM_WORLD);
    std::cout << "[slave] Sleeping for " << sleep << " secs..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(sleep));
    std::cout << "[slave] Sending some data to master with tag 3..." << std::endl;
    MPI_Send(data, 5, MPI_BYTE, 0, 103, MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
    int world_rank = 0;
    int world_size;

    // Initialize MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Branch out the program execution
    if (world_rank == 0) {
        master(world_size, argc, argv); 
    } else {
        slave(world_rank);
    }

    // Finalize MPI
    MPI_Finalize();
    
    return 0;
}