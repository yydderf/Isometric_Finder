#pragma once

class Algorithm {
public:
    Algorithm(int *matrix, int size, int xlen, int src, int dst, bool *signalLock);
    ~Algorithm();
    void DFS();
    void BFS();
    void A_Star();
    bool isValidDFS(bool*, int);
private:
    int *pWorld = nullptr;
    int pWorldSize;
    int xLen;
    int Src;
    int Dst;
    bool *lock;
};
