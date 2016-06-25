#pragma once

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>

#include "Camera.h"
#include "Terrain.h"

class SunController
{
private:
	D3DXMATRIX LightViewProjection;
	void ComputeNearAndFar( float& fNearPlane, float& fFarPlane, float minX, float minY, float maxX, float maxY, D3DXVECTOR4* pvPointsInCameraView ); 
	float GetByIndex(D3DXVECTOR4& Vec, int Index);
public:
	SunController(void);
	~SunController(void);

	D3DXVECTOR3 Dir, Up;

	D3DXMATRIX &getLightViewProjection(Camera *p_Camera, Terrain *p_Terrain);
	void getLightDirection(D3DXVECTOR3 &LightDirection);
	void Update(DWORD t);
};

