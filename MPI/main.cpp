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
#include "node.h"
#include "mpi.h"

#define MASTER    0
#define REQUEST   1
#define REPLY     2
#define DATASIZE 13

std::vector<Node> visitedNodes;

int row[] = { 1, 0, -1, 0 };
int col[] = { 0, -1, 0, 1 };
 
void solve(const std::array<std::array<int, N>, N>& initial)
{
    int numprocs, myid, request = 1;
    int data[DATASIZE];
    MPI_Status status;
    std::string solutionPath;
    auto start = std::chrono::steady_clock::now();

    // 1. Initialize MPI
    MPI_Comm world = MPI_COMM_WORLD;
    MPI_Comm_size( world, &numprocs );
    MPI_Comm_rank( world, &myid );

    std::vector<Node> nodesStack;
 
    // 2. Set first node
    if (myid == MASTER) 
    {
        auto [x, y] = getBlankPosition(initial);
        Node root = Node(initial, x, y);
        root.calculateCost();
        nodesStack.push_back(root);
        visitedNodes.push_back(root);
    }

    int foundSolution = 0, receivedSolution = 0;
    const int workPerCheck = 1000;
    int workCounter = workPerCheck;
 
    while (!foundSolution)
    {
        // 3. Do some work
        while (!nodesStack.empty() && workCounter)
        {
            workCounter--;
            Node min = nodesStack.back();
    
            nodesStack.pop_back();
            if (visitedNodes.size()%1000 == 0) printf("Proc %u: visited nodes = %lu\n", myid, visitedNodes.size());
    
            if (min.cost == 0)
            {
                printf("Found!\n");
                solutionPath = min.path();
                foundSolution = myid + 1;
                min.printMatrix();
                break;
            }
 
            for (int i = 0; i < 4; i++)
            {
                if (isSafe(min.x + row[i], min.y + col[i]))
                {        
                    Node child = min.copy();
                    child.swapBlank(min.x, min.y, min.x + row[i], min.y + col[i]);
                    child.calculateCost(); 

                    if ((std::find(visitedNodes.begin(), visitedNodes.end(), child) == visitedNodes.end()))
                    {
                        nodesStack.push_back(child);
                        visitedNodes.push_back(child);
                    }
                }
            }
        }

        //4. After some work, check if solution was found by MPI_Allreduce
        MPI_Allreduce( &foundSolution, &receivedSolution, 1, MPI_INT, MPI_MAX, world);
        if (receivedSolution != 0)
        {
            break;
        }

        workCounter = workPerCheck;

        //5. Handle job requests by MPI_Send and MPI_Recv
        if (myid == MASTER)
        {
            for (int i=0; i<numprocs-1; i++)
            {
                MPI_Recv( &request, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, world, &status );
                if (request)
                {
                    //Send data to workers
                    nodesStack.front().serialize(data);
                    nodesStack.erase(nodesStack.begin());
                    MPI_Send( data, DATASIZE, MPI_INT, status.MPI_SOURCE, REPLY, world );
                }
            }
        }
        else
        {
            if (nodesStack.empty())
            {
              request = 1;
              MPI_Send( &request, 1, MPI_INT, MASTER, REQUEST, world );
              MPI_Recv( data, DATASIZE, MPI_INT, MASTER, REPLY, world, &status );
              
              //Add new node to stack
              Node newNode = Node(initial, 0, 0);
              newNode.deserialize(data);
              nodesStack.push_back(newNode);
              visitedNodes.push_back(newNode);
            }
            else
            {
              request = 0;
              MPI_Send( &request, 1, MPI_INT, MASTER, REQUEST, world );
            }
        }
    }

    //6. Save solution in output.txt
    auto end = std::chrono::steady_clock::now();
    if (myid == MASTER)
    {
        std::ofstream file;
        file.open("output.txt");
        file << solutionPath << std::endl;
        file << "Solution found in: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() <<" seconds\n"; 
        file.close();
    }
}
 
int main(int argc, char **argv)
{
    MPI_Init( &argc, &argv );
    
    std::array<std::array<int, N>, N> initial = getInputArray("input.txt");
    solve(initial);
    
    MPI_Finalize();
 
    return 0;
}