#include "HeightField.h"



HeightField::HeightField(Terrain terrain, PxPhysics& sdk, PxMaterial &material, int width, int height)
{
	this->nrVertices = terrain.NumVertices;
	this->terrain = terrain;
	this->width = width;
	this->height = height;
	this->fillSamples();
	this->fillDesc();
	this->aHeightField = sdk.createHeightField(hfDesc);
	this->hfGeom = new PxHeightFieldGeometry(aHeightField, PxMeshGeometryFlags(), this->terrain.dy / 255.0, this->terrain.dx, this->terrain.dz);
	this->terrainPos = new PxTransform(PxVec3(-this->terrain.dx*(this->width - 1) / 2, 0.0f, this->terrain.dz*(this->height - 1) / 2), PxQuat(3.1415 / 2.0, PxVec3(0, 1, 0)));
	this->g_pxHeightField = sdk.createRigidDynamic(*this->terrainPos);
	this->g_pxHeightField->setRigidDynamicFlag(PxRigidDynamicFlag::eKINEMATIC, true);
	PxShape* aHeightFieldShape = this->g_pxHeightField->createShape(*(this->hfGeom), material);
}


HeightField::~HeightField()
{
}


void HeightField::fillSamples()
{
	this->samples = (PxHeightFieldSample*)malloc(sizeof(PxHeightFieldSample)*(this->nrVertices));

	for (int i = 0; i < this->nrVertices; i++)
	{
		samples[i].height = this->terrain.hminfo.heightMap[i].y;
		samples[i].clearTessFlag();
	}
}

void HeightField::fillDesc()
{
	this->hfDesc.format = PxHeightFieldFormat::eS16_TM;
	this->hfDesc.nbColumns = this->width;
	this->hfDesc.nbRows = this->height;
	this->hfDesc.samples.data = this->samples;
	this->hfDesc.samples.stride = sizeof(PxHeightFieldSample);
}