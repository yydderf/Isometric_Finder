#include <iostream>
#include <vector>
#include <unistd.h>

#include "../include/Algorithm.h"

Algorithm::Algorithm(int *matrix, int size, int xlen, int src, int dst)
{
    pWorld = matrix;
    pWorldSize = size;
    xLen = xlen;
    Src = src;
    Dst = dst;
}

Algorithm::~Algorithm()
{
    std::cout << "Memory Released." << std::endl;
}

void
Algorithm::DFS()
{
    bool *vis = new bool[pWorldSize];
    std::fill(vis, vis+pWorldSize, false);

    std::vector<int> v;
    v.push_back(Src);
    while (!v.empty()) {
        int curr = v.back();
        v.pop_back();

        if (!this->isValidDFS(vis, curr))
            continue;

        vis[curr] = true;
        usleep(50000);
        // std::cout << pWorld[curr] << std::endl;
        // change the tile
        // std::cout << curr << std::endl;
        if (curr == Dst) break;
        if (curr != Src) pWorld[curr] = 2;

        // push all the adjacent cells && boundary check
        if ((curr - xLen) >= 0)         v.push_back(curr - xLen);
        if ((curr) % xLen != 0)         v.push_back(curr - 1);
        if ((curr + xLen) < pWorldSize) v.push_back(curr + xLen);
        if ((curr + 1) % xLen != 0)     v.push_back(curr + 1);
    }
}

bool
Algorithm::isValidDFS(bool *vis, int ind)
{
    // check if out of bound || obstacle 
    if (pWorld[ind] == 1) return false;
    if (vis[ind]) return false;
    return true;
}

void
Algorithm::BFS()
{
}

void
Algorithm::A_Star()
{
}
