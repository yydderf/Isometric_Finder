#pragma once
#include <vector>

#define CLUSTER_NUM 10
#define CLUSTER_SIZE 10

class Algorithm {
public:
    Algorithm(int *matrix, int size, int xlen, int src, int dst, bool *signalLock, int *nsteps);
    ~Algorithm();
    void DFS();
    void BFS();
    void A_Star();
    bool isValidDFS(bool*, int);
    // [TODO]
    void ObstacleGen();
private:
    int *pWorld = nullptr;
    int pWorldSize;
    int xLen;
    int Src;
    int Dst;

    struct sNode
    {
        int x;
        int y;
        int ind;
        float fGlobalGoal;
        float fLocalGoal;
        std::vector<sNode*> vecNeighbors;
        sNode* parent;
    };

    sNode *nodes = nullptr;
    sNode *nodeStart = nullptr;
    sNode *nodeEnd = nullptr;

    bool *lock;
    int *steps;
};

