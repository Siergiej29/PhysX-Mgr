#include "Terrain.h"


Terrain::Terrain(void)
{
	v = NULL;
	indices = NULL;
	dx = dz = 1000; //odleg³oœæ miêdzy punktami grid'a 
	dy = 1000; //maksymalna wysokoœæ terenu
}


Terrain::~Terrain(void)
{
	if (v != NULL) delete [] v;
	if (indices != NULL) delete indices;
	if (hminfo.heightMap != NULL) delete [] hminfo.heightMap;
}

bool Terrain::HeightMapLoad(char* filename, float sx, float sz, float maxy)
{
	//FILE *filePtr;							// Point to the current position in the file
	//BITMAPFILEHEADER bitmapFileHeader;		// Structure which stores information about file
	//BITMAPINFOHEADER bitmapInfoHeader;		// Structure which stores information about image
	int imageSize, index;
	unsigned char height;

	// Open the file
	//filePtr = fopen(filename,"rb");
	//if (filePtr == NULL)
	//	return 0;

	dx = sz;
	dz = sz;
	dy = maxy;

	

	// Get the width and height (width and length) of the image
	hminfo.terrainWidth = 145;// bitmapInfoHeader.biWidth;
	hminfo.terrainHeight = 145;// bitmapInfoHeader.biHeight;


	// Initialize the heightMap array (stores the vertices of our terrain)
	hminfo.heightMap = new IntV3[hminfo.terrainWidth * hminfo.terrainHeight];

	// We use a greyscale image, so all 3 rgb values are the same, but we only need one for the height
	// So we use this counter to skip the next two components in the image data (we read R, then skip BG)
	int k=0;

	// Read the image data into our heightMap array
	for(int j=0; j< hminfo.terrainHeight; j++)
	{
		for(int i=0; i< hminfo.terrainWidth; i++)
		{
			height = rand()%50+1;

			index = ( hminfo.terrainWidth * (hminfo.terrainHeight - 1 - j)) + i;

			hminfo.heightMap[index].x = i - (hminfo.terrainWidth - 1)/2;
			hminfo.heightMap[index].y = height;
			hminfo.heightMap[index].z = j - (hminfo.terrainHeight - 1)/2;

			k+=3;
		}
		k++;
	}


	int cols = hminfo.terrainWidth;
	int rows = hminfo.terrainHeight;

	//Create the grid
	NumVertices = 2 * rows * cols;
	NumFaces  = (rows-1)*(cols-1)*2;

	v = new struct HeightFieldVertex[NumVertices];

	for(DWORD i = 0; i < rows; ++i)
	{
		for(DWORD j = 0; j < cols; ++j)
		{
			v[i*cols+j].pos.x = hminfo.heightMap[i*cols+j].x * dx;
			v[i*cols+j].pos.y = (float(hminfo.heightMap[i*cols+j].y)/255.0) * dy;
			v[i*cols+j].pos.z = hminfo.heightMap[i*cols+j].z * dz;
			v[i*cols+j].texCoord = D3DXVECTOR2(j, i);
		}
	}

	indices = new DWORD[NumFaces * 3];

	k = 0;
	for(DWORD i = 0; i < rows-1; i++)
	{
		for(DWORD j = 0; j < cols-1; j++)
		{
			indices[k]   = i*cols+j;		// Bottom left of quad
			indices[k+1] = i*cols+j+1;		// Bottom right of quad
			indices[k+2] = (i+1)*cols+j;	// Top left of quad
			indices[k+3] = (i+1)*cols+j;	// Top left of quad
			indices[k+4] = i*cols+j+1;		// Bottom right of quad
			indices[k+5] = (i+1)*cols+j+1;	// Top right of quad

			k += 6; // next quad
		}
	}

	//normals & tangents
    std::vector<D3DXVECTOR3> tempNormal;

    //normalized and unnormalized normals
    D3DXVECTOR3 unnormalized(0.0f, 0.0f, 0.0f);

    //tangent stuff
    std::vector<D3DXVECTOR3> tempTangent;

    D3DXVECTOR3 tangent(0.0f, 0.0f, 0.0f);
    float tcU1, tcV1, tcU2, tcV2;

    //Used to get vectors (sides) from the position of the verts
    float vecX, vecY, vecZ;

    //Two edges of our triangle
    D3DXVECTOR3 edge1(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 edge2(0.0f, 0.0f, 0.0f);

    //Compute face normals
    //And Tangents
    for(int i = 0; i < NumFaces; ++i)
    {
        //Get the vector describing one edge of our triangle (edge 0,2)
        vecX = v[indices[(i*3)+1]].pos.x - v[indices[(i*3)]].pos.x;
        vecY = v[indices[(i*3)+1]].pos.y - v[indices[(i*3)]].pos.y;
        vecZ = v[indices[(i*3)+1]].pos.z - v[indices[(i*3)]].pos.z;       
        edge1 = D3DXVECTOR3(vecX, vecY, vecZ);    //Create our first edge

        //Get the vector describing another edge of our triangle (edge 2,1)
        vecX = v[indices[(i*3)+2]].pos.x - v[indices[(i*3)]].pos.x;
        vecY = v[indices[(i*3)+2]].pos.y - v[indices[(i*3)]].pos.y;
        vecZ = v[indices[(i*3)+2]].pos.z - v[indices[(i*3)]].pos.z;     
        edge2 = D3DXVECTOR3(vecX, vecY, vecZ);    //Create our second edge

        //Cross multiply the two edge vectors to get the un-normalized face normal
		D3DXVec3Cross(&unnormalized, &edge1, &edge2);
        tempNormal.push_back(unnormalized);

        //Find first texture coordinate edge 2d vector
        tcU1 = v[indices[(i*3)+1]].texCoord.x - v[indices[(i*3)]].texCoord.x;
        tcV1 = v[indices[(i*3)+1]].texCoord.y - v[indices[(i*3)]].texCoord.y;

        //Find second texture coordinate edge 2d vector
        tcU2 = v[indices[(i*3)+2]].texCoord.x - v[indices[(i*3)]].texCoord.x;
        tcV2 = v[indices[(i*3)+2]].texCoord.y - v[indices[(i*3)]].texCoord.y;

        //Find tangent using both tex coord edges and position edges
        tangent.x = (tcV2 * edge1.x - tcV1 * edge2.x)  / (tcU1 * tcV2 - tcU2 * tcV1);
        tangent.y = (tcV2 * edge1.y - tcV1 * edge2.y)  / (tcU1 * tcV2 - tcU2 * tcV1);
        tangent.z = (tcV2 * edge1.z - tcV1 * edge2.z)  / (tcU1 * tcV2 - tcU2 * tcV1);

        tempTangent.push_back(tangent);
    }

    //Compute vertex normals (normal Averaging)
    D3DXVECTOR4 normalSum(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR4 tangentSum(0.0f, 0.0f, 0.0f, 0.0f);
    int facesUsing = 0;
    float tX, tY, tZ;   //temp axis variables

    //Go through each vertex
    for(int i = 0; i < NumVertices; ++i)
    {
        //Check which triangles use this vertex
        for(int j = 0; j < NumFaces; ++j)
        {
            if(indices[j*3] == i ||
                indices[(j*3)+1] == i ||
                indices[(j*3)+2] == i)
            {
                tX = normalSum.x + tempNormal[j].x;
                tY = normalSum.y + tempNormal[j].y;
                tZ = normalSum.z + tempNormal[j].z;

                normalSum = D3DXVECTOR4(tX, tY, tZ, 0.0f);  //If a face is using the vertex, add the unormalized face normal to the normalSum
    
                facesUsing++;
            }
        }
        //Get the actual normal by dividing the normalSum by the number of faces sharing the vertex
        normalSum = normalSum / facesUsing;

		facesUsing = 0;
        //Check which triangles use this vertex
        for(int j = 0; j < NumFaces; ++j)
        {
            if(indices[j*3] == i ||
                indices[(j*3)+1] == i ||
                indices[(j*3)+2] == i)
            {
   
                //We can reuse tX, tY, tZ to sum up tangents
                tX = tangentSum.x + tempTangent[j].x;
                tY = tangentSum.y + tempTangent[j].y;
                tZ = tangentSum.z + tempTangent[j].z;

                tangentSum = D3DXVECTOR4(tX, tY, tZ, 0.0f); //sum up face tangents using this vertex

                facesUsing++;
            }
        }
        //Get the actual normal by dividing the normalSum by the number of faces sharing the vertex
        tangentSum = tangentSum / facesUsing;

        //Normalize the normalSum vector and tangent
		D3DXVec4Normalize(&normalSum, &normalSum);
		D3DXVec4Normalize(&tangentSum, &tangentSum);

        //Store the normal and tangent in our current vertex
        v[i].normal.x = normalSum.x;
        v[i].normal.y = normalSum.y;
        v[i].normal.z = normalSum.z;

        v[i].tangent.x = tangentSum.x;
        v[i].tangent.y = tangentSum.y;
        v[i].tangent.z = tangentSum.z;

		D3DXVECTOR3 bit;

		D3DXVec3Cross(&bit, &v[i].normal, &v[i].tangent);
		v[i].bitangent = -1.0 * bit;

        //Clear normalSum, tangentSum and facesUsing for next vertex
        normalSum = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
        tangentSum = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
        facesUsing = 0;
    }

	////terrain AABB
	//MinX =  -1.0 * dx * (hminfo.terrainWidth - 1)/2;
	//MinY = 0.0;
	//MinZ =  -1.0 * dz * (hminfo.terrainHeight - 1)/2;
	//MaxX =  dx * (hminfo.terrainWidth - 1)/2;
	//MaxY = dy;
	//MaxZ =  dz * (hminfo.terrainHeight - 1)/2;

 	return true;
}