// FPS_Shooter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <Windows.h>

using namespace std;


const float  PI_F = 3.14159265358979f;
const int nScreenWidth = 120;
const int nScreenHeight = 40;

// Player position and Nagle he is looking at in the world
float fPlayerX = 1.0f;
float fPlayerY = 1.0f;
float fPlayerAngle = 0;

// Player's field of view is fixed
const float fFOV = PI_F / 4.0;
const float fDepth = 16;

// Map of the world (2D array)
//  '#' represents outside the track, and '.' represents the track
int nMapHeight = 16;
int nMapWidth = 16;

int main()
{
	// Cout is too slow, so we need to use the console buffer directly
	wchar_t *screen = new wchar_t[nScreenHeight * nScreenWidth];

	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	std::wstring map;


	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#...########...#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto time1 = chrono::system_clock::now();
	auto time2 = chrono::system_clock::now();

	// Game Loop
	while (1)
	{
		time2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = time2 - time1;
		time1 = time2;
		float fElapsedTime = elapsedTime.count();

		// Controls for player movement
		// Counter clockwise rotation
		// In order to give a consistent movement experience - we use the Frame Rate to change the angles
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerAngle -= 2.0f * fElapsedTime;

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerAngle += 2.0f * fElapsedTime;

		if (fPlayerAngle > PI_F)
			fPlayerAngle -= 2.0f * PI_F;
		else if ((fPlayerAngle < -1.0f * PI_F))
			fPlayerAngle += 2.0f * PI_F;

		// Basic Geometry to figure out where new X and Y position are
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			float tmp_fPlayerX = fPlayerX;
			float tmp_fPlayerY = fPlayerY;

			//fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			//fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerX += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY += sinf(fPlayerAngle) * 5.0f * fElapsedTime;

			//Player can advance only if there is no wall (collision detection)
			// So if updated position is in a wall - undo the changes
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX = tmp_fPlayerX;
				fPlayerY = tmp_fPlayerY;
			}
		}

		// Basic Geometry to figure out where new X and Y position are
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			float tmp_fPlayerX = fPlayerX;
			float tmp_fPlayerY = fPlayerY;

			//fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			//fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;

			fPlayerX -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;

			//Player can advance only if there is no wall (collision detection)
			// So if updated position is in a wall - undo the changes
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX = tmp_fPlayerX;
				fPlayerY = tmp_fPlayerY;
			}
		}


		// Need to cast a ray for each pixel on the screen (120)
		for (int x = 0; x < nScreenWidth; x++)
		{

			// For each column (pixel) - calculate the projected ray angle into the world
			//Basically we shoot rays in the range [-fFOV/2 to +fFOV/2] with the fPlayerAngle as the reference point
			float fRayAngle = fPlayerAngle - (fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fDistanceToWall = 0.0f;
			bool bHitWall = false;
			bool bBoundary = false;

			// Unit vector in the direction of the Ray in player space
			//float fEyeX = sinf(fRayAngle);
			//float fEyeY = cosf(fRayAngle);

			float fEyeX = cosf(fRayAngle);
			float fEyeY = sinf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += 0.1f;

				// Move the unit vector to the actual position of the player + incremental ray distance
				// Using this, we can check if we have hit a wall or not
				// We use int => walls start at integer boundaries
				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Check if ray is out of bounds (wrt the map)
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else {
					// Check if we have hit the wall
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						// if we differentiate the Corner vs Other pixels in a wall
						// The rendering looks much better - to do that
						// Cast rays from the existing corners and check if any our rays line up with them
						// If so they are corner pixels

						// Store distance and dot product as a pair
						vector<pair<float, float>> distanceDotPair;

						// Cast a ray from each edge
						for (int tx = 0; tx < 2; tx++)
						{
							for (int ty = 0; ty < 2; ty++)
							{
								// Remember, walls occur at integer values, and nTestX/Y are ints
								// so +1 in X and Y dir are the corners of the wall
								float vx = (float)nTestX + tx - fPlayerX;
								float vy = (float)nTestY + ty - fPlayerY;

								// Magnitude of the vector from each edge
								float magnitude = sqrt(vx * vx + vy * vy);

								// Dot product of unit vector vectors
								float dot = (fEyeX * vx / magnitude) + (fEyeY * vy / magnitude);

								distanceDotPair.push_back(make_pair(magnitude, dot));
							}
						}

						// Now the idea is to sort all the the pairs based on dot product value
						// Note : greater dot product => closer to 0 => identical dir => corner/edge
						sort(distanceDotPair.begin(), distanceDotPair.end(),
							[](const pair<float, float> &left, const pair<float, float> &right)
						{
							return left.first < right.first;
						}
						);

						// Choose a small enough angle so that it catches only edges
						float fBound = 0.01;
						if (acos(distanceDotPair.at(0).second) < fBound) bBoundary = true;
						if (acos(distanceDotPair.at(1).second) < fBound) bBoundary = true;
						if (acos(distanceDotPair.at(2).second) < fBound) bBoundary = true;

					}
				}

				// Calculate distance to ceiling and floor
				// A perspective of distance is formed by distance to ceiling and floor as well
				// This gives the illusion of depth

				int nCeiling = (float)(nScreenHeight / 2) - nScreenHeight / (float)fDistanceToWall;
				int nFloor = nScreenHeight - nCeiling;

				short nWallShade;
				short nFloorShade;

				// Color to shade Wall based on distance
				if (fDistanceToWall <= fDepth / 4.0f)		nWallShade = 0x2588; // Very close
				else if (fDistanceToWall <= fDepth / 3.0f)	nWallShade = 0x2593;
				else if (fDistanceToWall <= fDepth / 2.0f)	nWallShade = 0x2592;
				else if (fDistanceToWall <= fDepth)			nWallShade = 0x2591;
				else										nWallShade = '^';	 // Very far away			

				if (bBoundary) nWallShade = ' '; // Black out the edge pixels

				// We are ready to draw on the screen
				for (int y = 0; y < nScreenHeight; y++)
				{
					if (y < nCeiling)
						screen[y*nScreenWidth + x] = ' ';
					else if (y > nCeiling && y <= nFloor)
						screen[y*nScreenWidth + x] = nWallShade;
					else
					{
						// Color to shade the floor based on distance
						float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));

						if (b < 0.25)		nFloorShade = '#';
						else if (b < 0.5)	nFloorShade = 'x';
						else if (b < 0.75)	nFloorShade = '.';
						else if (b < 0.9)	nFloorShade = '-';
						else				nFloorShade = ' ';

						screen[y*nScreenWidth + x] = nFloorShade;
					}
				}
			}
		}

		// Display Stats
		swprintf_s(screen, 45, L"X=%3.2f, Y=%3.2f, Angle=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerAngle, 1.0f / fElapsedTime);

		// Dsiplay the Map for simplicity
		for (int nx = 0; nx < nMapWidth; nx++)
			for (int ny = 0; ny < nMapHeight; ny++)
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];

		short playerSymBol = ' ';
		if ((fPlayerAngle > -1.0f * PI_F / 4.0f) && (fPlayerAngle <= PI_F / 4.0f))					playerSymBol = 0x2192;
		else if ((fPlayerAngle > PI_F / 4.0f) && (fPlayerAngle <= 3.0f * PI_F / 4.0f))				playerSymBol = 0x2193;
		else if ((fPlayerAngle > 3.0f * PI_F / 4.0f) || (fPlayerAngle <= -3.0f * PI_F / 4.0f))		playerSymBol = 0x2190;
		else if ((fPlayerAngle > -3.0f * PI_F / 4.0f) && (fPlayerAngle <= -1.0f * PI_F / 4.0f))		playerSymBol = 0x2191;
		else	playerSymBol = 'P';

		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = playerSymBol;

		// Sample Writing to screen
		screen[nScreenHeight * nScreenWidth - 1] = '\0';
		int result = WriteConsoleOutputCharacter(hConsole, screen, nScreenHeight * nScreenWidth, { 0,0 }, &dwBytesWritten);

		if (result == 0)
			result += 1;

	}

	return 0;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
