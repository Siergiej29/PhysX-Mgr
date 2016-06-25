#pragma once
#include <PxPhysicsAPI.h> //Single header file to include all features of PhysX API 

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include "resource.h"
#include <time.h>
#include <string>

#include "Camera.h" 
#include "Object.h"
#include "Terrain.h"

//-------Loading PhysX libraries (_x64 lub _x86)----------//
#ifdef _DEBUG //If in 'Debug' load libraries for debug mode 
#pragma comment(lib, "PhysX3DEBUG_x64.lib")				//Always be needed  
#pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")		//Always be needed
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")		//PhysX extended library 
#pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib") //For PVD only 

#else //Else load libraries for 'Release' mode
#pragma comment(lib, "PhysX3_x64.lib")	
#pragma comment(lib, "PhysX3Common_x64.lib") 
#pragma comment(lib, "PhysX3Extensions.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDK.lib")
#endif

using namespace std;
using namespace physx;

class HeightField
{
public:
	HeightField(Terrain terrain, PxPhysics& sdk, PxMaterial &material, int width, int height);
	~HeightField();
	PxHeightFieldSample* samples;
	PxHeightFieldDesc hfDesc;
	PxHeightField* aHeightField;
	PxHeightFieldGeometry *hfGeom;
	PxTransform *terrainPos;
	PxShape* aHeightFieldShape;
	Terrain terrain;
	PxRigidDynamic *g_pxHeightField;
	int width;
	int height;
	int nrVertices;
	void fillSamples();
	void fillDesc();
};

