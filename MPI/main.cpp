#include <cstdio>
#include <cstring>
#include <climits>
#include <vector>
#include <queue>
#include <array>
#include <algorithm>
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include "node.h"
#include "mpi.h"

#define MASTER    0
#define REQUEST   1
#define REPLY     2
#define DATASIZE 13

std::vector<Node> visitedNodes;
std::string solutionPath;

const int rowToMove[] = { 1, 0, -1, 0 };
const int colToMove[] = { 0, -1, 0, 1 };
const int IterationsPerCheck = 1000;

void solve(const std::array<std::array<int, N>, N>& initial) {

    int numprocs, currentId, request = 1;
    int data[DATASIZE];
    auto startTimer = std::chrono::steady_clock::now();

    MPI_Status status;
    MPI_Comm world = MPI_COMM_WORLD;
    MPI_Comm_size(world, &numprocs);
    MPI_Comm_rank(world, &currentId);

    std::vector<Node> nodesStack;
 
    if (currentId == MASTER) {
        auto [x, y] = getBlankPosition(initial);
        Node root = Node(initial, x, y);
        root.calculateCost();
        nodesStack.push_back(root);
        visitedNodes.push_back(root);
    }

    int foundSolution = 0, receivedSolution = 0;
    int IterationCounter = numprocs;
 
    while (!foundSolution) {

        while (!nodesStack.empty() && IterationCounter) {
            IterationCounter--;
            Node lastNode = nodesStack.back();
    
            nodesStack.pop_back();
            if (visitedNodes.size() % 2000 == 0) {
                std::cout << "Proc " << currentId << ": visited nodes = " << visitedNodes.size() << std::endl;
            }
            if (lastNode.cost == 0) {
                printf("Found in proc %u!\n", currentId);
                solutionPath = lastNode.path();
                printf("%s\n", solutionPath.c_str());
                lastNode.printMatrix();
                foundSolution = currentId + 1;
                break;
            }
 
            for (int i = 0; i < 4; i++) {
                if (isSafe(lastNode.x + rowToMove[i], lastNode.y + colToMove[i])) {        
                    Node child = Node(lastNode);
                    child.swapBlank(lastNode.x, lastNode.y, lastNode.x + rowToMove[i], lastNode.y + colToMove[i]);
                    child.calculateCost(); 

                    if ((std::find(visitedNodes.begin(), visitedNodes.end(), child) == visitedNodes.end())) {
                        nodesStack.push_back(child);
                        visitedNodes.push_back(child);
                    }
                }
            }
        }

        MPI_Allreduce(&foundSolution, &receivedSolution, 1, MPI_INT, MPI_MAX, world);
        if (receivedSolution) {
            break;
        }

        IterationCounter = IterationsPerCheck;

        if (currentId == MASTER) {
            for (int i=0; i<numprocs-1; i++) {
                MPI_Recv(&request, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, world, &status);
                if (request) {
                    nodesStack.front().serialize(data);
                    nodesStack.erase(nodesStack.begin());
                    MPI_Send(data, DATASIZE, MPI_INT, status.MPI_SOURCE, REPLY, world);
                }
            }
        } else {
            if (nodesStack.empty()) {
              request = 1;
              MPI_Send(&request, 1, MPI_INT, MASTER, REQUEST, world);
              MPI_Recv(data, DATASIZE, MPI_INT, MASTER, REPLY, world, &status);
              
              Node newNode = Node(initial, 0, 0);
              newNode.deserialize(data);
              nodesStack.push_back(newNode);
              visitedNodes.push_back(newNode);
            } else {
              request = 0;
              MPI_Send( &request, 1, MPI_INT, MASTER, REQUEST, world );
            }
        }
    }

    auto endTimer = std::chrono::steady_clock::now();
    if (currentId == MASTER) {
        std::ofstream file;
        file.open("output.txt");
        file << solutionPath.c_str() << std::endl;
        file << "Solution found in: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTimer - startTimer).count() / 1000.0f <<" seconds\n"; 
        file.close();
    }
}

int main(int argc, char **argv) {

    MPI_Init( &argc, &argv );
    
    std::array<std::array<int, N>, N> initial = getInputArray("../input.txt");
    solve(initial);
    
    MPI_Finalize();
 
    return 0;
}