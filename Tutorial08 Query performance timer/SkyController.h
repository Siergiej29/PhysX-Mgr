#pragma once
#include <Windows.h>

class SkyController
{
public:
	SkyController(int tMax, DWORD dt);
	~SkyController(void);
	void Update(DWORD ActTime);

	float t;
	int T1;
	int T2;
	int TMax;

	DWORD Time0;
	DWORD Time1;
	DWORD DT;
};

