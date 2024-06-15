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
#include <upcxx/upcxx.hpp>

#define MASTER 0
#define REQUEST 1
#define REPLY 2
#define DATASIZE 13

std::vector<Node> visitedNodes;
std::vector<Node> nodesStack;

upcxx::global_ptr<Node> all_nodes_ptr = nullptr;

int row[] = {1, 0, -1, 0};
int col[] = {0, -1, 0, 1};

std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;

void initMaster(const std::array<std::array<int, N_ARR>, N_ARR> &initial) {
    auto [x, y] = getBlankPosition(initial);
    Node root = Node(initial, x, y);
    root.calculateCost();
    nodesStack.push_back(root);
    visitedNodes.push_back(root);
}

void distributeInitialTasksToWorkers(std::vector<Node>& nodesStack, int numprocs) {
    all_nodes_ptr = upcxx::new_array<Node>(numprocs);
    for (int i = 0; i < numprocs; ++i) {
        upcxx::global_ptr<Node> proc_node_ptr = all_nodes_ptr + i;
        Node nodeToSend = nodesStack.front();
        nodesStack.erase(nodesStack.begin());
        upcxx::rput(nodeToSend, proc_node_ptr).wait();
    }
}

void receiveInitialTaskFromMaster(std::vector<Node>& nodesStack, int myid) {
    Node newNode = upcxx::rget(all_nodes_ptr + myid).wait();

    // to oznacza, że node jest już przetworzony i nie trzeba go już przetwarzać
    newNode.isProcessed = true;
    upcxx::rput(newNode, all_nodes_ptr + myid).wait();
    nodesStack.push_back(newNode);
    visitedNodes.push_back(newNode);
}

void distributeTasksToWorkers(int numprocs) {
    for (int i = 0; i < numprocs; i++) {
        upcxx::global_ptr<Node> proc_node_ptr = all_nodes_ptr + i;
        Node node = upcxx::rget(proc_node_ptr).wait();

        if (node.isProcessed) {
            Node nodeToSend = nodesStack.front();
            nodesStack.erase(nodesStack.begin());
            upcxx::rput(nodeToSend, proc_node_ptr).wait();
        }
    }
}

void receiveTaskFromMaster(int myid) {
    Node newNode = upcxx::rget(all_nodes_ptr + myid).wait();
    newNode.isProcessed = true;
    upcxx::rput(newNode, all_nodes_ptr + myid).wait();
    nodesStack.push_back(newNode);
    visitedNodes.push_back(newNode);
}

void movePuzzles(Node min) {
    for (int i = 0; i < 4; i++){
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

void solve(const std::array<std::array<int, N_ARR>, N_ARR> &initial)
{
    int numprocs, myid, request = 1;
    std::string solutionPath;
    start = std::chrono::steady_clock::now();

    numprocs = upcxx::rank_n();
    myid = upcxx::rank_me();

    if (myid == MASTER)
    {
        initMaster(initial);
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
            if (visitedNodes.size() % 1000 == 0)
                printf("Proc %u: visited nodes = %lu\n", myid, visitedNodes.size());

            if (min.cost == 0)
            {
                printf("Found!\n");
                solutionPath = min.path();
                foundSolution = myid + 1;
                break;
            }

            movePuzzles(min);
        }
        upcxx::barrier();

        receivedSolution = upcxx::reduce_all(foundSolution, upcxx::op_fast_add).wait();
        if (receivedSolution != 0)
        {
            break;
        }

        workCounter = workPerCheck;

        if (!all_nodes_ptr) {
            if (myid == MASTER) distributeInitialTasksToWorkers(nodesStack, numprocs);
            
            all_nodes_ptr = upcxx::broadcast(all_nodes_ptr, 0).wait();
            upcxx::barrier();

            if (myid != MASTER) {
                if (nodesStack.empty()) receiveInitialTaskFromMaster(nodesStack, myid);
            }

        } else {
            if (myid == MASTER) {
                distributeTasksToWorkers(numprocs);
            }

            all_nodes_ptr = upcxx::broadcast(all_nodes_ptr, 0).wait();

            upcxx::barrier();

            if (myid != MASTER) {
                if (nodesStack.empty()) receiveTaskFromMaster(myid);
            }
        }
    }

    end = std::chrono::steady_clock::now();
    if (myid == MASTER)
    {
        upcxx::delete_array(all_nodes_ptr);
    }
    if (myid == foundSolution - 1)
    {
        std::cout << "Saving solution...\n";
        std::ofstream file;
        file.open("output.txt");
        file << solutionPath << std::endl;
        file << "Solution found in: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds\n";
        file.close();
    }
}

int main(int argc, char **argv)
{
    upcxx::init();

    std::array<std::array<int, N_ARR>, N_ARR> initial = getInputArray("input.txt");
    solve(initial);

    upcxx::finalize();

    return 0;
}
