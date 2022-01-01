#define OLC_PGE_APPLICATION
#include "../include/olcPixelGameEngine.h"
#include "../include/Algorithm.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// animation
#include <time.h>
// threading
#include <thread>
#include <future>
// file io
#include <fstream>
// file removal
#include <cstdio>
// random
#include <cstdlib>

class Isometric : public olc::PixelGameEngine
{
public:
	Isometric()
	{
		sAppName = "Isometric_Finder";
	}

private:
	// Number of tiles in world
	// olc::vi2d vWorldSize = { 11, 11 };
    olc::vi2d vWorldSize = {22, 22};

	// Size of single tile graphic
	olc::vi2d vTileSize = { 40, 20 };

	// Where to place tile (0,0) on screen (in tile size steps)
	olc::vi2d vOrigin = { 11, 5 };

	// Sprite that holds all imagery
	olc::Sprite *sprIsom = nullptr;
    olc::Sprite *sprEkko = nullptr;

	// Pointer to create 2D world array
	int *pWorld = nullptr;

    // Locker for keyboard / mouse input
    bool signalLock = false;


    Algorithm *al = nullptr;
    int nsteps = 0;

    struct alRecord {
        const char *recAl;
        int recStep;
    };

    std::vector<alRecord> alRec;
    int alRecSize = 0;
    // A* / DFS / BFS
    int alCount = 3;
    int obstacleCount = 0;

    olc::vi2d prevMouseHeldPos = { -1, -1 };
    int touchDownTileState;
    int prevTime = 0;
    int srcPos = -1;
    int dstPos = -1;

    const char *outFileName = "../scripts/records.txt";
    std::ofstream ofd{outFileName, std::ios::out | std::ios::app};

    bool keyToggled = false;
    bool saveToggle = false;
    bool autoRunToggle = false;
    int alToggle = 0;
public:
	bool OnUserCreate() override
	{
		// Load sprites used in demonstration
		// sprIsom = new olc::Sprite("../res/isometric_demo.png");
		sprIsom = new olc::Sprite("../res/texture.png");
        sprEkko = new olc::Sprite("../res/ekko.png");

		// Create empty world
		pWorld = new int[vWorldSize.x * vWorldSize.y]{ 0 };

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::WHITE);

		// Get Mouse in world
		olc::vi2d vMouse = { GetMouseX(), GetMouseY() };
		
		// Work out active cell
		olc::vi2d vCell = { vMouse.x / vTileSize.x, vMouse.y / vTileSize.y };

		// Work out mouse offset into cell
		olc::vi2d vOffset = { vMouse.x % vTileSize.x, vMouse.y % vTileSize.y };

		// Sample into cell offset colour
		olc::Pixel col = sprIsom->GetPixel(3 * vTileSize.x + vOffset.x, vOffset.y);

		// Work out selected cell by transforming screen cell
		olc::vi2d vSelected = 
		{
			(vCell.y - vOrigin.y) + (vCell.x - vOrigin.x),
			(vCell.y - vOrigin.y) - (vCell.x - vOrigin.x) 
		};

		// "Bodge" selected cell by sampling corners
		if (col == olc::RED) vSelected += {-1, +0};
		if (col == olc::BLUE) vSelected += {+0, -1};
		if (col == olc::GREEN) vSelected += {+0, +1};
		if (col == olc::YELLOW) vSelected += {+1, +0};

        // [TODO]
        // Done. held drawing
        // Done. set toggle for each tile
        if (GetKey(olc::Key::S).bPressed) {
            saveToggle = !saveToggle;
            if (saveToggle) {
                std::cout << "save toggled" << std::endl;
            }
        }
        
        if (GetKey(olc::Key::ESCAPE).bPressed) {
            exitHandler(EXIT_SUCCESS);
        }

        if (signalLock) {
            if (GetKey(olc::Key::P).bPressed) {
                autoRunToggle = !autoRunToggle;
            }
        }

        if (!signalLock) {
            // obstacle -- click
            if (GetMouse(0).bPressed) {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) {
                    touchDownTileState = !pWorld[vSelected.y * vWorldSize.x + vSelected.x];
                    recordReset();
                }
            }

            // obstacle -- held
            if (GetMouse(0).bHeld && (vSelected.x != prevMouseHeldPos.x || vSelected.y != prevMouseHeldPos.y)) {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) {
                    int curr = vSelected.y * vWorldSize.x + vSelected.x;
                    // erase only the obstacle
                    if (pWorld[curr] <= 1) {
                        pWorld[curr] = touchDownTileState;
                    }
                    prevMouseHeldPos = vSelected;
                    recordReset();
                }
            }

            // src
            if (GetMouse(1).bPressed)
            {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) {
                    // toggle
                    if (srcPos == vSelected.y * vWorldSize.x + vSelected.x) {
                        pWorld[srcPos] = !pWorld[srcPos] * 4;
                        if (pWorld[srcPos] == 0) srcPos = -1;
                    } else {
                        if (srcPos != -1) pWorld[srcPos] = 0;
                        srcPos = vSelected.y * vWorldSize.x + vSelected.x;
                        if (dstPos == srcPos) dstPos = -1;
                        pWorld[srcPos] = 4;
                    }
                    clearPath();
                    recordReset();
                }
            }

            // dst
            if (GetMouse(2).bPressed)
            {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) {
                    // toggle
                    if (dstPos == vSelected.y * vWorldSize.x + vSelected.x) {
                        pWorld[dstPos] = !pWorld[dstPos] * 5;
                        if (pWorld[dstPos] == 0) dstPos = -1;
                    } else {
                        if (dstPos != -1) pWorld[dstPos] = 0;
                        dstPos = vSelected.y * vWorldSize.x + vSelected.x;
                        if (srcPos == dstPos) srcPos = -1;
                        pWorld[dstPos] = 5;
                    }
                    clearPath();
                    recordReset();
                }
            }

            // [TODO]
            // do the calculation and set the value for the tiles in each iteration
            // Done. better do it in pthread
            if (GetKey(olc::Key::SPACE).bPressed) {
                delete al;
                if (srcPos != -1 && dstPos != -1) {
                    std::cout << "Solving" << std::endl;
                    if (alToggle == 0)
                        std::thread (&Isometric::runAll, this).detach();
                    else
                        Solve();
                }
            }

            if (GetKey(olc::Key::R).bPressed) {
                delete al;
                std::cout << "Resetting" << std::endl;
                recordReset();
                for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) pWorld[i] = 0;
                srcPos = -1; dstPos = -1;
                nsteps = 0;
            }

            if (GetKey(olc::Key::T).bPressed) {
                keyToggled = !keyToggled;
                std::cout << "Keybindings toggled" << std::endl;
            }


            if (GetKey(olc::Key::P).bPressed) {
                autoRunToggle = !autoRunToggle;
                std::thread (&Isometric::autoRunAll, this).detach();
            }

            // Could be done better
            // hash function
            if (GetKey(olc::Key::K0).bPressed) {
                alToggle = 0;
            }
            if (GetKey(olc::Key::K1).bPressed) {
                alToggle = 1;
            }
            if (GetKey(olc::Key::K2).bPressed) {
                alToggle = 2;
            }
            if (GetKey(olc::Key::K3).bPressed) {
                alToggle = 3;
            }

        }
						
		// Labmda function to convert "world" coordinate into screen space
		auto ToScreen = [&](int x, int y)
		{			
			return olc::vi2d
			{
				(vOrigin.x * vTileSize.x) + (x - y) * (vTileSize.x / 2),
				(vOrigin.y * vTileSize.y) + (x + y) * (vTileSize.y / 2)
			};
		};
		
		// Draw World - has binary transparancy so enable masking
		SetPixelMode(olc::Pixel::MASK);

        if (time(NULL) - prevTime > 0) printMap();

		// (0,0) is at top, defined by vOrigin, so draw from top to bottom
		// to ensure tiles closest to camera are drawn last
        // reset obstacle count every time
        obstacleCount = 0;
		for (int y = 0; y < vWorldSize.y; y++)
		{
			for (int x = 0; x < vWorldSize.x; x++)
			{
				// Convert cell coordinate to world space
				olc::vi2d vWorld = ToScreen(x, y);
				
				switch(pWorld[y*vWorldSize.x + x])
				{
				case 0:
					// Invisble Tile
					/* DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 1 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
					break; */
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 0 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 1:
                    // obstacle
                    // barren land
					/* DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 2 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
					break; */
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 1 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
                    obstacleCount++;
					break;
				case 2:
					// Tree
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 2 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 3:
                    // Forward Path
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 2 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
                    // DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprEkko, 3 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 4:
					// Beach
					/* DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 2 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break; */
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 3 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
                    // DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprEkko, 0 * vTileSize.x, 0, vTileSize.x, vTileSize.y * 2);
					break;
				case 5:
					// Water
					/* DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 3 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break; */
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 4 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
                    // DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprEkko, 1 * vTileSize.x, 0, vTileSize.x, vTileSize.y * 2);
					break;
                case 6:
                    // Rewind Path
                    DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprEkko, 6 * vTileSize.x, 0, vTileSize.x, vTileSize.y * 2);
					break;
				}
			}
		}

		// Draw Selected Cell - Has varying alpha components
		SetPixelMode(olc::Pixel::ALPHA);

		// Convert selected cell coordinate to world space
		olc::vi2d vSelectedWorld = ToScreen(vSelected.x, vSelected.y);

		// Draw "highlight" tile
		// DrawPartialSprite(vSelectedWorld.x, vSelectedWorld.y, sprIsom, 0 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
		DrawPartialSprite(vSelectedWorld.x, vSelectedWorld.y - vTileSize.y, sprIsom, 5 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);

		// Go back to normal drawing with no expected transparency
		SetPixelMode(olc::Pixel::NORMAL);

		// Draw Hovered Cell Boundary
		//DrawRect(vCell.x * vTileSize.x, vCell.y * vTileSize.y, vTileSize.x, vTileSize.y, olc::RED);
				
		// Draw Debug Info
		DrawString(4, 4,  "Mouse   : " + std::to_string(vMouse.x) + ", " + std::to_string(vMouse.y), olc::BLACK);
		DrawString(4, 14, "Cell    : " + std::to_string(vCell.x) + ", " + std::to_string(vCell.y), olc::BLACK);
		DrawString(4, 24, "Selected: " + std::to_string(vSelected.x) + ", " + std::to_string(vSelected.y), olc::BLACK);
        DrawString(4, 54, "Finished in " + std::to_string(nsteps) + " steps", olc::BLACK);

        DrawString(4, 64, "Algorithm: ", olc::BLACK);
        DrawString(94, 64, alToggle == 0 ? "*"   :
                           alToggle == 1 ? "A*"  :
                           alToggle == 2 ? "DFS" :
                           alToggle == 3 ? "BFS" : "", olc::BLACK);
        DrawString(4, 84, "Previous run:", olc::BLACK);
        if (alRecSize)
            alRec[alRecSize-1].recStep = nsteps;
        for (int i = 0; i < alRecSize; i++) {
            int curr = alRecSize - i - 1;
            DrawString(44, i*10+94, std::to_string(i+1) + ": " + alRec[curr].recAl + " -- " + std::to_string((alRec[curr].recStep)), olc::BLACK);
        }
        DrawString(4, 134, saveToggle ? "Save: Toggled" : "Save: Untoggled", olc::BLACK);
        DrawString(4, 144, "Obstacle Count: " + std::to_string(obstacleCount), olc::BLACK);
        // DrawString(304, 4, "Press T For Keybindings", olc::BLACK);
		return true;
	}

    void Solve()
    {
        Algorithm *al = new Algorithm(pWorld, vWorldSize.x * vWorldSize.y, vWorldSize.x, srcPos, dstPos, &signalLock, &nsteps);
        alRecord tmpRec;
        switch (alToggle) {
            case 1:
                tmpRec.recAl = "A*";
                break;
            case 2:
                tmpRec.recAl = "DFS";
                break;
            case 3:
                tmpRec.recAl = "BFS";
                break;
            default:
                break;
        }
        while (alRecSize > alCount - 1) {
            alRec.erase(alRec.begin());
            alRecSize -= 1;
        }
        alRec.push_back(tmpRec);
        alRecSize += 1;
        // Async would be better
        /* std::cout << "Solving using DFS" << std::endl;
        std::thread (&Algorithm::DFS, al).detach(); */
        /* std::cout << "Solving using BFS" << std::endl;
        std::thread (&Algorithm::BFS, al).detach(); */
        /* std::cout << "Solving using A*" << std::endl;
        std::thread (&Algorithm::A_Star, al).detach(); */
        // std::async (&Algorithm::A_Star, al);

        clearPath();
        switch (alToggle) {
            case 0:
                std::cout << "Cycle through all algorithms"<< std::endl;
                break;
            case 1:
                std::cout << "Solving by A*"<< std::endl;
                std::thread (&Algorithm::A_Star, al).detach();
                break;
            case 2:
                std::cout << "Solving by DFS" << std::endl;
                std::thread (&Algorithm::DFS, al).detach();
                break;
            case 3:
                std::cout << "Solving using BFS" << std::endl;
                std::thread (&Algorithm::BFS, al).detach();
                break;
            default:
                break;
        }
    }

    void runAll()
    {
        // cycle through all algorithms
        for (int i = 1; i <= alCount; i++) {
            if (!signalLock) {
                alToggle = i;
                Solve();
                // lock immediately in case of race condition
                signalLock = true;
            }
            while (signalLock);
        }
        alToggle = 0;
    }

    void clearPath()
    {
        std::cout << "Clearing Path" << std::endl;
        for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) {
            if (pWorld[i] == 2 || pWorld[i] == 6) {
                pWorld[i] = 0;
            }
        }
    }

    void printMap()
    {
		for (int y = 0; y < vWorldSize.y; y++)
		{
			for (int x = 0; x < vWorldSize.x; x++)
			{
                std::cout << pWorld[y*vWorldSize.x+x] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        prevTime = time(NULL);
    }

    void recordReset()
    {
        alRec.clear();
        alRecSize = 0;
    }

    void exitHandler(int state)
    {
        std::ifstream ifd{outFileName};
        // remove output file if it is empty
        if (ifd.peek() == std::ifstream::traits_type::eof()) {
            if (remove(outFileName) != 0) {
                perror("Failed writing to file.");
            }
        }
        exit(state);
    }

    void autoRunAll()
    {
        int roundCnt = 0;
        if (srcPos != -1 && dstPos != -1) {
            pWorld[srcPos] = 0;
            pWorld[dstPos] = 0;
        }
        while (true) {
            std::cout << "starting a new round" << std::endl;
            if (roundCnt % 10 == 0) srand(time(NULL));
            do {
                std::cout << "setting up src / dst" << std::endl;
                srcPos = rand() % (vWorldSize.x * vWorldSize.y);
                dstPos = rand() % (vWorldSize.x * vWorldSize.y);
            } while (srcPos == dstPos || pWorld[srcPos] != 0 || pWorld[dstPos] != 0);
            pWorld[srcPos] = 4;
            pWorld[dstPos] = 5;
            std::thread th(&Isometric::runAll, this);
            th.join();
            if (saveToggle) {
                for (int i = 0; i < alCount; i++) {
                    if (i) {
                        ofd << ",";
                    }
                    ofd << alRec[i].recStep;
                }
                ofd << std::endl;
            }
            usleep(50000);
            if (!autoRunToggle) break;
            pWorld[srcPos] = 0;
            pWorld[dstPos] = 0;
            roundCnt++;
        }
    }
};


int main()
{
	Isometric demo;
    srand(time(NULL));
	/* if (demo.Construct(512, 480, 2, 2))
		demo.Start(); */
    /* if (demo.Construct(720, 480, 2, 2))
        demo.Start(); */
    if (demo.Construct(920, 540, 2, 2))
        demo.Start();
	return 0;
}
