#include "SkyController.h"


SkyController::SkyController(int tMax, DWORD dt)
{
	TMax = tMax;
	DT = dt;
	T1 = 2;
	T2 = rand()%TMax;
	t = 0.0;
}


SkyController::~SkyController(void)
{
}

void SkyController::Update(DWORD ActTime) //czas od pocz¹tku gry
{
	if (ActTime > Time1)
	{
		Time0 = Time1;
		Time1 += DT;
		T1 = T2;
		T2 = rand()%TMax;
	}
	t = float(ActTime - Time0)/float(DT);

}