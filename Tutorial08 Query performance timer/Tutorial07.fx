//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//--------------------------------------------------------------------------------------
	#define DepthBias 0.0001
	
	struct DirectionalLight
	{
		float4 color;
		float3 dir;
	};
 
	struct Material
	{
		float Ka, Kd, Ks, A;
	};

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
	Texture2D txColorMap;
	Texture2D txNormalMap;
	Texture2D txShadowMap;
	Texture2D txDepthMap;
	Texture2D txSpecularMap;
	Texture2D txAOMap;
		
	TextureCube txSkyBox1;
	TextureCube txSkyBox2;
	float SkyMix;

    matrix World;
    matrix View;
    matrix Projection;
	matrix InvWorld;
	matrix InvView;
	matrix InvProjection;

	matrix ViewProjection;
	matrix WorldViewProjection;

	matrix LightViewProjection;

	float3 CameraPos;
	float3 CameraUp;
	float3 CameraDir;
	float3 CameraRight;

	float2 shadowMapSize = float2(1024, 1024);
	
	float4 ambientLight;
	DirectionalLight light;
	Material material;

//--------------------------------------------------------------------------------------
// Shaders I/O structs
//-------------------------------------------------------------------------------------
struct NORMAL_MAP_VS_INPUT
{
    float4 Pos : POSITION; //Pozycja w Object-Space
    float2 Tex : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
};

struct NORMAL_MAP_PS_INPUT
{
    float4 Pos : SV_POSITION; //Pozycja 2D (po projekcji, przed homogenizacj¹ na wyjœciu VS, po homogenizacji na wejœciu PS)
	float4 WPos : POSITION; //Pozycja 3D w œwiecie (World-Space)
	float4 LPos : TEXCOORD0; //Pozycja 2D po projekcji z punktu widzenia œwiat³a (Light-Space)
	float4 OPos : TEXCOORD1; //Oryginalna pozycja 3D (Object-Space)
    float2 Tex : TEXCOORD2; //wsp. tekstur
	float3 N : TEXCOORD3; //Normal
	float3 T : TEXCOORD4; //Tangent
	float3 B : TEXCOORD5; //Bitangent
	float3 H : TEXCOORD6; //wektor po³ówkowy Blinn'a w Object-Space
	float3 V : TEXCOORD7; //dir to Light w Object-Space
	float3 H1 : TEXCOORD8; //H w Tangent-Space
	float3 V1 : TEXCOORD9; //V w Tangent-Space
	float3 FN : TEXCOORD10; //Face Normal (wyliczany przez shader geometrii)
};

struct SHADOW_VS_INPUT
{
    float4 Pos : POSITION;
};

struct SHADOW_PS_INPUT
{
    float4 Pos : SV_POSITION;
};

struct SKYBOX_VS_INPUT
{
	float4	Pos	: POSITION;
	float3	Tex	: TEXCOORD0;
};

struct SKYBOX_PS_INPUT
{
	float4 	Pos	: SV_POSITION;
	float3	Tex	: TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// States
//-------------------------------------------------------------------------------------
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState samVolume
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
};

SamplerState samPoint
{
    Filter = min_mag_mip_point;
    AddressU = MIRROR;
    AddressV = MIRROR;	
};

SamplerState samBilinear
{
    Filter = min_mag_mip_linear;
    AddressU = MIRROR;
    AddressV = MIRROR;	
};

SamplerState samCubeLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;	
};

SamplerComparisonState cmpSampler
{
   // sampler state
   Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
   AddressU = MIRROR;
   AddressV = MIRROR;
   ComparisonFunc = LESS_EQUAL; // sampler comparison state
};

DepthStencilState renderLess
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
	DEPTHFUNC = LESS;
};

DepthStencilState renderEqual
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
	DEPTHFUNC = LESS_EQUAL;
};

BlendState AlphaBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};

DepthStencilState DisableDepthWrite
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
};

//--------------------------------------------------------------------------------------
// SkyBox
//--------------------------------------------------------------------------------------
SKYBOX_PS_INPUT SkyBoxVS( SKYBOX_VS_INPUT input )
{
    SKYBOX_PS_INPUT output = (SKYBOX_PS_INPUT)0;
	input.Pos.xyz = input.Pos.xyz * float3(100.0f, 100.0f, 100.0f);
	output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection ).xyww; // trick: po homogenizacji do porównania z wartoœci¹ z-bufora pójdzie 1 (wartoœæ max)!
    output.Tex = input.Tex;

    return output;
}

float4 SkyBoxPS( SKYBOX_PS_INPUT input ) : SV_Target
{
	return lerp(txSkyBox1.Sample(samCubeLinear, normalize(input.Tex)), txSkyBox2.Sample(samCubeLinear, normalize(input.Tex)), SkyMix);
}

//--------------------------------------------------------------------------------------
// ShadowMap
//--------------------------------------------------------------------------------------
SHADOW_PS_INPUT ShadowMapVS( SHADOW_VS_INPUT input )
{
    SHADOW_PS_INPUT output = (SHADOW_PS_INPUT)0;;
    output.Pos = mul( input.Pos, mul( World, LightViewProjection ) );
    return output;
}

void ShadowMapPS( SHADOW_PS_INPUT input ) {} //tylko zapis do z-bufora

//--------------------------------------------------------------------------------------
// Blinn-Phong Lighting Reflection Model
//--------------------------------------------------------------------------------------
float4 calcBlinnPhongLighting( Material M, float4 LColor, float3 N, float3 L, float3 H )
{
    float4 Ia = M.Ka * ambientLight;
    float4 Id = M.Kd * saturate( dot(N,L) );
    float4 Is = M.Ks * pow( saturate(dot(N,H)), M.A );
 
    return Ia + (Id + Is) * LColor;
}

float2 texOffset( int u, int v )
{
    return float2( u * 1.0f/shadowMapSize.x, v * 1.0f/shadowMapSize.y );
}

//--------------------------------------------------------------------------------------
// Normal Mapping
//--------------------------------------------------------------------------------------
NORMAL_MAP_PS_INPUT NM_VS( NORMAL_MAP_VS_INPUT input )
{
    NORMAL_MAP_PS_INPUT output = (NORMAL_MAP_PS_INPUT)0;
    output.WPos = mul( input.Pos, World ); //position in World Space
    output.Pos = mul( output.WPos, ViewProjection );
    output.Tex = input.Tex;
    output.LPos = mul( output.WPos, LightViewProjection ); //Pos 2D from the light point of view
	output.OPos = input.Pos;

    output.N = input.Normal;	//TBN in Object Space
	output.T = input.Tangent;
	output.B = input.Bitangent;

	float3 LD = -light.dir; //dla s³oñca jest ok, dla bliskich Ÿróde³ œwiat³a robimy podobnie jak poni¿ej
    float3 V = CameraPos - output.WPos.xyz;

    output.H = mul(LD + V, InvWorld); //Blinn's H in Object Space
	output.V = mul(LD, InvWorld); //light dir in Object Space 

	float3x3 tbn = float3x3( input.Tangent, input.Bitangent, input.Normal ); //tbn przenosi z TS do OS => transpose(tbn) w drug¹ stronê (tbn jest macierz¹ ortonormaln¹ => odwrotnoœæ = transpozycja)
	output.V1 = mul(tbn, output.V); //light dir to tangent space (mno¿enie v*transpose(M) = M*v [kolejnoœæ macierz-wektor])
    output.H1 = mul(tbn, output.H); //Blinn's H in tangent space

	return output;
}

float4 TEX_PS( NORMAL_MAP_PS_INPUT input) : SV_Target
{
    //re-homogenize position after interpolation
    input.LPos.xyz /= input.LPos.w;

    //if position is not visible to the light - dont illuminate it
    //results in hard light frustum
    if( input.LPos.x < -1.0f || input.LPos.x > 1.0f ||
        input.LPos.y < -1.0f || input.LPos.y > 1.0f ||
        input.LPos.z < 0.0f  || input.LPos.z > 1.0f ) return txColorMap.Sample( samLinear, input.Tex ) * 0.2f;

     //transform clip space coords to texture space coords (-1:1 to 0:1)
    input.LPos.x = input.LPos.x/2 + 0.5;
    input.LPos.y = input.LPos.y/-2 + 0.5;
	input.LPos.z -= DepthBias; //DepthBias

	//----- hardware PCF - single texel
    float shadowFactor = txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy, input.LPos.z );

    ////-----16xPCF sampling for shadow map
    //float sum = 0;
    //float x, y;
 
    ////perform PCF filtering on a 4 x 4 texel neighborhood
    //for (y = -1.5; y <= 1.5; y += 1.0)
    //{
    //    for (x = -1.5; x <= 1.5; x += 1.0)
    //    {
    //        sum += txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy + texOffset(x,y), input.LPos.z );
    //    }
    //}
    //float shadowFactor = sum / 16.0;

//Object-Space Lightning:
	float4 I = calcBlinnPhongLighting( material, light.color, normalize( input.FN ), normalize( input.V ),  normalize( input.H ) ); //cieniowanie p³askie
	//float4 I = calcBlinnPhongLighting( material, light.color, normalize( input.N ), normalize( input.V ),  normalize( input.H ) ); //interpolacja wektorów normalnych
    return shadowFactor * txColorMap.Sample( samLinear, input.Tex ) * I;
}

float4 NM1_PS( NORMAL_MAP_PS_INPUT input) : SV_Target
{
    //re-homogenize position after interpolation
    input.LPos.xyz /= input.LPos.w;

    //if position is not visible to the light - dont illuminate it
    //results in hard light frustum
    if( input.LPos.x < -1.0f || input.LPos.x > 1.0f ||
        input.LPos.y < -1.0f || input.LPos.y > 1.0f ||
        input.LPos.z < 0.0f  || input.LPos.z > 1.0f ) return txColorMap.Sample( samLinear, input.Tex ) * 0.2f;

     //transform clip space coords to texture space coords (-1:1 to 0:1)
    input.LPos.x = input.LPos.x/2 + 0.5;
    input.LPos.y = input.LPos.y/-2 + 0.5;
	input.LPos.z -= DepthBias; //DepthBias

	//----- hardware PCF - single texel
    float shadowFactor = txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy, input.LPos.z );

    ////-----16xPCF sampling for shadow map
    //float sum = 0;
    //float x, y;
 
    ////perform PCF filtering on a 4 x 4 texel neighborhood
    //for (y = -1.5; y <= 1.5; y += 1.0)
    //{
    //    for (x = -1.5; x <= 1.5; x += 1.0)
    //    {
    //        sum += txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy + texOffset(x,y), input.LPos.z );
    //    }
    //}
    //float shadowFactor = sum / 16.0;

	float4 txNorm = txNormalMap.Sample( samLinear, input.Tex ); //Sampled Normal in Tangent Space
	float4 atNorm = 2.0 * txNorm - 1.0;
	float3 tNorm = atNorm.xyz;

    float3 OSN = normalize(tNorm.x * input.T + tNorm.y * input.B + tNorm.z * input.N); //tNorm in Object Space (TS2OS)

	//Object-Space Lightning:
	float4 I = calcBlinnPhongLighting( material, light.color, OSN, normalize( input.V ),  normalize( input.H ) );
    return shadowFactor * txColorMap.Sample( samLinear, input.Tex ) * I;
}

float4 NM2_PS( NORMAL_MAP_PS_INPUT input) : SV_Target //Object-Space Normal Map:
{
    //re-homogenize position after interpolation
    input.LPos.xyz /= input.LPos.w;

    //if position is not visible to the light - dont illuminate it
    //results in hard light frustum
    if( input.LPos.x < -1.0f || input.LPos.x > 1.0f ||
        input.LPos.y < -1.0f || input.LPos.y > 1.0f ||
        input.LPos.z < 0.0f  || input.LPos.z > 1.0f ) return txColorMap.Sample( samLinear, input.Tex ) * 0.2f;

     //transform clip space coords to texture space coords (-1:1 to 0:1)
    input.LPos.x = input.LPos.x/2 + 0.5;
    input.LPos.y = input.LPos.y/-2 + 0.5;
	input.LPos.z -= DepthBias; //DepthBias

	//----- hardware PCF - single texel
    float shadowFactor = txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy, input.LPos.z );

    ////-----16xPCF sampling for shadow map
    //float sum = 0;
    //float x, y;
 
    ////perform PCF filtering on a 4 x 4 texel neighborhood
    //for (y = -1.5; y <= 1.5; y += 1.0)
    //{
    //    for (x = -1.5; x <= 1.5; x += 1.0)
    //    {
    //        sum += txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy + texOffset(x,y), input.LPos.z );
    //    }
    //}
    //float shadowFactor = sum / 16.0;

	float4 txNorm = txNormalMap.Sample( samLinear, input.Tex ); //Sampled Normal in Object Space
	float4 atNorm = 2.0 * txNorm - 1.0;
	float3 tNorm = atNorm.xyz;
	//Object-Space Lightning:
	float4 I = calcBlinnPhongLighting( material, light.color, tNorm, normalize( input.V ),  normalize( input.H ) );
    return shadowFactor * txColorMap.Sample( samLinear, input.Tex ) * I;
}

float4 NM3_PS( NORMAL_MAP_PS_INPUT input) : SV_Target
{
    //re-homogenize position after interpolation
    input.LPos.xyz /= input.LPos.w;

    //if position is not visible to the light - dont illuminate it
    //results in hard light frustum
    if( input.LPos.x < -1.0f || input.LPos.x > 1.0f ||
        input.LPos.y < -1.0f || input.LPos.y > 1.0f ||
        input.LPos.z < 0.0f  || input.LPos.z > 1.0f ) return txColorMap.Sample( samLinear, input.Tex ) * 0.2f;

     //transform clip space coords to texture space coords (-1:1 to 0:1)
    input.LPos.x = input.LPos.x/2 + 0.5;
    input.LPos.y = input.LPos.y/-2 + 0.5;
	input.LPos.z -= DepthBias; //DepthBias

	//----- hardware PCF - single texel
    float shadowFactor = txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy, input.LPos.z );

    ////-----16xPCF sampling for shadow map
    //float sum = 0;
    //float x, y;
 
    ////perform PCF filtering on a 4 x 4 texel neighborhood
    //for (y = -1.5; y <= 1.5; y += 1.0)
    //{
    //    for (x = -1.5; x <= 1.5; x += 1.0)
    //    {
    //        sum += txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy + texOffset(x,y), input.LPos.z );
    //    }
    //}
    //float shadowFactor = sum / 16.0;

	float4 txNorm = txNormalMap.Sample( samLinear, input.Tex ); //Sampled Normal in Tangent Space
	float4 atNorm = 2.0 * txNorm - 1.0;
	float3 tNorm = normalize(atNorm.xyz);

	//Tangent-Space Lightning:
	float4 I = calcBlinnPhongLighting( material, light.color, tNorm, normalize( input.V1 ), normalize( input.H1 ) );
    return shadowFactor * txColorMap.Sample( samLinear, input.Tex ) * I;
}

float4 NM4_PS( NORMAL_MAP_PS_INPUT input) : SV_Target
{
    //re-homogenize position after interpolation
    input.LPos.xyz /= input.LPos.w;

    //if position is not visible to the light - dont illuminate it
    //results in hard light frustum
    if( input.LPos.x < -1.0f || input.LPos.x > 1.0f ||
        input.LPos.y < -1.0f || input.LPos.y > 1.0f ||
        input.LPos.z < 0.0f  || input.LPos.z > 1.0f ) return txColorMap.Sample( samLinear, input.Tex ) * 0.2f;

     //transform clip space coords to texture space coords (-1:1 to 0:1)
    input.LPos.x = input.LPos.x/2 + 0.5;
    input.LPos.y = input.LPos.y/-2 + 0.5;
	input.LPos.z -= DepthBias; //DepthBias

	//----- hardware PCF - single texel
    float shadowFactor = txShadowMap.SampleCmpLevelZero( cmpSampler, input.LPos.xy, input.LPos.z );

	float4 txNorm = txNormalMap.Sample( samLinear, input.Tex ); //Sampled Normal in Tangent Space
	float4 atNorm = 2.0 * txNorm - 1.0;
	float3 tNorm = normalize(atNorm.xyz);

	//Tangent-Space Lightning:
//  float4 I = calcBlinnPhongLighting( material, light.color, tNorm, normalize( input.V1 ), normalize( input.H1 ) );

	float4 Ka = txAOMap.Sample(samLinear, input.Tex );
	float4 Ks = txSpecularMap.Sample(samLinear, input.Tex );
    float4 Ia = Ka * ambientLight;
    float4 Id = material.Kd * saturate( dot(tNorm,normalize( input.V1 )) );
    float4 Is = Ks * pow( saturate(dot(tNorm,normalize( input.H1 ))), material.A );
    float4 I = Ia + (Id + Is) * light.color;

    return shadowFactor * txColorMap.Sample( samLinear, input.Tex ) * I;
}


[maxvertexcount(3)]   // produce a maximum of 3 output vertices
void NM_GS( triangle NORMAL_MAP_PS_INPUT input[3], inout TriangleStream<NORMAL_MAP_PS_INPUT> triStream )
{
  NORMAL_MAP_PS_INPUT psInput = (NORMAL_MAP_PS_INPUT)0;
  
  float3 faceEdgeA = input[1].OPos - input[0].OPos; //Face-normal dla trójk¹ta w Object-Space
  float3 faceEdgeB = input[2].OPos - input[0].OPos;

  //float3 faceEdgeA = input[1].WPos - input[0].WPos; //Face-normal dla trójk¹ta w World-Space
  //float3 faceEdgeB = input[2].WPos - input[0].WPos;

  float3 FN = normalize( cross(faceEdgeA, faceEdgeB) );

  for( uint i = 0; i < 3; i++ )
  {
    psInput.Pos = input[i].Pos;
    psInput.WPos = input[i].WPos;
    psInput.OPos = input[i].OPos;
    psInput.LPos = input[i].LPos;
    psInput.Tex = input[i].Tex;
    psInput.N = input[i].N;
    psInput.T = input[i].T;
    psInput.B = input[i].B;
    psInput.H = input[i].H;
	psInput.V = input[i].V;
    psInput.H1 = input[i].H1;
    psInput.V1 = input[i].V1;
    psInput.FN = FN;
    triStream.Append(psInput);
  }
  triStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
technique10 NormalMap1
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NM_VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, NM_GS() ) );
        SetPixelShader( CompileShader( ps_4_0, NM1_PS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 NormalMap2
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NM_VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, NM_GS() ) );
        SetPixelShader( CompileShader( ps_4_0, NM2_PS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 NormalMap3
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NM_VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, NM_GS() ) );
        SetPixelShader( CompileShader( ps_4_0, NM3_PS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 NormalMap4
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NM_VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, NM_GS() ) );
        SetPixelShader( CompileShader( ps_4_0, NM4_PS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 Texture
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NM_VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, NM_GS() ) );
        SetPixelShader( CompileShader( ps_4_0, TEX_PS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 Shadow
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ShadowMapVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ShadowMapPS() ) );
		SetDepthStencilState( renderLess, 0 );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 SkyBox
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, SkyBoxVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, SkyBoxPS() ) );
		SetDepthStencilState( renderEqual, 0 ); //renderuj gdy = (wyrenderuje niebo wszêdzie tam, gdzie po rysowaniu obiektów zosta³a 1 (od czasu czyszczenia zbufora)
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

