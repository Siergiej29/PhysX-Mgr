#pragma once

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>

#include <vector>
using namespace std;


struct IntV3
{
	int x, y, z;
};

struct HeightMapInfo{		// Heightmap structure
	int terrainWidth;		// Width of heightmap
	int terrainHeight;		// Height (Length) of heightmap
	IntV3 *heightMap;	// Array to store terrain's vertex positions
};

struct HeightFieldVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR2 texCoord;
    D3DXVECTOR3 normal;
    D3DXVECTOR3 tangent;
    D3DXVECTOR3 bitangent;
};

class Terrain
{
public:
	Terrain(void);
	~Terrain(void);
	bool HeightMapLoad(char* filename, float sx, float sz, float maxy);

	int NumFaces;
	int NumVertices;
	HeightMapInfo hminfo;
	struct HeightFieldVertex *v;
	DWORD *indices;
	float dx, dy, dz; //skala

	float MinX, MinY, MinZ, MaxX, MaxY, MaxZ;

};

