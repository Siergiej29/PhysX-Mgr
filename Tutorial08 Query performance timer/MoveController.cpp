#include "MoveController.h"

#define DEG_TO_RAD 0.01745329251994329576923690768489

MoveController::MoveController(void)
{
	Forward.x = 0;
	Forward.y = 1;
	Forward.z = 1;
}


MoveController::~MoveController(void)
{
}


void MoveController::setMovementToggle( int i, int v )
{
	movementToggles[i] = v;
}


void MoveController::Update(float x, float y, float z)
{
	//odszumianie
	if (fabs(Forward.x - x) < 0.0001 ) x = Forward.x;
	if (fabs(Forward.y - y) < 0.0001 ) y = Forward.y;
	if (fabs(Forward.z - z) < 0.0001 ) z = Forward.z;

	D3DXVECTOR3 iForward(x, y, z);

	if (D3DXVec3Length(&iForward) > 0.0001)
	{
		D3DXVec3Normalize(&Forward, &iForward);
	}
}

bool MoveController::getRightDirection(D3DXVECTOR3 &R)
{
	//wyznaczenie R i UP na podstawie Forward i UpWorld
	D3DXVECTOR3 UpWorld(0.0, 1.0, 0.0);
	D3DXVECTOR3 Up;
	float d = D3DXVec3Dot(&Forward, &UpWorld);
	Up = UpWorld - Forward * d;
	D3DXVec3Cross(&R, &Forward, &Up);

	//jeœli 2 lub 3 to obrót R wokó³ UP
	if (movementToggles[2] != 0 || movementToggles[3] != 0)
	{
		D3DXMATRIX rot;
		float angle = 10.0f * DEG_TO_RAD * (movementToggles[2] - movementToggles[3]);
		D3DXMatrixRotationAxis(&rot, &Up, angle);
		D3DXVec3TransformNormal(&R, &R, &rot);
	}

	//jeœli zero lub 1 to zwróæ znacznik i wektor
	if (movementToggles[0] != 0 || movementToggles[1] != 0)
	{
		R = 10.0 * R * (movementToggles[1] - movementToggles[0]);
		return true;
	}
	else return false;
}