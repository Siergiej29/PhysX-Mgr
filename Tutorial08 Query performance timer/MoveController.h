#pragma once
#include <d3d10.h>
#include <d3dx10.h>
class MoveController
{
private:
	D3DXVECTOR3 Forward;
	int movementToggles[4];
public:
	MoveController(void);
	~MoveController(void);
	void setMovementToggle( int i, int v );
	void Update(float x, float y, float z);
	bool getRightDirection(D3DXVECTOR3 &R);
};

