#ifndef _XCAMERA
#define _XCAMERA

#include <d3dx10.h>

#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.01745329251994329576923690768489

class Camera
{	
	/*******************************************************************
	* Members
	********************************************************************/	
private:
		
	//view parameters
	float heading, pitch;					//in radians

	//matrices
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projectionMatrix;	
	D3DXMATRIX rotationMatrix;
		
	//view vectors
	const D3DXVECTOR3 dV, dU;				//default view and up vectors
	D3DXVECTOR3 eye, view, up, right;

	//movement toggles
	int movementToggles[4];					//przód, ty³, lewo, prawo
	float movementSpeed;

	//projection parameters
	bool ProjectionDirtyFlag;
	
	/*******************************************************************
	* Methods
	********************************************************************/	
public:

	//constructor and destructor
	Camera();
	virtual ~Camera();

	//set projection methods
	void setPerspectiveProjectionLH( float iFOV, float iaspectRatio, float izNear, float izFar );
			
	//camera positioning methods
	void setPositionAndView( float x, float y, float z, float hDeg, float pDeg );
	void adjustHeadingPitch( float hRad, float pRad );
	void setMovementToggle( int i, int v );	
	void setMovementSpeed( float s );	

	//update camera view/position
	void update(double dt);	
	void update(double x, double y, double z);

	//get methods	
	D3DXMATRIX&		getViewMatrix(){ return viewMatrix; }
	D3DXMATRIX&		getProjectionMatrix(){ ProjectionDirtyFlag = false; return projectionMatrix; }
	D3DXMATRIX&		getRotationMatrix(){ return rotationMatrix; }
	D3DXVECTOR3&	getCameraPosition(){ return eye; }
	D3DXVECTOR3&	getCameraDirection(){ return view; }
	D3DXVECTOR3&	getCameraUp(){ return up; }
	D3DXVECTOR3&	getCameraRight(){ return right; }
	bool			isProjectionDirty(){ return ProjectionDirtyFlag; }

	float FOV, aspectRatio, zNear, zFar;

private:

	//create view, forward, strafe vectors from heading/pitch
	void updateView();
};

#endif