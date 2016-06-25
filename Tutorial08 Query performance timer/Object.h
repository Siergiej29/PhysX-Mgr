#pragma once
#include <d3d10.h>
#include <d3dx10.h>
#include <vector>
#include <fstream>
#include <istream> 
#include <sstream>

#include "mikktspace.h"

using namespace std;

//OBJ Loader
struct Vertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR2 texCoord;
    D3DXVECTOR3 normal;
    D3DXVECTOR3 tangent;
    D3DXVECTOR3 bitangent;
};

// Texture manager structure, So all the textures are nice and grouped together
struct TextureManager
{
    std::vector<ID3D10ShaderResourceView*> TextureList;
    std::vector<std::string> TextureNameArray;     // So we don't load in the same texture twice
};

// Create material structure
struct SurfaceMaterial
{
    std::string MatName;   // So we can match the subset with it's material

    // Surface's colors
    D3DXVECTOR4 Diffuse;       // Transparency (Alpha) stored in 4th component
    D3DXVECTOR3 Ambient;
    D3DXVECTOR4 Specular;      // Specular power stored in 4th component

    // Texture ID's to look up texture in SRV array
    int DiffuseTextureID;
    int AmbientTextureID;
    int SpecularTextureID;
    int AlphaTextureID;
    int NormMapTextureID;

    // Booleans so we don't implement techniques we don't need
    bool HasDiffTexture;
    bool HasAmbientTexture;
    bool HasSpecularTexture;
    bool HasAlphaTexture;
    bool HasNormMap;
    bool IsTransparent;
};

struct SSimpleVertex
{
	D3DXVECTOR3 pos, norm, tang, bitang;
	D3DXVECTOR2 magST;
	D3DXVECTOR2 texc;
};

struct MySimpleVertex
{
	int pos, norm, tang, bitang, texc;
};

struct SSimpleFace
{
	int iVertOffs;
	int iNrVerts;	// should be 3!
};
#pragma endregion

#pragma region "CSimpleMesh wrapper class"
class CSimpleMesh
{
public:
    CSimpleMesh ( vector<SSimpleVertex> &simpleVerts, vector<SSimpleFace> &simpleFaces, const int iNrFaces, const bool bIsDerivMapOut ) : 
        m_stlSimpleVerts(simpleVerts), m_stlSimpleFaces(simpleFaces), mc_iNrFaces(iNrFaces), m_bIsDerivMapOut(bIsDerivMapOut)
    {
    }

	~CSimpleMesh()
    {
       	// data deleted externally
    }

	void SetTangent ( const D3DXVECTOR3 &tang, const float fMagS, const int index )
	{
		m_stlSimpleVerts[static_cast<size_typeV>(index)].tang = tang;
		m_stlSimpleVerts[static_cast<size_typeV>(index)].magST.x = fMagS;
	}

	void SetBiTangent ( const D3DXVECTOR3 &bitang, const float fMagT, const int index )
	{
		m_stlSimpleVerts[static_cast<size_typeV>(index)].bitang = bitang;
		m_stlSimpleVerts[static_cast<size_typeV>(index)].magST.y = fMagT;
	}

	int GetNrFaces() const
    { 
        return mc_iNrFaces;
    }

	bool GetIsDerivMapOut() const
    { 
        return m_bIsDerivMapOut;
    }

	const SSimpleVertex& GetSimpleVertex ( const int index ) const
    { 
        return m_stlSimpleVerts[static_cast<size_typeV>(index)];
    }

	const SSimpleFace& GetSimpleFace ( const int index ) const
    { 
        return m_stlSimpleFaces[static_cast<size_typeF>(index)];
    }

private:
    typedef vector<SSimpleVertex>::size_type size_typeV;
    vector<SSimpleVertex> &m_stlSimpleVerts;

    typedef vector<SSimpleFace>::size_type size_typeF;
    const vector<SSimpleFace> &m_stlSimpleFaces;

    const int mc_iNrFaces;
	const bool m_bIsDerivMapOut;
};
#pragma endregion

#pragma region "C static functions for wrapper"
static int getNumFaces ( const SMikkTSpaceContext * pContext )
{
	CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	return l_pSimpleMesh->GetNrFaces();	// not necessarily triangles
}

static int getNumVerticesOfFace ( const SMikkTSpaceContext * pContext, const int iFace )
{
	CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	return l_pSimpleMesh->GetSimpleFace(iFace).iNrVerts;
}

// 0, 1, 2 for tri and 3 for quad
static void getPosition ( const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert )
{
	CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	const SSimpleFace &face ( l_pSimpleMesh->GetSimpleFace(iFace) );

	const D3DXVECTOR3 &P ( l_pSimpleMesh->GetSimpleVertex(face.iVertOffs + iVert).pos );
	fvPosOut[0] = P.x; 
    fvPosOut[1] = P.y; 
    fvPosOut[2] = P.z;
}

// 0, 1, 2 for tri and 3 for quad
static void getNormal ( const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace, const int iVert )
{
	CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	const SSimpleFace &face ( l_pSimpleMesh->GetSimpleFace(iFace) );
	
	const D3DXVECTOR3 &N ( l_pSimpleMesh->GetSimpleVertex(face.iVertOffs + iVert).norm );
	fvNormOut[0] = N.x; 
    fvNormOut[1] = N.y; 
    fvNormOut[2] = N.z;
}

// 0, 1, 2 for tri and 3 for quad
static void getTexCoord ( const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace, const int iVert )
{
	CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	const SSimpleFace &face ( l_pSimpleMesh->GetSimpleFace(iFace) );
	
	const D3DXVECTOR2 &T ( l_pSimpleMesh->GetSimpleVertex(face.iVertOffs + iVert).texc );
	fvTexcOut[0] = T.x;
    fvTexcOut[1] = T.y;
}


static void setTSpace (const SMikkTSpaceContext * pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT,
					   const tbool bIsOrientationPreserving, const int iFace, const int iVert)
{
    CSimpleMesh *l_pSimpleMesh ( reinterpret_cast<CSimpleMesh*>(pContext->m_pUserData) );
	const SSimpleFace &face ( l_pSimpleMesh->GetSimpleFace(iFace) );
	const int index ( face.iVertOffs + iVert );
	const bool bIsDerivMapOut ( l_pSimpleMesh->GetIsDerivMapOut() );
	
    vector<float> fNorm ( static_cast<vector<float>::size_type>(3U) );
    getNormal ( pContext, &fNorm[0], iFace, iVert );

	l_pSimpleMesh->SetTangent( D3DXVECTOR3(reinterpret_cast<const float*>(fvTangent)), fMagS, index );

	if ( false==bIsDerivMapOut )
	{
		// fSign * cross(vN, vT) <-- In that order!
		const float fSign = (bIsOrientationPreserving!=0) ? 1.0f : -1.0f;
		const float bx ( fSign * (fNorm[1]*fvTangent[2] - fNorm[2]*fvTangent[1]) );
		const float by ( fSign * (fNorm[2]*fvTangent[0] - fNorm[0]*fvTangent[2]) );
		const float bz ( fSign * (fNorm[0]*fvTangent[1] - fNorm[1]*fvTangent[0]) );
		
		l_pSimpleMesh->SetBiTangent ( D3DXVECTOR3(bx,by,bz), 1.0f, index );
	}
	else
    {
		l_pSimpleMesh->SetBiTangent( D3DXVECTOR3(reinterpret_cast<const float*>(fvBiTangent)), fMagT, index );
    }
}
#pragma endregion


class Object
{
public:
	Object(void);
	~Object(void);

    int Subsets;                        // Number of subsets in obj model
    ID3D10Buffer* VertBuff;             // Models vertex buffer
    ID3D10Buffer* IndexBuff;            // Models index buffer
    std::vector<DWORD> Indices;         // Models index list
	std::vector<Vertex> Vertices;       // Models vertices (unique)
    std::vector<int> SubsetIndexStart;  // Subset's index offset
    std::vector<int> SubsetMaterialID;  // Lookup ID for subsets surface material
	std::vector<SurfaceMaterial> material;

	bool LoadObjModel(ID3D10Device* device,
    std::string Filename, 
    TextureManager& TexMgr,
    bool IsRHCoordSys);

	void Render(ID3D10Device* device,
    TextureManager& TexMgr,
	ID3D10EffectTechnique* technique,
	ID3D10InputLayout* vertexlayout,
	ID3D10EffectMatrixVariable* v1,
	ID3D10EffectMatrixVariable* v4,
	ID3D10EffectShaderResourceVariable* v2,
	ID3D10EffectShaderResourceVariable* v3,
	D3DXMATRIX* world);

};

