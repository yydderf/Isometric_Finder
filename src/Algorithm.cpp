#include <iostream>
#include <vector>
#include <list>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <cstdlib>

#include "../include/Algorithm.h"

#define PATH_USLEEP 5000

Algorithm::Algorithm(int *matrix, int size, int xlen, int src, int dst, bool *signalLock, int *nsteps)
{
    pWorld = matrix;
    pWorldSize = size;
    xLen = xlen;
    Src = src;
    Dst = dst;
    lock = signalLock;
    steps = nsteps;
}

Algorithm::~Algorithm()
{
    std::cout << "Memory Released." << std::endl;
}

void
Algorithm::DFS()
{
    *lock = true;
    *steps = 0;
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
        usleep(PATH_USLEEP);
        *steps += 1;
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
    *lock = false;
}

void
Algorithm::BFS()
{
    *lock = true;
    *steps = 0;
    bool *vis = new bool[pWorldSize];
    std::fill(vis, vis+pWorldSize, false);

    std::vector<int> v;
    v.push_back(Src);
    while (!v.empty()) {
        int curr = v.front();
        v.erase(v.begin());
        if (!this->isValidDFS(vis, curr)) continue;
        vis[curr] = true;
        usleep(PATH_USLEEP);
        *steps += 1;
        if (curr == Dst) break;
        // keep the texture of Src
        if (curr != Src) pWorld[curr] = 2;

        if ((curr - xLen) >= 0)         v.push_back(curr - xLen);
        if ((curr) % xLen != 0)         v.push_back(curr - 1);
        if ((curr + xLen) < pWorldSize) v.push_back(curr + xLen);
        if ((curr + 1) % xLen != 0)     v.push_back(curr + 1);
    }

    *lock = false;
}

bool
Algorithm::isValidDFS(bool *vis, int ind)
{
    // check if is obstacle || visited 
    if (pWorld[ind] == 1) return false;
    if (vis[ind]) return false;
    return true;
}

void
Algorithm::A_Star()
{
    *lock = true;
    *steps = 0;

    bool *vis = new bool[pWorldSize];
    std::fill(vis, vis+pWorldSize, false);

    nodes = new sNode[pWorldSize];
    nodeStart = &nodes[Src];
    nodeEnd = &nodes[Dst];

    for (int y = 0 ; y < pWorldSize / xLen; y++) {
        for (int x = 0; x < xLen; x++) {
            int curr = y * xLen + x;
            nodes[curr].x = x;
            nodes[curr].y = y;
            nodes[curr].ind = curr;
            nodes[curr].parent = nullptr;

            // create connections to adjacent nodes;
            if (curr >= xLen)               nodes[curr].vecNeighbors.push_back(&nodes[curr-xLen]);
            if (curr %  xLen)               nodes[curr].vecNeighbors.push_back(&nodes[curr-1]);
            if (curr <  pWorldSize - xLen)  nodes[curr].vecNeighbors.push_back(&nodes[curr+xLen]);
            if ((curr + 1) % xLen)          nodes[curr].vecNeighbors.push_back(&nodes[curr+1]);
        }
    }

    auto
    distance = [](sNode *a, sNode *b)
    {
        return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
    };

    auto
    heuristic = [distance](sNode *a, sNode *b)
    {
        return distance(a, b);
    };

    for (int x = 0; x < xLen; x++) {
        for (int y = 0; y < xLen; y++) {
            nodes[y * xLen + x].fGlobalGoal = INFINITY;
            nodes[y * xLen + x].fLocalGoal = INFINITY;
            nodes[y * xLen + x].parent = nullptr;
        }
    }

    sNode *nodeCurr = nodeStart;
    nodeStart->fLocalGoal = 0.0f;
    nodeStart->fGlobalGoal = heuristic(nodeStart, nodeEnd);

    std::list<sNode*> listTBD;
    listTBD.push_back(nodeStart);

    while (!listTBD.empty()) {
        listTBD.sort([](const sNode *lhs, const sNode *rhs){ return lhs->fGlobalGoal < rhs->fGlobalGoal; } );

        while (!listTBD.empty() && vis[listTBD.front()->ind])
            listTBD.pop_front();
        if (listTBD.empty())
            break;

        nodeCurr = listTBD.front();
        vis[nodeCurr->ind] = true;
        usleep(PATH_USLEEP);
        *steps += 1;
        if (nodeCurr->ind == Dst)
            break;
        if (nodeCurr->ind != Src)
            pWorld[nodeCurr->ind] = 2;

        for (auto nodeNeighbor: nodeCurr->vecNeighbors) {
            if (!vis[nodeNeighbor->ind] && pWorld[nodeNeighbor->ind] != 1)
                listTBD.push_back(nodeNeighbor);

            // calculate local goal
            float possibleLower = nodeCurr->fLocalGoal + distance(nodeCurr, nodeNeighbor);
            if (possibleLower < nodeNeighbor->fLocalGoal) {
                nodeNeighbor->parent = nodeCurr;
                nodeNeighbor->fLocalGoal = possibleLower;
                nodeNeighbor->fGlobalGoal = nodeNeighbor->fLocalGoal + heuristic(nodeNeighbor, nodeEnd);
            }
        }
    }

    // [TODO]
    // migrate the rewind to Isometric
    if (nodeEnd != nullptr) {
        int offset = 0;
        sNode *ptr = nodeEnd;
        if (ptr->parent != nullptr) ptr = ptr->parent;
        while (ptr->parent != nullptr) {
            usleep(PATH_USLEEP);
            pWorld[ptr->ind] = 6;
            ptr = ptr->parent;
        }
    }
    *lock = false;
}

// [TODO]
void
Algorithm::ObstacleGen()
{
    int clusterSrc;
    int clusterSize;
    int clusterNum = rand() % CLUSTER_NUM;
    while (clusterNum--) {
        clusterSize = rand() % CLUSTER_SIZE;
        clusterSrc = rand() % pWorldSize;
    }
}
