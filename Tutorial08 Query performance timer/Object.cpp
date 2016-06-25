#include "Object.h"


Object::Object(void)
{
}


Object::~Object(void)
{
}


bool Object::LoadObjModel(ID3D10Device* device,
    std::string Filename, 
    TextureManager& TexMgr,
    bool IsRHCoordSys)
{
    HRESULT hr = 0;
	Subsets = 0;
    std::ifstream fileIn (Filename.c_str());   // Open file
    std::string meshMatLib;                    // String to hold our obj material library filename (model.mtl)

    // Arrays to store our model's information
    std::vector<D3DXVECTOR3> vertPos;
    std::vector<D3DXVECTOR3> vertNorm;
    std::vector<D3DXVECTOR3> vertTang;
    std::vector<D3DXVECTOR3> vertBiTang;
    std::vector<D3DXVECTOR2> vertTexCoord;
    std::vector<std::string> meshMaterials;

	vector<MySimpleVertex> l_stlMySimpleVerts;
	vector<SSimpleVertex> l_stlSimpleVerts;
	vector<SSimpleFace> l_stlSimpleFaces;

    // Make sure we have a default if no tex coords or normals are defined
    bool hasTexCoord = false;
    bool hasNorm = false;

    // Temp variables to store into vectors
    std::string meshMaterialsTemp;
    int vertPosIndexTemp;
    int vertNormIndexTemp;
    int vertTCIndexTemp;

    char checkChar;      // The variable we will use to store one char from file at a time
    std::string face;      // Holds the string containing our face vertices
    int vIndex = 0;         // Keep track of our vertex index count
    int triangleCount = 0;  // Total Triangles
    int totalVerts = 0;
    int meshTriangles = 0;
    bool ang = false;

    //Check to see if the file was opened
    if (fileIn)
    {
        while(fileIn)
        {           
            checkChar = fileIn.get();   //Get next char

            switch (checkChar)
            {       
                // A comment. Skip rest of the line
            case '#':
                checkChar = fileIn.get();
                while(checkChar != '\n')
                    checkChar = fileIn.get();
                break;

                    // Get Vertex Descriptions;
            case 'v':
                checkChar = fileIn.get();
                if(checkChar == ' ')  // v - vert position
                {
                    float vz, vy, vx;
                    fileIn >> vx >> vy >> vz;   // Store the next three types

                    if(IsRHCoordSys)            // If model is from an RH Coord System
                        vertPos.push_back(D3DXVECTOR3( vx, vy, vz * -1.0f));   // Invert the Z axis
                    else
                        vertPos.push_back(D3DXVECTOR3( vx, vy, vz));
                }
                if(checkChar == 't')  // vt - vert tex coords
                {           
                    float vtcu, vtcv;
                    fileIn >> vtcu >> vtcv;     // Store next two types
                    vertTexCoord.push_back(D3DXVECTOR2(vtcu, vtcv));   
                    hasTexCoord = true;         // We know the model uses texture coords
                }
                if(checkChar == 'n')          // vn - vert normal
                {
                    float vnx, vny, vnz;
                    fileIn >> vnx >> vny >> vnz;    // Store next three types

                    if(IsRHCoordSys)                // If model is from an RH Coord System
                        vertNorm.push_back(D3DXVECTOR3( vnx, vny, vnz * -1.0f ));  // Invert the Z axis
                    else
                        vertNorm.push_back(D3DXVECTOR3( vnx, vny, vnz ));  

                    hasNorm = true;                 // We know the model defines normals
                }
                break;

                // New group (Subset)
            case 'g': // g - defines a group
                checkChar = fileIn.get();
                if(checkChar == ' ')
                {
                    ang = true;
                    SubsetIndexStart.push_back(vIndex);       // Start index for this subset
                    Subsets++;
                }
                break;

                // Get Face Index
            case 'f': // f - defines the faces
                checkChar = fileIn.get();
                if(checkChar == ' ')
                {
                    face = "";               // Holds all vertex definitions for the face (eg. "111 222 333")
                    std::string VertDef;   // Holds one vertex definition at a time (eg. "111")
                    triangleCount = 0;      // Keep track of the triangles for this face, since we will have to 
                                            // "retriangulate" faces with more than 3 sides

                    checkChar = fileIn.get();
                    while(checkChar != '\n')
                    {
                        face += checkChar;          // Add the char to our face string
                        checkChar = fileIn.get();   // Get the next Character
                        if(checkChar == ' ')      // If its a space...
                            triangleCount++;        // Increase our triangle count
                    }

                    // Check for space at the end of our face string
                    if(face[face.length()-1] == ' ')
                        triangleCount--;    // Each space adds to our triangle count

                    triangleCount -= 1;     // Ever vertex in the face AFTER the first two are new faces

                    std::stringstream ss(face);

                    if(face.length() > 0)
                    {
                        int firstVIndex, lastVIndex;    // Holds the first and last vertice's index

                        for(int i = 0; i < 3; ++i)      // First three vertices (first triangle)
                        {
                            ss >> VertDef;  // Get vertex definition (vPos/vTexCoord/vNorm)

                            std::string vertPart;
                            int whichPart = 0;      // (vPos, vTexCoord, or vNorm)

                            // Parse this string
                            for(int j = 0; j < VertDef.length(); ++j)
                            {
                                if(VertDef[j] != '/') // If there is no divider "/", add a char to our vertPart
                                    vertPart += VertDef[j];

                                // If the current char is a divider "/", or its the last character in the string
                                if(VertDef[j] == '/' || j ==  VertDef.length()-1)
                                {
                                    std::istringstream stringToInt(vertPart); // Used to convert wstring to int

                                    if(whichPart == 0)  // If it's the vPos
                                    {
                                        stringToInt >> vertPosIndexTemp;
                                        vertPosIndexTemp -= 1;      // subtract one since c++ arrays start with 0, and obj start with 1

                                        // Check to see if the vert pos was the only thing specified
                                        if(j == VertDef.length()-1)
                                        {
                                            vertNormIndexTemp = 0;
                                            vertTCIndexTemp = 0;
                                        }
                                    }

                                    else if(whichPart == 1) // If vTexCoord
                                    {
                                        if(vertPart != "")   // Check to see if there even is a tex coord
                                        {
                                            stringToInt >> vertTCIndexTemp;
                                            vertTCIndexTemp -= 1;   // subtract one since c++ arrays start with 0, and obj start with 1
                                        }
                                        else    // If there is no tex coord, make a default
                                            vertTCIndexTemp = 0;

                                        // If the cur. char is the second to last in the string, then
                                        // there must be no normal, so set a default normal
                                        if(j == VertDef.length()-1)
                                            vertNormIndexTemp = 0;

                                    }                               
                                    else if(whichPart == 2) // If vNorm
                                    {
                                        std::istringstream stringToInt(vertPart);

                                        stringToInt >> vertNormIndexTemp;
                                        vertNormIndexTemp -= 1;     // subtract one since c++ arrays start with 0, and obj start with 1
                                    }

                                    vertPart = "";   // Get ready for next vertex part
                                    whichPart++;    // Move on to next vertex part                  
                                }
                            }

                            // Check to make sure there is at least one subset
                            if(Subsets == 0)
                            {
                                SubsetIndexStart.push_back(vIndex);       // Start index for this subset
                                Subsets++;
                            }

							MySimpleVertex tempVert;
							tempVert.pos = vertPosIndexTemp;
							tempVert.norm = vertNormIndexTemp;
							tempVert.texc = vertTCIndexTemp;
							l_stlMySimpleVerts.push_back(tempVert); //trzy kolejne to face

							SSimpleVertex tempSVert;
							tempSVert.pos = vertPos[vertPosIndexTemp];
							tempSVert.norm = vertNorm[vertNormIndexTemp];
							tempSVert.texc = vertTexCoord[vertTCIndexTemp];
							l_stlSimpleVerts.push_back(tempSVert); //trzy kolejne to face

                            totalVerts++;   // We created a new vertex
                            vIndex++;   // Increment index count
                        }
						//zapis face
						SSimpleFace tempFace;
						tempFace.iNrVerts = 3;
						tempFace.iVertOffs = meshTriangles * 3;
						l_stlSimpleFaces.push_back(tempFace);
                        meshTriangles++;    // One triangle down
                    }
                }
                break;

            case 'm': // mtllib - material library filename
                checkChar = fileIn.get();
                if(checkChar == 't')
                {
                    checkChar = fileIn.get();
                    if(checkChar == 'l')
                    {
                        checkChar = fileIn.get();
                        if(checkChar == 'l')
                        {
                            checkChar = fileIn.get();
                            if(checkChar == 'i')
                            {
                                checkChar = fileIn.get();
                                if(checkChar == 'b')
                                {
                                    checkChar = fileIn.get();
                                    if(checkChar == ' ')
                                    {
                                        // Store the material libraries file name
                                        fileIn >> meshMatLib;
                                    }
                                }
                            }
                        }
                    }
                }

                break;

            case 'u': // usemtl - which material to use
                checkChar = fileIn.get();
                if(checkChar == 's')
                {
                    checkChar = fileIn.get();
                    if(checkChar == 'e')
                    {
                        checkChar = fileIn.get();
                        if(checkChar == 'm')
                        {
                            checkChar = fileIn.get();
                            if(checkChar == 't')
                            {
                                checkChar = fileIn.get();
                                if(checkChar == 'l')
                                {
                                    checkChar = fileIn.get();
                                    if(checkChar == ' ')
                                    {
                                        meshMaterialsTemp = "";  // Make sure this is cleared

                                        fileIn >> meshMaterialsTemp; // Get next type (string)

                                        meshMaterials.push_back(meshMaterialsTemp);
                                        if(!ang)
                                        {
                                            SubsetIndexStart.push_back(vIndex);       // Start index for this subset
                                            Subsets++;
                                        }
                                        ang = false;
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            default:                
                break;
            }
        }
    }
    else    // If we could not open the file
	{

        return false;
    }

    SubsetIndexStart.push_back(vIndex); // There won't be another index start after our last subset, so set it here

    // sometimes "g" is defined at the very top of the file, then again before the first group of faces.
    // This makes sure the first subset does not conatain "0" indices.
    if(SubsetIndexStart[1] == 0)
    {
        SubsetIndexStart.erase(SubsetIndexStart.begin()+1);
        Subsets--;
    }

    // Make sure we have a default for the tex coord and normal
    // if one or both are not specified
    if(!hasNorm)
        vertNorm.push_back(D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    if(!hasTexCoord)
        vertTexCoord.push_back(D3DXVECTOR2(0.0f, 0.0f));

    // Close the obj file, and open the mtl file
    fileIn.close();
    fileIn.open(meshMatLib.c_str());

    std::string lastStringRead;
    int matCount = material.size(); // Total materials

    if (fileIn)
    {
        while(fileIn)
        {
            checkChar = fileIn.get();   // Get next char

            switch (checkChar)
            {
                // Check for comment
            case '#':
                checkChar = fileIn.get();
                while(checkChar != '\n')
                    checkChar = fileIn.get();
                break;

                // Set the colors
            case 'K':
                checkChar = fileIn.get();
                if(checkChar == 'd')  // Diffuse Color
                {
                    checkChar = fileIn.get();   // Remove space

                    fileIn >> material[matCount-1].Diffuse.x;
                    fileIn >> material[matCount-1].Diffuse.y;
                    fileIn >> material[matCount-1].Diffuse.z;
                }

                if(checkChar == 'a')  // Ambient Color
                {                   
                    checkChar = fileIn.get();   // Remove space

                    fileIn >> material[matCount-1].Ambient.x;
                    fileIn >> material[matCount-1].Ambient.y;
                    fileIn >> material[matCount-1].Ambient.z;
                }

                if(checkChar == 's')  // Ambient Color
                {                   
                    checkChar = fileIn.get();   // Remove space

                    fileIn >> material[matCount-1].Specular.x;
                    fileIn >> material[matCount-1].Specular.y;
                    fileIn >> material[matCount-1].Specular.z;
                }
                break;
                
            case 'N':
                checkChar = fileIn.get();

                if(checkChar == 's')  // Specular Power (Coefficient)
                {                   
                    checkChar = fileIn.get();   // Remove space

                    fileIn >> material[matCount-1].Specular.w;
                }

                break;

                // Check for transparency
            case 'T':
                checkChar = fileIn.get();
                if(checkChar == 'r')
                {
                    checkChar = fileIn.get();   // Remove space
                    float Transparency;
                    fileIn >> Transparency;

                    material[matCount-1].Diffuse.w = Transparency;

                    if(Transparency > 0.0f)
                        material[matCount-1].IsTransparent = true;
                }
                break;

                // Some obj files specify d for transparency
            case 'd':
                checkChar = fileIn.get();
                if(checkChar == ' ')
                {
                    float Transparency;
                    fileIn >> Transparency;

                    // 'd' - 0 being most transparent, and 1 being opaque, opposite of Tr
                    Transparency = 1.0f - Transparency;

                    material[matCount-1].Diffuse.w = Transparency;

                    if(Transparency > 0.0f)
                        material[matCount-1].IsTransparent = true;                  
                }
                break;

                // Get the diffuse map (texture)
            case 'm':
                checkChar = fileIn.get();
                if(checkChar == 'a')
                {
                    checkChar = fileIn.get();
                    if(checkChar == 'p')
                    {
                        checkChar = fileIn.get();
                        if(checkChar == '_')
                        {
                            // map_Kd - Diffuse map
                            checkChar = fileIn.get();
                            if(checkChar == 'K')
                            {
                                checkChar = fileIn.get();
                                if(checkChar == 'd')
                                {
                                    std::string fileNamePath;

                                    fileIn.get();   // Remove whitespace between map_Kd and file

                                    // Get the file path - We read the pathname char by char since
                                    // pathnames can sometimes contain spaces, so we will read until
                                    // we find the file extension
                                    bool texFilePathEnd = false;
                                    while(!texFilePathEnd)
                                    {
                                        checkChar = fileIn.get();

                                        fileNamePath += checkChar;

                                        if(checkChar == '.')
                                        {
                                            for(int i = 0; i < 3; ++i)
                                                fileNamePath += fileIn.get();

                                            texFilePathEnd = true;
                                        }                           
                                    }

                                    //check if this texture has already been loaded
                                    bool alreadyLoaded = false;
                                    for(int i = 0; i < TexMgr.TextureNameArray.size(); ++i)
                                    {
                                        if(fileNamePath == TexMgr.TextureNameArray[i])
                                        {
                                            alreadyLoaded = true;
                                            material[matCount-1].DiffuseTextureID = i;
                                            material[matCount-1].HasDiffTexture = true;
                                        }
                                    }

                                    //if the texture is not already loaded, load it now
                                    if(!alreadyLoaded)
                                    {
                                        ID3D10ShaderResourceView* tempSRV;

										std::wstring stemp = std::wstring(fileNamePath.begin(), fileNamePath.end());
										LPCWSTR sw = stemp.c_str();

										hr = D3DX10CreateShaderResourceViewFromFile( device, sw, NULL, NULL, &tempSRV, NULL );
                                        if(SUCCEEDED(hr))
                                        {
                                            TexMgr.TextureNameArray.push_back(fileNamePath.c_str());
                                            material[matCount-1].DiffuseTextureID = TexMgr.TextureList.size();
                                            TexMgr.TextureList.push_back(tempSRV);
                                            material[matCount-1].HasDiffTexture = true;
                                        }
                                    }   
                                }

                                // Get Ambient Map (texture)
                                if(checkChar == 'a')
                                {
                                    std::string fileNamePath;

                                    fileIn.get();   // Remove whitespace between map_Kd and file

                                    // Get the file path - We read the pathname char by char since
                                    // pathnames can sometimes contain spaces, so we will read until
                                    // we find the file extension
                                    bool texFilePathEnd = false;
                                    while(!texFilePathEnd)
                                    {
                                        checkChar = fileIn.get();

                                        fileNamePath += checkChar;

                                        if(checkChar == '.')
                                        {
                                            for(int i = 0; i < 3; ++i)
                                                fileNamePath += fileIn.get();

                                            texFilePathEnd = true;
                                        }                           
                                    }

                                    //check if this texture has already been loaded
                                    bool alreadyLoaded = false;
                                    for(int i = 0; i < TexMgr.TextureNameArray.size(); ++i)
                                    {
                                        if(fileNamePath == TexMgr.TextureNameArray[i])
                                        {
                                            alreadyLoaded = true;
                                            material[matCount-1].AmbientTextureID = i;
                                            material[matCount-1].HasAmbientTexture = true;
                                        }
                                    }

                                    //if the texture is not already loaded, load it now
                                    if(!alreadyLoaded)
                                    {
                                        ID3D10ShaderResourceView* tempSRV;
										std::wstring stemp = std::wstring(fileNamePath.begin(), fileNamePath.end());
										LPCWSTR sw = stemp.c_str();

										hr = D3DX10CreateShaderResourceViewFromFile( device, sw, NULL, NULL, &tempSRV, NULL );
                                        if(SUCCEEDED(hr))
                                        {
                                            TexMgr.TextureNameArray.push_back(fileNamePath.c_str());
                                            material[matCount-1].AmbientTextureID = TexMgr.TextureList.size();
                                            TexMgr.TextureList.push_back(tempSRV);
                                            material[matCount-1].HasAmbientTexture = true;
                                        }
                                    }   
                                }

                                // Get Specular Map (texture)
                                if(checkChar == 's')
                                {
                                    std::string fileNamePath;

                                    fileIn.get();   // Remove whitespace between map_Ks and file

                                    // Get the file path - We read the pathname char by char since
                                    // pathnames can sometimes contain spaces, so we will read until
                                    // we find the file extension
                                    bool texFilePathEnd = false;
                                    while(!texFilePathEnd)
                                    {
                                        checkChar = fileIn.get();

                                        fileNamePath += checkChar;

                                        if(checkChar == '.')
                                        {
                                            for(int i = 0; i < 3; ++i)
                                                fileNamePath += fileIn.get();

                                            texFilePathEnd = true;
                                        }                           
                                    }

                                    //check if this texture has already been loaded
                                    bool alreadyLoaded = false;
                                    for(int i = 0; i < TexMgr.TextureNameArray.size(); ++i)
                                    {
                                        if(fileNamePath == TexMgr.TextureNameArray[i])
                                        {
                                            alreadyLoaded = true;
                                            material[matCount-1].SpecularTextureID = i;
                                            material[matCount-1].HasSpecularTexture = true;
                                        }
                                    }

                                    //if the texture is not already loaded, load it now
                                    if(!alreadyLoaded)
                                    {
                                        ID3D10ShaderResourceView* tempSRV;
										std::wstring stemp = std::wstring(fileNamePath.begin(), fileNamePath.end());
										LPCWSTR sw = stemp.c_str();

										hr = D3DX10CreateShaderResourceViewFromFile( device, sw, NULL, NULL, &tempSRV, NULL );
                                        if(SUCCEEDED(hr))
                                        {
                                            TexMgr.TextureNameArray.push_back(fileNamePath.c_str());
                                            material[matCount-1].SpecularTextureID = TexMgr.TextureList.size();
                                            TexMgr.TextureList.push_back(tempSRV);
                                            material[matCount-1].HasSpecularTexture = true;
                                        }
                                    }   
                                }
                            }
                            
                            //map_d - alpha map
                            else if(checkChar == 'd')
                            {
                                std::string fileNamePath;

                                fileIn.get();   // Remove whitespace between map_Ks and file

                                // Get the file path - We read the pathname char by char since
                                // pathnames can sometimes contain spaces, so we will read until
                                // we find the file extension
                                bool texFilePathEnd = false;
                                while(!texFilePathEnd)
                                {
                                    checkChar = fileIn.get();

                                    fileNamePath += checkChar;

                                    if(checkChar == '.')
                                    {
                                        for(int i = 0; i < 3; ++i)
                                            fileNamePath += fileIn.get();

                                        texFilePathEnd = true;
                                    }                           
                                }

                                //check if this texture has already been loaded
                                bool alreadyLoaded = false;
                                for(int i = 0; i < TexMgr.TextureNameArray.size(); ++i)
                                {
                                    if(fileNamePath == TexMgr.TextureNameArray[i])
                                    {
                                        alreadyLoaded = true;
                                        material[matCount-1].AlphaTextureID = i;
                                        material[matCount-1].IsTransparent = true;
                                    }
                                }

                                //if the texture is not already loaded, load it now
                                if(!alreadyLoaded)
                                {
                                    ID3D10ShaderResourceView* tempSRV;
										std::wstring stemp = std::wstring(fileNamePath.begin(), fileNamePath.end());
										LPCWSTR sw = stemp.c_str();

										hr = D3DX10CreateShaderResourceViewFromFile( device, sw, NULL, NULL, &tempSRV, NULL );
                                    if(SUCCEEDED(hr))
                                    {
                                        TexMgr.TextureNameArray.push_back(fileNamePath.c_str());
                                        material[matCount-1].AlphaTextureID = TexMgr.TextureList.size();
                                        TexMgr.TextureList.push_back(tempSRV);
                                        material[matCount-1].IsTransparent = true;
                                    }
                                }   
                            }

                            // map_bump - bump map (Normal Map)
                            else if(checkChar == 'B')
                            {
                                checkChar = fileIn.get();
                                if(checkChar == 'u')
                                {
                                    checkChar = fileIn.get();
                                    if(checkChar == 'm')
                                    {
                                        checkChar = fileIn.get();
                                        if(checkChar == 'p')
                                        {
                                            std::string fileNamePath;

                                            fileIn.get();   // Remove whitespace between map_bump and file

                                            // Get the file path - We read the pathname char by char since
                                            // pathnames can sometimes contain spaces, so we will read until
                                            // we find the file extension
                                            bool texFilePathEnd = false;
                                            while(!texFilePathEnd)
                                            {
                                                checkChar = fileIn.get();

                                                fileNamePath += checkChar;

                                                if(checkChar == '.')
                                                {
                                                    for(int i = 0; i < 3; ++i)
                                                        fileNamePath += fileIn.get();

                                                    texFilePathEnd = true;
                                                }                           
                                            }

                                            //check if this texture has already been loaded
                                            bool alreadyLoaded = false;
                                            for(int i = 0; i < TexMgr.TextureNameArray.size(); ++i)
                                            {
                                                if(fileNamePath == TexMgr.TextureNameArray[i])
                                                {
                                                    alreadyLoaded = true;
                                                    material[matCount-1].NormMapTextureID = i;
                                                    material[matCount-1].HasNormMap = true;
                                                }
                                            }

                                            //if the texture is not already loaded, load it now
                                            if(!alreadyLoaded)
                                            {
                                                ID3D10ShaderResourceView* tempSRV;
										std::wstring stemp = std::wstring(fileNamePath.begin(), fileNamePath.end());
										LPCWSTR sw = stemp.c_str();

										hr = D3DX10CreateShaderResourceViewFromFile( device, sw, NULL, NULL, &tempSRV, NULL );
                                                if(SUCCEEDED(hr))
                                                {
                                                    TexMgr.TextureNameArray.push_back(fileNamePath.c_str());
                                                    material[matCount-1].NormMapTextureID = TexMgr.TextureList.size();
                                                    TexMgr.TextureList.push_back(tempSRV);
                                                    material[matCount-1].HasNormMap = true;
                                                }
                                            }   
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            case 'n': // newmtl - Declare new material
                checkChar = fileIn.get();
                if(checkChar == 'e')
                {
                    checkChar = fileIn.get();
                    if(checkChar == 'w')
                    {
                        checkChar = fileIn.get();
                        if(checkChar == 'm')
                        {
                            checkChar = fileIn.get();
                            if(checkChar == 't')
                            {
                                checkChar = fileIn.get();
                                if(checkChar == 'l')
                                {
                                    checkChar = fileIn.get();
                                    if(checkChar == ' ')
                                    {
                                        // New material, set its defaults
                                        SurfaceMaterial tempMat;
                                        material.push_back(tempMat);
                                        fileIn >> material[matCount].MatName;
                                        material[matCount].IsTransparent = false;
                                        material[matCount].HasDiffTexture = false;
                                        material[matCount].HasAmbientTexture = false;
                                        material[matCount].HasSpecularTexture = false;
                                        material[matCount].HasAlphaTexture = false;
                                        material[matCount].HasNormMap = false;
                                        material[matCount].NormMapTextureID = 0;
                                        material[matCount].DiffuseTextureID = 0;
                                        material[matCount].AlphaTextureID = 0;
                                        material[matCount].SpecularTextureID = 0;
                                        material[matCount].AmbientTextureID = 0;
                                        material[matCount].Specular = D3DXVECTOR4(0,0,0,0);
                                        material[matCount].Ambient = D3DXVECTOR3(0,0,0);
                                        material[matCount].Diffuse = D3DXVECTOR4(0,0,0,0);
                                        matCount++;
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            default:
                break;
            }
        }
    }   
    else    // If we could not open the material library
    {
        return false;
    }

    // Set the subsets material to the index value
    // of the its material in our material array
    for(int i = 0; i < Subsets; ++i)
    {
        bool hasMat = false;
        for(int j = 0; j < material.size(); ++j)
        {
            if(meshMaterials[i] == material[j].MatName)
            {
                SubsetMaterialID.push_back(j);
                hasMat = true;
            }
        }
        if(!hasMat)
            SubsetMaterialID.push_back(0); // Use first material in array
    }

//Calc Tangent Basis MikkTS

	    CSimpleMesh simple_mesh ( l_stlSimpleVerts, l_stlSimpleFaces, meshTriangles, false );

	    // Generate tangent spaces. Set all six call-backs
        SMikkTSpaceInterface sInterface;
	    memset ( reinterpret_cast<void*>(&sInterface), 0, sizeof(SMikkTSpaceInterface) );

	    sInterface.m_getNumFaces = getNumFaces;
	    sInterface.m_getNumVerticesOfFace = getNumVerticesOfFace;
	    sInterface.m_getPosition = getPosition;
	    sInterface.m_getNormal = getNormal;
	    sInterface.m_getTexCoord = getTexCoord;
	    sInterface.m_setTSpace = setTSpace;
	
        SMikkTSpaceContext sContext;
	    memset ( reinterpret_cast<void*>(&sContext), 0, sizeof(SMikkTSpaceContext) );
	    sContext.m_pUserData = reinterpret_cast<void*>(&simple_mesh);
	    sContext.m_pInterface = &sInterface;

	    const bool lc_bRes = ( genTangSpaceDefault(&sContext)!=0 );
        if ( lc_bRes )
	    {
/*
			FILE* pliczeka;
			D3DXVECTOR3 tv;
			D3DXVECTOR2 tu;
			pliczeka = fopen("NMObject.txt","w");
			for (int i = 0; i < l_stlSimpleVerts.size(); i++)
			{
				fprintf(pliczeka, "V%d:\n",i);
				tv = l_stlSimpleVerts[i].pos;
				fprintf(pliczeka, "%f %f %f\n", tv.x, tv.y, tv.z);
				tv = l_stlSimpleVerts[i].norm;
				fprintf(pliczeka, "%f %f %f\n", tv.x, tv.y, tv.z);
				tv = l_stlSimpleVerts[i].tang;
				fprintf(pliczeka, "%f %f %f\n", tv.x, tv.y, tv.z);
				tv = l_stlSimpleVerts[i].bitang;
				fprintf(pliczeka, "%f %f %f\n", tv.x, tv.y, tv.z);
				tu = l_stlSimpleVerts[i].texc;
				fprintf(pliczeka, "%f %f\n", tu.x, tu.y);
			}
			fclose(pliczeka);
*/

			for (int i = 0; i < l_stlSimpleVerts.size(); i++)
			{
				D3DXVECTOR3 tempTan = l_stlSimpleVerts[i].tang;
				int j;
				for (j = 0; j < vertTang.size(); j++)
				{
					if (vertTang[j] == tempTan) break;
				}
				if (j >= vertTang.size()) 
				{
					vertTang.push_back(tempTan); //jeœli nie by³o w wektorze to na koniec
				}
				l_stlMySimpleVerts[i].tang = j;

				tempTan = l_stlSimpleVerts[i].bitang;
				for (j = 0; j < vertBiTang.size(); j++)
				{
					if (vertBiTang[j] == tempTan) break;
				}
				if (j >= vertBiTang.size()) 
				{
					vertBiTang.push_back(tempTan); //jeœli nie by³o w wektorze to na koniec
				}
				l_stlMySimpleVerts[i].bitang = j;
			}
		}
		else
		{
			return false;
		}
//make indexed geometry & GPU buffers

	vector<MySimpleVertex> tempMySimpleVerts;
	for (int i = 0; i < l_stlMySimpleVerts.size(); i++)
	{

		int j;
		for (j = 0; j < tempMySimpleVerts.size(); j++)
		{
			if ( (tempMySimpleVerts[j].pos == l_stlMySimpleVerts[i].pos) && (tempMySimpleVerts[j].norm == l_stlMySimpleVerts[i].norm)  && (tempMySimpleVerts[j].texc == l_stlMySimpleVerts[i].texc)  && (tempMySimpleVerts[j].tang == l_stlMySimpleVerts[i].tang)  && (tempMySimpleVerts[j].bitang == l_stlMySimpleVerts[i].bitang)  )
				break;
		}
		if (j >= tempMySimpleVerts.size()) 
		{
			tempMySimpleVerts.push_back(l_stlMySimpleVerts[i]); //jeœli nie by³o w wektorze to na koniec
		}
		Indices.push_back(j); //push indice
	}

	for (int i = 0; i < tempMySimpleVerts.size(); i++)
	{
		Vertex tempV;
		tempV.pos = vertPos[tempMySimpleVerts[i].pos];
		tempV.normal = vertNorm[tempMySimpleVerts[i].norm];
		tempV.texCoord = vertTexCoord[tempMySimpleVerts[i].texc];
		if(!IsRHCoordSys)            // If model is from an RH Coord System
			tempV.texCoord.y = 1.0 - tempV.texCoord.y;
		tempV.tangent = vertTang[tempMySimpleVerts[i].tang];
		tempV.bitangent = vertBiTang[tempMySimpleVerts[i].bitang];
		Vertices.push_back(tempV);
	}


    if(IsRHCoordSys) //faces exported ccw (zmieniamy tylko jeœli by³a transf RH->LH)
	{
		for (int i = 1; i < Indices.size(); i += 3)
		{
			int w = Indices[i];
			Indices[i] = Indices[i+1];
			Indices[i+1] = w;
		}
	}

    //Create index buffer
    D3D10_BUFFER_DESC indexBufferDesc;
    ZeroMemory( &indexBufferDesc, sizeof(indexBufferDesc) );

    indexBufferDesc.Usage = D3D10_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(DWORD) * meshTriangles*3;
    indexBufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    D3D10_SUBRESOURCE_DATA iinitData;

    iinitData.pSysMem = &Indices[0];
    device->CreateBuffer(&indexBufferDesc, &iinitData, &IndexBuff);

    //Create Vertex Buffer
    D3D10_BUFFER_DESC vertexBufferDesc;
    ZeroMemory( &vertexBufferDesc, sizeof(vertexBufferDesc) );

    vertexBufferDesc.Usage = D3D10_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof( Vertex ) * Vertices.size();
    vertexBufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D10_SUBRESOURCE_DATA vertexBufferData; 

    ZeroMemory( &vertexBufferData, sizeof(vertexBufferData) );
    vertexBufferData.pSysMem = &Vertices[0];
    hr = device->CreateBuffer( &vertexBufferDesc, &vertexBufferData, &VertBuff);

    return true;
}

void Object::Render(ID3D10Device* device,
    TextureManager& TexMgr,
	ID3D10EffectTechnique* technique,
	ID3D10InputLayout* vertexlayout,
	ID3D10EffectMatrixVariable* v1,
	ID3D10EffectMatrixVariable* v4,
	ID3D10EffectShaderResourceVariable* v2,
	ID3D10EffectShaderResourceVariable* v3,
	D3DXMATRIX* world)
{
	for (int op=0; op < Subsets; op++)
	{
    v1->SetMatrix( ( float* )world );

	D3DXMATRIX invworld;
	D3DXMatrixInverse( &invworld, NULL, world );

	v4->SetMatrix( ( float* )invworld );
	int TempMatID = SubsetMaterialID[op];
	v2->SetResource( TexMgr.TextureList[material[TempMatID].DiffuseTextureID] );
	v3->SetResource( TexMgr.TextureList[material[TempMatID].NormMapTextureID] );

    device->IASetInputLayout( vertexlayout );
    UINT stride = sizeof( Vertex );
    UINT offset = 0;
	device->IASetVertexBuffers( 0, 1, &VertBuff, &stride, &offset );
    device->IASetIndexBuffer( IndexBuff, DXGI_FORMAT_R32_UINT, 0 );
    device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    technique->GetPassByIndex( 0 )->Apply( 0 );
    device->DrawIndexed( SubsetIndexStart[op+1] - SubsetIndexStart[op], SubsetIndexStart[op], 0 ); 
	}
}


