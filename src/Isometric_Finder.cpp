#define OLC_PGE_APPLICATION
#include "../include/olcPixelGameEngine.h"
#include "../include/Algorithm.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <time.h>
#include <thread>

class Isometric : public olc::PixelGameEngine
{
public:
	Isometric()
	{
		sAppName = "Isometric_Finder";
	}

private:
	// Number of tiles in world
	olc::vi2d vWorldSize = { 11, 11 };

	// Size of single tile graphic
	olc::vi2d vTileSize = { 40, 20 };

	// Where to place tile (0,0) on screen (in tile size steps)
	olc::vi2d vOrigin = { 6, 5 };

	// Sprite that holds all imagery
	olc::Sprite *sprIsom = nullptr;

	// Pointer to create 2D world array
	int *pWorld = nullptr;

    // Locker for keyboard / mouse input
    bool signalLock = false;

    bool keyToggled = false;

    Algorithm *al = nullptr;

    olc::vi2d prevMouseHeldPos = { -1, -1 };
    int touchDownTileState;
    int prevTime = 0;
    int srcPos = -1;
    int dstPos = -1;

public:
	bool OnUserCreate() override
	{
		// Load sprites used in demonstration
		sprIsom = new olc::Sprite("../res/isometric_demo.png");

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

        if (!signalLock) {
            // obstacle -- click
            if (GetMouse(0).bPressed) {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) 
                    touchDownTileState = !pWorld[vSelected.y * vWorldSize.x + vSelected.x];
            }

            // obstacle -- held
            if (GetMouse(0).bHeld && (vSelected.x != prevMouseHeldPos.x || vSelected.y != prevMouseHeldPos.y)) {
                if (vSelected.x >= 0 && vSelected.x < vWorldSize.x && vSelected.y >= 0 && vSelected.y < vWorldSize.y) {
                    int curr = vSelected.y * vWorldSize.x + vSelected.x;
                    // erase only the obstacle
                    if (pWorld[curr] <= 1) pWorld[curr] = touchDownTileState;
                    prevMouseHeldPos = vSelected;
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
                        pWorld[srcPos] = 4;
                    }
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
                        pWorld[dstPos] = 5;
                    }
                }
            }

            // [TODO]
            // do the calculation and set the value for the tiles in each iteration
            // Done. better do it in pthread
            if (GetKey(olc::Key::SPACE).bPressed) {
                delete al;
                if (srcPos != -1 && dstPos != -1) {
                    std::cout << "Solving" << std::endl;
                    Solve(1);
                }
            }

            if (GetKey(olc::Key::R).bPressed) {
                std::cout << "Resetting" << std::endl;
                for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) pWorld[i] = 0;
                srcPos = -1; dstPos = -1;
            }

            if (GetKey(olc::Key::T).bPressed) {
                keyToggled = !keyToggled;
                std::cout << "Keybindings toggled" << std::endl;
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
					DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 1 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
					break;
				case 1:
					// Visible Tile
					DrawPartialSprite(vWorld.x, vWorld.y, sprIsom, 2 * vTileSize.x, 0, vTileSize.x, vTileSize.y);
					break;
				case 2:
					// Tree
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 0 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 3:
					// Spooky Tree
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 1 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 4:
					// Beach
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 2 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				case 5:
					// Water
					DrawPartialSprite(vWorld.x, vWorld.y - vTileSize.y, sprIsom, 3 * vTileSize.x, 1 * vTileSize.y, vTileSize.x, vTileSize.y * 2);
					break;
				}
			}
		}

		// Draw Selected Cell - Has varying alpha components
		SetPixelMode(olc::Pixel::ALPHA);

		// Convert selected cell coordinate to world space
		olc::vi2d vSelectedWorld = ToScreen(vSelected.x, vSelected.y);

		// Draw "highlight" tile
		DrawPartialSprite(vSelectedWorld.x, vSelectedWorld.y, sprIsom, 0 * vTileSize.x, 0, vTileSize.x, vTileSize.y);

		// Go back to normal drawing with no expected transparency
		SetPixelMode(olc::Pixel::NORMAL);

		// Draw Hovered Cell Boundary
		//DrawRect(vCell.x * vTileSize.x, vCell.y * vTileSize.y, vTileSize.x, vTileSize.y, olc::RED);
				
		// Draw Debug Info
		DrawString(4, 4,  "Mouse   : " + std::to_string(vMouse.x) + ", " + std::to_string(vMouse.y), olc::BLACK);
		DrawString(4, 14, "Cell    : " + std::to_string(vCell.x) + ", " + std::to_string(vCell.y), olc::BLACK);
		DrawString(4, 24, "Selected: " + std::to_string(vSelected.x) + ", " + std::to_string(vSelected.y), olc::BLACK);
        DrawString(4, 54, "Press T For Keybindings", olc::BLACK);
		return true;
	}

    void Solve(int algorithm)
    {
        signalLock = true;
        for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) {
            if (pWorld[i] == 2) pWorld[i] = 0;
        }
        Algorithm *al = new Algorithm(pWorld, vWorldSize.x * vWorldSize.y, vWorldSize.x, srcPos, dstPos, &signalLock);
        std::cout << "Solving using DFS" << std::endl;
        // Async would be better
        std::thread (&Algorithm::DFS, al).detach();
        /* switch (algorithm) {
            case 0:
                std::cout << "Solving using A*" << std::endl;
                break;
            case 1:
                std::cout << "Solving using DFS" << std::endl;
                std::thread th(&Algorithm::DFS, al);
                break;
            case 2:
                std::cout << "Solving using BFS" << std::endl;
                break;
        } */
        signalLock = false;
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
};


int main()
{
	Isometric demo;
	if (demo.Construct(512, 480, 2, 2))
		demo.Start();
	return 0;
}
