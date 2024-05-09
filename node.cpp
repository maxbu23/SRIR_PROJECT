#include "node.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

int isSafe(int x, int y) {
    return (x >= 0 && x < N && y >= 0 && y < N);
}

std::array<std::array<int, N>, N> getInputArray(const std::string& filename) {
    std::ifstream file(filename, std::ios::in);
    std::string line;
    std::array<std::array<int, N>, N> initial{};

    int i = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int j = 0;
        int value;
        while (iss >> value) {
            initial[i][j++] = value;
        }
        i++;
    }

    return initial;
}

void print2dArray(const std::array<std::array<int, N>, N>& arr) {
    for (const auto &row : arr) {
        for (const auto &elem : row) {
            std::cout << elem << ' ';
        }
        std::cout << '\n';
    }
}

std::pair<int, int> getBlankPosition(const std::array<std::array<int, N>, N>& board) {
    for (size_t i = 0; i < board.size(); i++)
        for (size_t j = 0; j < board[i].size(); j++)
            if (!board[i][j])
                return std::make_pair(i, j);
                
    return std::make_pair(-1, -1);
}