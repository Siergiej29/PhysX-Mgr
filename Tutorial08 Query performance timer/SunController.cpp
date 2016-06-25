#include "SunController.h"

#define DEG_TO_RAD 0.01745329251994329576923690768489

SunController::SunController(void)
{
}


SunController::~SunController(void)
{
}


void SunController::Update(DWORD t)
{
	float Alpha = DEG_TO_RAD * 100.0f;
	float Beta = t/5000.0f;
	D3DXVECTOR3 tdir(0, 0, -1);
	D3DXVECTOR3 tup(0, 1, 0);

	D3DXMATRIX mata, matb;
	D3DXMatrixRotationX(&mata, -Alpha);
	D3DXMatrixRotationY(&matb, Beta);
	D3DXMatrixMultiply(&mata, &mata, &matb);
	D3DXVec3TransformNormal(&Dir, &tdir, &mata);
	D3DXVec3TransformNormal(&Up, &tup, &mata);
}

void SunController::getLightDirection(D3DXVECTOR3 &LightDirection)
{
	LightDirection.x = Dir.x;
	LightDirection.y = Dir.y;
	LightDirection.z = Dir.z;
}


D3DXMATRIX& SunController::getLightViewProjection(Camera *p_Camera, Terrain *p_Terrain)
{
	D3DXVECTOR3 sat(0,0,0);
	D3DXVec3Normalize(&Dir, &Dir);
	D3DXVECTOR3 seye = - Dir;

	//wyznaczenie UP na podstawie Dir i UpWorld (Ortogonalizacja Grama-Schmidta)
	D3DXVECTOR3 UpWorld(1.0, 1.0, 1.0);
	D3DXVECTOR3 sup;
	float d = D3DXVec3Dot(&Dir, &UpWorld);
	sup = UpWorld - Dir * d;
	D3DXVec3Normalize(&sup, &sup);


	D3DXMATRIX LVM, iLVM;
	D3DXMatrixLookAtLH( &LVM, &seye, &sat, &sup );
	
	float det;
	D3DXMatrixInverse( &iLVM, &det, &LVM);

	//calc 8 points of Cam Frustum in LightSpace
	float minX, maxX, minY, maxY;

	float Hnear = 2 * tan(p_Camera->FOV / 2) * p_Camera->zNear;
	float Wnear = Hnear * p_Camera->aspectRatio;
	float Hfar = 2 * tan(p_Camera->FOV / 2) * p_Camera->zFar;
	float Wfar = Hfar * p_Camera->aspectRatio;

	D3DXVECTOR3 p = p_Camera->getCameraPosition();
	D3DXVECTOR3 dr = p_Camera->getCameraDirection();
	D3DXVECTOR3 u = p_Camera->getCameraUp();

	//float Hnear = 2 * tan(0.01745329251994329576923690768489 * 45.0f / 2.0f);
	//float Wnear = Hnear * 1.0f;
	//float Hfar = 2 * tan(0.01745329251994329576923690768489 * 45.0f / 2.0f) * 4.0f;
	//float Wfar = Hfar * 1.0f;

	//D3DXVECTOR3 p(0,0,0);
	//D3DXVECTOR3 dr(0,0,1);
	//D3DXVECTOR3 u(0,1,0);

	D3DXVECTOR3 r;
	D3DXVec3Cross(&r, &dr, &u);

	D3DXVECTOR3 Points[8];
	D3DXVECTOR3 fc = p + dr * p_Camera->zFar; 
	Points[0] = fc + (u * Hfar/2) - (r * Wfar/2);
	Points[1] = fc + (u * Hfar/2) + (r * Wfar/2);
	Points[2] = fc - (u * Hfar/2) - (r * Wfar/2);
	Points[3] = fc - (u * Hfar/2) + (r * Wfar/2);

	D3DXVECTOR3 nc = p + dr * p_Camera->zNear; 
	Points[4] = nc + (u * Hnear/2) - (r * Wnear/2);
	Points[5] = nc + (u * Hnear/2) + (r * Wnear/2);
	Points[6] = nc - (u * Hnear/2) - (r * Wnear/2);
	Points[7] = nc - (u * Hnear/2) + (r * Wnear/2);

    // Compute the frustum corners in light space (WS -> LS).
	D3DXVECTOR4 Point = D3DXVECTOR4(Points[0].x, Points[0].y, Points[0].z, 1.0f);

	// Transform point.
	D3DXVec4Transform(&Point, &Point, &LVM);
	minX = maxX = Point.x;
	minY = maxY = Point.y;

    for( int i = 1; i < 8; i++ )
    {
        // Transform point.
		Point = D3DXVECTOR4(Points[i].x, Points[i].y, Points[i].z, 1.0f);
		D3DXVec4Transform(&Point, &Point, &LVM);
		if (Point.x < minX) minX = Point.x;
		if (Point.x > maxX) maxX = Point.x;
		if (Point.y < minY) minY = Point.y;
		if (Point.y > maxY) maxY = Point.y;
    }

	//czêœæ danych do obliczenia macierzy projekcji ortograficznej dla œwiat³a
	float W = maxX - minX;
	float H = maxY - minY;
	float posX = minX + W / 2.0;
	float posY = minY + H / 2.0;
	
	//teraz zN i zF

	//scene AABB
    //This map enables us to use a for loop and do vector math.
	D3DXVECTOR4 AABB[] = 
    { 
		D3DXVECTOR4(p_Terrain->MaxX, p_Terrain->MaxY, p_Terrain->MinZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MinX, p_Terrain->MaxY, p_Terrain->MinZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MaxX, p_Terrain->MinY, p_Terrain->MinZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MinX, p_Terrain->MinY, p_Terrain->MinZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MaxX, p_Terrain->MaxY, p_Terrain->MaxZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MinX, p_Terrain->MaxY, p_Terrain->MaxZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MaxX, p_Terrain->MinY, p_Terrain->MaxZ, 1.0f), 
        D3DXVECTOR4(p_Terrain->MinX, p_Terrain->MinY, p_Terrain->MaxZ, 1.0f) 
    };
    
    for( int i = 0; i < 8; ++i ) 
    {
		D3DXVec4Transform(&AABB[i], &AABB[i], &LVM);
    }
	
	float minZ, maxZ;
	ComputeNearAndFar(minZ, maxZ, minX, minY, maxX, maxY, AABB);

	float posZ = minZ - 1;
	float zN = 1;
	float zF = maxZ - minZ + 1;

	D3DXVECTOR4 Leye(posX, posY, posZ, 1.0f);
	D3DXVECTOR4 Leye2;
	D3DXVec4Transform(&Leye2, &Leye, &iLVM);
	D3DXVECTOR3 teye(Leye2.x, Leye2.y, Leye2.z);
	sat = teye + Dir;

	D3DXMATRIX mtmp, mtmp2;
	D3DXMatrixLookAtLH( &mtmp, &teye, &sat, &sup );	
	D3DXMatrixOrthoLH(&mtmp2, W, H, zN, zF);
	D3DXMatrixMultiply(&LightViewProjection, &mtmp, &mtmp2);

	return LightViewProjection;
}


float SunController::GetByIndex(D3DXVECTOR4& Vec, int Index)
{
	switch (Index)
	{
		case 0: return Vec.x; break;
		case 1: return Vec.y; break;
		case 2: return Vec.z; break;
		default: return Vec.w;
	}
}

//Adaptacja metody opisanej w CascadedShadowMaps11 (Microsoft DirectX SDK June 2010)

//--------------------------------------------------------------------------------------
// Used to compute an intersection of the orthographic projection and the Scene AABB
//--------------------------------------------------------------------------------------
struct Triangle 
{
	D3DXVECTOR4 pt[3];
    BOOL culled;
};


//--------------------------------------------------------------------------------------
// Computing an accurate near and flar plane will decrease surface acne and Peter-panning.
// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
// shadows disappear near the base of an object.
// As offsets are generally used with PCF filtering due self shadowing issues, computing the
// correct near and far planes becomes even more important.
// This concept is not complicated, but the intersection code is.
//--------------------------------------------------------------------------------------
void SunController::ComputeNearAndFar( float& fNearPlane, 
                                       float& fFarPlane, 
                                       float minX, float minY, float maxX, float maxY, 
									   D3DXVECTOR4* pvPointsInCameraView ) 
{

    // Initialize the near and far planes
    fNearPlane = FLT_MAX;
    fFarPlane = -FLT_MAX;
    
    Triangle triangleList[16];
    INT iTriangleCnt = 1;

    triangleList[0].pt[0] = pvPointsInCameraView[0];
    triangleList[0].pt[1] = pvPointsInCameraView[1];
    triangleList[0].pt[2] = pvPointsInCameraView[2];
    triangleList[0].culled = false;

    // These are the indices used to tesselate an AABB into a list of triangles.
    static const int iAABBTriIndexes[] = 
    {
        0,1,2,  1,2,3,
        4,5,6,  5,6,7,
        0,2,4,  2,4,6,
        1,3,5,  3,5,7,
        0,1,4,  1,4,5,
        2,3,6,  3,6,7 
    };

    int iPointPassesCollision[3];

    // At a high level: 
    // 1. Iterate over all 12 triangles of the AABB.  
    // 2. Clip the triangles against each plane. Create new triangles as needed.
    // 3. Find the min and max z values as the near and far plane.
    
    //This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.
    
    float fLightCameraOrthographicMinX = minX;//XMVectorGetX( vLightCameraOrthographicMin );
    float fLightCameraOrthographicMaxX = maxX;//XMVectorGetX( vLightCameraOrthographicMax ); 
    float fLightCameraOrthographicMinY = minY;//XMVectorGetY( vLightCameraOrthographicMin );
    float fLightCameraOrthographicMaxY = maxY;//XMVectorGetY( vLightCameraOrthographicMax );
    
    for( INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter ) 
    {

        triangleList[0].pt[0] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 0 ] ];
        triangleList[0].pt[1] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 1 ] ];
        triangleList[0].pt[2] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 2 ] ];
        iTriangleCnt = 1;
        triangleList[0].culled = FALSE;

        // Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
        //add them to the list.
        for( INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter ) 
        {

            FLOAT fEdge;
            INT iComponent;
            
            if( frustumPlaneIter == 0 ) 
            {
                fEdge = fLightCameraOrthographicMinX; // todo make float temp
                iComponent = 0;
            } 
            else if( frustumPlaneIter == 1 ) 
            {
                fEdge = fLightCameraOrthographicMaxX;
                iComponent = 0;
            } 
            else if( frustumPlaneIter == 2 ) 
            {
                fEdge = fLightCameraOrthographicMinY;
                iComponent = 1;
            } 
            else 
            {
                fEdge = fLightCameraOrthographicMaxY;
                iComponent = 1;
            }

            for( INT triIter=0; triIter < iTriangleCnt; ++triIter ) 
            {
                // We don't delete triangles, so we skip those that have been culled.
                if( !triangleList[triIter].culled ) 
                {
                    INT iInsideVertCount = 0;
					D3DXVECTOR4 tempOrder;
                    // Test against the correct frustum plane.
                    // This could be written more compactly, but it would be harder to understand.
                    
                    if( frustumPlaneIter == 0 ) 
                    {
                        for( INT triPtIter=0; triPtIter < 3; ++triPtIter ) 
                        {
                            if( triangleList[triIter].pt[triPtIter].x > minX )
                            { 
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else 
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else if( frustumPlaneIter == 1 ) 
                    {
                        for( INT triPtIter=0; triPtIter < 3; ++triPtIter ) 
                        {
                            if( triangleList[triIter].pt[triPtIter].x < maxX )
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else
                            { 
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else if( frustumPlaneIter == 2 ) 
                    {
                        for( INT triPtIter=0; triPtIter < 3; ++triPtIter ) 
                        {
                            if( triangleList[triIter].pt[triPtIter].y > minY )
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else 
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else 
                    {
                        for( INT triPtIter=0; triPtIter < 3; ++triPtIter ) 
                        {
                            if( triangleList[triIter].pt[triPtIter].y < maxY )
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else 
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }

                    // Move the points that pass the frustum test to the begining of the array.
                    if( iPointPassesCollision[1] && !iPointPassesCollision[0] ) 
                    {
                        tempOrder =  triangleList[triIter].pt[0];   
                        triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
                        triangleList[triIter].pt[1] = tempOrder;
                        iPointPassesCollision[0] = TRUE;            
                        iPointPassesCollision[1] = FALSE;            
                    }
                    if( iPointPassesCollision[2] && !iPointPassesCollision[1] ) 
                    {
                        tempOrder =  triangleList[triIter].pt[1];   
                        triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
                        triangleList[triIter].pt[2] = tempOrder;
                        iPointPassesCollision[1] = TRUE;            
                        iPointPassesCollision[2] = FALSE;                        
                    }
                    if( iPointPassesCollision[1] && !iPointPassesCollision[0] ) 
                    {
                        tempOrder =  triangleList[triIter].pt[0];   
                        triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
                        triangleList[triIter].pt[1] = tempOrder;
                        iPointPassesCollision[0] = TRUE;            
                        iPointPassesCollision[1] = FALSE;            
                    }
                    
                    if( iInsideVertCount == 0 ) 
                    { // All points failed. We're done,  
                        triangleList[triIter].culled = true;
                    }
                    else if( iInsideVertCount == 1 ) 
                    {// One point passed. Clip the triangle against the Frustum plane
                        triangleList[triIter].culled = false;
                        
                        D3DXVECTOR4 vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
                        D3DXVECTOR4 vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];
                        
                        // Find the collision ratio.
                        FLOAT fHitPointTimeRatio = fEdge - GetByIndex( triangleList[triIter].pt[0], iComponent ) ;
                        // Calculate the distance along the vector as ratio of the hit ratio to the component.
                        FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / GetByIndex( vVert0ToVert1, iComponent );
                        FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / GetByIndex( vVert0ToVert2, iComponent );
                        // Add the point plus a percentage of the vector.
                        vVert0ToVert1 *= fDistanceAlongVector01;
                        vVert0ToVert1 += triangleList[triIter].pt[0];
                        vVert0ToVert2 *= fDistanceAlongVector02;
                        vVert0ToVert2 += triangleList[triIter].pt[0];

                        triangleList[triIter].pt[1] = vVert0ToVert2;
                        triangleList[triIter].pt[2] = vVert0ToVert1;

                    }
                    else if( iInsideVertCount == 2 ) 
                    { // 2 in  // tesselate into 2 triangles
                        

                        // Copy the triangle\(if it exists) after the current triangle out of
                        // the way so we can override it with the new triangle we're inserting.
                        triangleList[iTriangleCnt] = triangleList[triIter+1];

                        triangleList[triIter].culled = false;
                        triangleList[triIter+1].culled = false;
                        
                        // Get the vector from the outside point into the 2 inside points.
						D3DXVECTOR4 vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
						D3DXVECTOR4 vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];
                        
                        // Get the hit point ratio.
                        FLOAT fHitPointTime_2_0 =  fEdge - GetByIndex( triangleList[triIter].pt[2], iComponent );
                        FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / GetByIndex( vVert2ToVert0, iComponent );
                        // Calcaulte the new vert by adding the percentage of the vector plus point 2.
                        vVert2ToVert0 *= fDistanceAlongVector_2_0;
                        vVert2ToVert0 += triangleList[triIter].pt[2];
                        
                        // Add a new triangle.
                        triangleList[triIter+1].pt[0] = triangleList[triIter].pt[0];
                        triangleList[triIter+1].pt[1] = triangleList[triIter].pt[1];
                        triangleList[triIter+1].pt[2] = vVert2ToVert0;
                        
                        //Get the hit point ratio.
                        FLOAT fHitPointTime_2_1 =  fEdge - GetByIndex( triangleList[triIter].pt[2], iComponent ) ;
                        FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / GetByIndex( vVert2ToVert1, iComponent );
                        vVert2ToVert1 *= fDistanceAlongVector_2_1;
                        vVert2ToVert1 += triangleList[triIter].pt[2];
                        triangleList[triIter].pt[0] = triangleList[triIter+1].pt[1];
                        triangleList[triIter].pt[1] = triangleList[triIter+1].pt[2];
                        triangleList[triIter].pt[2] = vVert2ToVert1;
                        // Cncrement triangle count and skip the triangle we just inserted.
                        ++iTriangleCnt;
                        ++triIter;              
                    }
                    else 
                    { // all in
                        triangleList[triIter].culled = false;
                    }
                }// end if !culled loop            
            }
        }
        for( INT index=0; index < iTriangleCnt; ++index ) 
        {
            if( !triangleList[index].culled ) 
            {
                // Set the near and far plan and the min and max z values respectivly.
                for( int vertind = 0; vertind < 3; ++ vertind ) 
                {
                    float fTriangleCoordZ = triangleList[index].pt[vertind].z;
                    if( fNearPlane > fTriangleCoordZ ) 
                    {
                        fNearPlane = fTriangleCoordZ;
                    }
                    if( fFarPlane  <fTriangleCoordZ ) 
                    {
                        fFarPlane = fTriangleCoordZ;
                    }
                }
            }
        }
    }
}