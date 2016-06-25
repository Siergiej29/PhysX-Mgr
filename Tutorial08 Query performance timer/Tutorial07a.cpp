//--------------------------------------------------------------------------------------
// Grafika komputerowa czasu rzeczywistego I/II, Uniwersytet Opolski, 2015-2016
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include "resource.h"

#include "Camera.h" 

using namespace std;
//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    D3DXVECTOR3 Pos;
	D3DXVECTOR3 Color;
	D3DXVECTOR2 Tex;
};

struct SkyBoxVertex //wierzcho³ek skybox'a
{
	D3DXVECTOR3 Pos; // (x,y,z)
	D3DXVECTOR3 Tex; // (u,v,w) - wspó³rzêdne w teksturze kubicznej
};
//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
//win32 api
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
//zwi¹zane z kart¹ graficzn¹ i jej zasobami
D3D10_DRIVER_TYPE                   g_driverType = D3D10_DRIVER_TYPE_NULL;
ID3D10Device*                       g_pd3dDevice = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;

D3D10_VIEWPORT						g_RenderTargetViewport;

ID3D10RenderTargetView*             g_pRenderTargetView = NULL;

ID3D10Texture2D*					g_pZBuffer = NULL; //tekstura (pamiêæ) na z-bufor
ID3D10DepthStencilView*				g_pZBufferDSView = NULL; //widok jako depth stencil (z-bufor)
ID3D10ShaderResourceView*			g_pZBufferSRView = NULL; //widok jako tekstura (zmienna) shadera

ID3D10ShaderResourceView*           g_pTexture0SRV = NULL; //widok na teksturê wczytan¹ z pliku
ID3D10ShaderResourceView*           g_pTexture1SRV = NULL; //widok na teksturê wczytan¹ z pliku
ID3D10ShaderResourceView*           g_pSkyBoxSRVArray[4]; //widoki na tekstury SkyBox'a z pliku

ID3D10Effect*                       g_pEffect = NULL;
ID3D10EffectTechnique*              g_pSimpleTechnique = NULL;
ID3D10EffectTechnique*              g_pSkyBoxTechnique = NULL;

//zmienne shaderów:
ID3D10EffectMatrixVariable*         g_pWorldVariable = NULL; //macierz W
ID3D10EffectMatrixVariable*         g_pViewVariable = NULL; //macierz V
ID3D10EffectMatrixVariable*         g_pProjectionVariable = NULL; //macierz P
ID3D10EffectMatrixVariable*         g_pInvWorldVariable = NULL; //macierz odwrotna do W
ID3D10EffectMatrixVariable*         g_pInvViewVariable = NULL; //macierz odwrotna do V
ID3D10EffectMatrixVariable*         g_pInvProjectionVariable = NULL; //macierz odwrotna do P
ID3D10EffectMatrixVariable*         g_pViewProjectionVariable = NULL; //iloczyn VP
ID3D10EffectMatrixVariable*         g_pWorldViewProjectionVariable = NULL; //iloczyn WVP

ID3D10EffectVectorVariable*         g_pCameraPosVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraUpVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraDirVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraRightVariable = NULL;

ID3D10EffectScalarVariable*			g_pScalarVariable = NULL; //wspó³czynnik mieszania dwóch tekstur nieba
ID3D10EffectScalarVariable*			g_pSkyMixVariable = NULL; //wspó³czynnik mieszania dwóch tekstur nieba

ID3D10EffectShaderResourceVariable* g_pColorMap0Variable = NULL; //tekstura 0
ID3D10EffectShaderResourceVariable* g_pColorMap1Variable = NULL; //tekstura 1
ID3D10EffectShaderResourceVariable* g_pSkyBox1Variable = NULL; //niebo 1
ID3D10EffectShaderResourceVariable* g_pSkyBox2Variable = NULL; //niebo 2


//obiekt 3D:
ID3D10InputLayout*                  g_pSimpleVertexLayout = NULL;
ID3D10InputLayout*                  g_pSkyBoxVertexLayout = NULL; //layout dla skybox'a

ID3D10Buffer*                       g_pPodlogaVertexBuffer = NULL;
ID3D10Buffer*                       g_pPodlogaIndexBuffer = NULL;

ID3D10Buffer*                       g_pCzworoscianVertexBuffer = NULL;
ID3D10Buffer*                       g_pCzworoscianIndexBuffer = NULL;

ID3D10Buffer*                       g_pSkyBoxVertexBuffer = NULL;
ID3D10Buffer*                       g_pSkyBoxIndexBuffer = NULL;

ID3D10Buffer*                       g_pSkyBoxVertexBufferA = NULL;
ID3D10Buffer*                       g_pSkyBoxIndexBufferA = NULL;
//inne
DWORD								g_TimeStart = 0;
D3DXMATRIX							g_Projection;

//Game objects
Camera								g_DebugCamera; //kamera trybu debug (obs³ugiwana z klawiatury)
Camera*								g_pActiveCamera = &g_DebugCamera;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
bool RegisterInputDevices( HWND &hWnd );
HRESULT InitDevice();
void CleanupDevice();
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Every ~16ms PhysX scene is simulated (pushed forward by 1/60s). Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

	RegisterInputDevices(g_hWnd);

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}


	DWORD NextUpdateTick;
	DWORD TicksToSkip = 1000 / 60; 

	g_TimeStart = NextUpdateTick = GetTickCount();


    // Main message loop
    MSG msg = {0};
    while( 1 )
    {
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg ); 
            DispatchMessage( &msg );
			if (WM_QUIT == msg.message) break;
        }
		if (WM_QUIT == msg.message) break;

        Render(); //renderujemy scenê
    }

	CleanupDevice();

    return ( int )msg.wParam;
}

void inline HandleRawInput( HWND &hWnd, HRAWINPUT &lParam )
{
	//get raw input data buffer size
	UINT dbSize;
	GetRawInputData( lParam, RID_INPUT, NULL, &dbSize,sizeof(RAWINPUTHEADER) );
    
	//allocate memory for raw input data and get data
	BYTE* buffer = new BYTE[dbSize];    
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dbSize, sizeof(RAWINPUTHEADER) );
	RAWINPUT* raw = (RAWINPUT*)buffer;
	
	// Handle Keyboard Input
	//---------------------------------------------------------------------------
	if (raw->header.dwType == RIM_TYPEKEYBOARD) 
	{
		switch( raw->data.keyboard.Message )
		{
			//key up
			case WM_KEYUP : 
				switch ( raw->data.keyboard.VKey )
				{
					case 'T' : g_DebugCamera.setMovementToggle( 0, 0 );
					break;

					case 'G' : g_DebugCamera.setMovementToggle( 1, 0 );
					break;

					case 'F' : g_DebugCamera.setMovementToggle( 3, 0 );
					break;

					case 'H' : g_DebugCamera.setMovementToggle( 2, 0 );
					break;
				}
			break;

			//key down
			case WM_KEYDOWN : 
				switch ( raw->data.keyboard.VKey )
				{
					case VK_ESCAPE : PostQuitMessage(0); //sam sobie wyœlij wiadomoœæ
					break;

					case 'T' : g_DebugCamera.setMovementToggle( 0, 1 );
					break;

					case 'G' : g_DebugCamera.setMovementToggle( 1, 1 );
					break;

					case 'F' : g_DebugCamera.setMovementToggle( 3, 1 );
					break;

					case 'H' : g_DebugCamera.setMovementToggle( 2, 1 );
					break;


				}
			break;
		}
	}
	
	// Handle Mouse Input
	//---------------------------------------------------------------------------
	else if (raw->header.dwType == RIM_TYPEMOUSE) 
	{
		//mouse camera control (ustawia kierunek patrzenia kamery)
		g_DebugCamera.adjustHeadingPitch( 0.0025f * raw->data.mouse.lLastX, 0.0025f * raw->data.mouse.lLastY );
	}

	//free allocated memory
	delete[] buffer;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

		case WM_INPUT:
		{
			HandleRawInput( hWnd, (HRAWINPUT&) lParam );
		}
		break;	

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

//register keyboard mouse as input devices
bool RegisterInputDevices( HWND &hWnd )
{
	RAWINPUTDEVICE inputDevices[2];
        
	//adds mouse and allow legacy messages
	inputDevices[0].usUsagePage = 0x01; 
	inputDevices[0].usUsage = 0x02; 
	inputDevices[0].dwFlags = 0;   
	inputDevices[0].hwndTarget = 0;

	//adds keyboard and allow legacy messages
	inputDevices[1].usUsagePage = 0x01; 
	inputDevices[1].usUsage = 0x06; 
	inputDevices[1].dwFlags = 0;   
	inputDevices[1].hwndTarget = 0;

	if ( RegisterRawInputDevices(inputDevices, 2, sizeof(inputDevices[0]) ) == FALSE ) 
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc; //metoda obs³ugi komunikatów
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Computer Graphics - IMII UO 2016", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc); //odczytujemy wielkoœæ okna
	UINT width = rc.right - rc.left; //zapamiêtujemy jego szerokoœæ i wysokoœæ
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

	D3D10_DRIVER_TYPE driverTypes[] =
	{
		D3D10_DRIVER_TYPE_HARDWARE,
		D3D10_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	DXGI_SWAP_CHAIN_DESC sd; //przygotowujemy opis bufora do którego bêdziemy renderowaæ
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width; //wymiary
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //format RGBAlfa po 8 bitów na ka¿d¹ sk³adow¹
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D10CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags,
			D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice); //tworzymy uchwyt do karty graficznej i bufor renderowania
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D10Texture2D* pBuffer;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL, &g_pRenderTargetView); //tworzymy widok na bufor renderowania
	pBuffer->Release();
	if (FAILED(hr))
		return hr;

// Opis tekstury (dla z-buffer)
    D3D10_TEXTURE2D_DESC descTex;
    descTex.Width = width;
    descTex.Height = height;
    descTex.MipLevels = 1;
    descTex.ArraySize = 1;
    descTex.Format = DXGI_FORMAT_R32_TYPELESS;
    descTex.SampleDesc.Count = 1;
    descTex.SampleDesc.Quality = 0;
    descTex.Usage = D3D10_USAGE_DEFAULT;
    descTex.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
    descTex.CPUAccessFlags = 0;
    descTex.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descTex, NULL, &g_pZBuffer );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"Z-buffer txt.", L"Error", MB_OK );
        return hr;
    }

    //Widoki typu depth stencil (test g³êbokoœci - zbuffer) 
    D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pZBuffer, &descDSV, &g_pZBufferDSView );
     if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"Z-buffer dsv.", L"Error", MB_OK );
        return hr;
    }
 
	//Widoki typu ShaderResource (zmienne shadera)
	D3D10_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
	descSRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = descTex.MipLevels;
	descSRV.Texture2D.MostDetailedMip = 0;
    hr = g_pd3dDevice->CreateShaderResourceView( g_pZBuffer, &descSRV, &g_pZBufferSRView );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"Z-buffer srv.", L"Error", MB_OK );
        return hr;
    }
	
	// Setup viewports
	g_RenderTargetViewport.Width = width;
	g_RenderTargetViewport.Height = height;
	g_RenderTargetViewport.MinDepth = 0.0f;
	g_RenderTargetViewport.MaxDepth = 1.0f;
	g_RenderTargetViewport.TopLeftX = 0;
	g_RenderTargetViewport.TopLeftY = 0;
//NEWS
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
    hr = D3DX10CreateEffectFromFile( L"Tutorial07.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL,
                                         NULL, &g_pEffect, NULL, NULL );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be located.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Obtain the technique
    g_pSimpleTechnique = g_pEffect->GetTechniqueByName( "Simple" );	
	g_pSkyBoxTechnique = g_pEffect->GetTechniqueByName( "SkyBox" );

   // Obtain the variables
    g_pWorldVariable = g_pEffect->GetVariableByName( "World" )->AsMatrix();
    g_pViewVariable = g_pEffect->GetVariableByName( "View" )->AsMatrix();
    g_pProjectionVariable = g_pEffect->GetVariableByName( "Projection" )->AsMatrix();
	g_pInvWorldVariable = g_pEffect->GetVariableByName( "InvWorld" )->AsMatrix();
	g_pInvViewVariable = g_pEffect->GetVariableByName( "InvView" )->AsMatrix();
	g_pInvProjectionVariable = g_pEffect->GetVariableByName( "InvProjection" )->AsMatrix();
	g_pViewProjectionVariable = g_pEffect->GetVariableByName( "ViewProjection" )->AsMatrix();
	g_pWorldViewProjectionVariable = g_pEffect->GetVariableByName( "WorldViewProjection" )->AsMatrix();

    g_pCameraPosVariable = g_pEffect->GetVariableByName( "CameraPos" )->AsVector();
    g_pCameraUpVariable = g_pEffect->GetVariableByName( "CameraUp" )->AsVector();
    g_pCameraDirVariable = g_pEffect->GetVariableByName( "CameraDir" )->AsVector();
    g_pCameraRightVariable = g_pEffect->GetVariableByName( "CameraRight" )->AsVector();

	g_pScalarVariable = g_pEffect->GetVariableByName("Scalar")->AsScalar();

	g_pColorMap0Variable = g_pEffect->GetVariableByName("txColorMap0")->AsShaderResource();
	g_pColorMap1Variable = g_pEffect->GetVariableByName("txColorMap1")->AsShaderResource();

	g_pSkyBox1Variable = g_pEffect->GetVariableByName("txSkyBox1")->AsShaderResource();
	g_pSkyBox2Variable = g_pEffect->GetVariableByName("txSkyBox2")->AsShaderResource();
	g_pSkyMixVariable = g_pEffect->GetVariableByName("SkyMix")->AsScalar();

//wczytywanie tekstur
// Load the Texture
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Fire.dds", NULL, NULL, &g_pTexture0SRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"smoke.dds", NULL, NULL, &g_pTexture1SRV, NULL);
	if (FAILED(hr))
		return hr;
	// Load SkyBox Texture
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"sky_3_cube.dds", NULL, NULL, &g_pSkyBoxSRVArray[0], NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"sky_4_cube.dds", NULL, NULL, &g_pSkyBoxSRVArray[1], NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"sky_17_cube.dds", NULL, NULL, &g_pSkyBoxSRVArray[2], NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Land1.dds", NULL, NULL, &g_pSkyBoxSRVArray[3], NULL);
	if (FAILED(hr))
		return hr;
//Czworoscian
    // Create vertex buffer
    SimpleVertex czworoscianVertices[] =
    {
        { D3DXVECTOR3( -1.0f, 1.0f, 0.5f ), D3DXVECTOR3( 1.0f, 0.0f, 0.0f ), D3DXVECTOR2(0.0f, 0.0f) },
        { D3DXVECTOR3(  1.0f, 1.0f, 0.5f ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f ), D3DXVECTOR2(1.0f, 0.0f) },
        { D3DXVECTOR3( 0.0f, 0.0f, 0.5f ), D3DXVECTOR3( 0.0f, 0.0f, 1.0f ), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3(0.9f, 0.5f, 0.6f), D3DXVECTOR3( 1.0f, 1.0f, 0.0f ), D3DXVECTOR2(1.0f, 1.0f) },
		{ D3DXVECTOR3(-0.9f, 0.5f, 0.4f), D3DXVECTOR3( 0.0f, 1.0f, 1.0f ), D3DXVECTOR2(1.0f, 0.0f) },
    };

    D3D10_BUFFER_DESC bd;
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(czworoscianVertices);
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = czworoscianVertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pCzworoscianVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Create index buffer
    DWORD czworoscianIndices[] =
    {
        0, 1, 2,
		2, 4, 3,
  	};
	
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(czworoscianIndices);        // 3*n vertices needed for n triangles in a triangle list
    bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    InitData.pSysMem = czworoscianIndices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pCzworoscianIndexBuffer );
    if( FAILED( hr ) )
        return hr;	

	//Podloga
	// Create vertex buffer
	SimpleVertex podlogaVertices[] =
	{
		{ D3DXVECTOR3(-10.0f, 0.0f, -10.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR3(-10.0f, 0.0f, 10.0f), D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR3(10.0f, 0.0f, 10.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR2(1.0f, 0.0f) },
		{ D3DXVECTOR3(10.0f, 0.0f, -10.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2(1.0f, 1.0f) },
	};

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(podlogaVertices);
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = podlogaVertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pPodlogaVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Create index buffer
	DWORD podlogaIndices[] =
	{
		0, 1, 2,
		2, 3, 0,
	};

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(podlogaIndices);        // 3*n vertices needed for n triangles in a triangle list
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = podlogaIndices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pPodlogaIndexBuffer);
	if (FAILED(hr))
		return hr;


	// Define the input layout
	D3D10_INPUT_ELEMENT_DESC Simplelayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	int numElements = sizeof(Simplelayout) / sizeof(Simplelayout[0]);

	// Create the input layout
	D3D10_PASS_DESC PassDesc;
	g_pSimpleTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	hr = g_pd3dDevice->CreateInputLayout(Simplelayout, numElements, PassDesc.pIAInputSignature,
		PassDesc.IAInputSignatureSize, &g_pSimpleVertexLayout);
	if (FAILED(hr))
		return hr;

	//SkyBox
	// Define the input layout
	D3D10_INPUT_ELEMENT_DESC SkyBoxlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	numElements = sizeof(SkyBoxlayout) / sizeof(SkyBoxlayout[0]);

	// Create the input layout
	g_pSkyBoxTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	hr = g_pd3dDevice->CreateInputLayout(SkyBoxlayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pSkyBoxVertexLayout);
	if (FAILED(hr))
		return hr;

	// Create vertex buffer
	SkyBoxVertex vertices[] =
	{
		{ D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(-1.0f, -1.010f, -1.0f) },
		{ D3DXVECTOR3(-1.0f, -1.0f, 1.0f), D3DXVECTOR3(-1.0f, -1.010f, -1.0f) },
		{ D3DXVECTOR3(1.0f, -1.0f, 1.0f), D3DXVECTOR3(1.0f, -1.010f, 1.0f) },
		{ D3DXVECTOR3(1.0f, -1.0f, -1.0f), D3DXVECTOR3(1.0f, -1.010f, -1.0f) },

		{ D3DXVECTOR3(-1.0f, 1.5f, -1.0f), D3DXVECTOR3(-1.0f, 1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, 1.5f, -1.0f), D3DXVECTOR3(1.0f, 1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, 1.5f, 1.0f), D3DXVECTOR3(1.0f, 1.0f, 1.0f) },
		{ D3DXVECTOR3(-1.0f, 1.5f, 1.0f), D3DXVECTOR3(-1.0f, 1.0f, 1.0f) },

		{ D3DXVECTOR3(1.0f, 0.0f, -1.0f), D3DXVECTOR3(1.0f, -1.0f, -1.0f) },
		{ D3DXVECTOR3(-1.0f, 0.0f, -1.0f), D3DXVECTOR3(-1.0f, -1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, 0.0f, 1.0f), D3DXVECTOR3(1.0f, -1.0f, 1.0f) },
		{ D3DXVECTOR3(-1.0f, 0.0f, 1.0f), D3DXVECTOR3(-1.0f, -1.0f, 1.0f) },
	};

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SkyBoxVertex) * 12;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSkyBoxVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Create index buffer
	DWORD indices[] =
	{
		0,1,2,
		2,3,0,

		4,5,6,
		6,7,4,

		0,3,8,
		8,9,0,

		9,8,5,
		5,4,9,

		3,2,10,
		10,8,3,

		8,10,6,
		6,5,8,

		2,1,11,
		11,10,2,

		10,11,7,
		7,6,10,

		1,0,9,
		9,11,1,

		11,9,4,
		4,7,11,
	};
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DWORD) * 60;        // 60 vertices needed for 20 triangles in a triangle list
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSkyBoxIndexBuffer);
	if (FAILED(hr))
		return hr;

	//SkyBoxA

	// Create vertex buffer
	SkyBoxVertex verticesA[] =
	{
		{ D3DXVECTOR3(-1.0f, 1.0f, -1.0f), D3DXVECTOR3(-1.0f, 1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, 1.0f, -1.0f), D3DXVECTOR3(1.0f, 1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, 1.0f, 1.0f), D3DXVECTOR3(1.0f, 1.0f, 1.0f) },
		{ D3DXVECTOR3(-1.0f, 1.0f, 1.0f), D3DXVECTOR3(-1.0f, 1.0f, 1.0f) },

		{ D3DXVECTOR3(1.0f, -1.0f, -1.0f), D3DXVECTOR3(1.0f, -1.0f, -1.0f) },
		{ D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(-1.0f, -1.0f, -1.0f) },
		{ D3DXVECTOR3(1.0f, -1.0f, 1.0f), D3DXVECTOR3(1.0f, -1.0f, 1.0f) },
		{ D3DXVECTOR3(-1.0f, -1.0f, 1.0f), D3DXVECTOR3(-1.0f, -1.0f, 1.0f) },
	};

	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SkyBoxVertex) * 8;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = verticesA;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSkyBoxVertexBufferA);
	if (FAILED(hr))
		return hr;

	// Create index buffer
	DWORD indicesA[] =
	{
		0,1,2,
		2,3,0,

		5,4,1,
		1,0,5,

		4,6,2,
		2,1,4,

		6,7,3,
		3,2,6,

		7,5,0,
		0,3,7,
	};
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DWORD) * 30;        // 30 vertices needed for 10 triangles in a triangle list
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = indicesA;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSkyBoxIndexBufferA);
	if (FAILED(hr))
		return hr;

//GAME OBJECTS
	g_DebugCamera.setPerspectiveProjectionLH(45.0, width / ( FLOAT )height, 0.1f, 250.0f);
	
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pd3dDevice) g_pd3dDevice->ClearState();

	if (g_pTexture0SRV) g_pTexture0SRV->Release();
	if (g_pTexture1SRV) g_pTexture1SRV->Release();
	if (g_pSkyBoxSRVArray[0]) g_pSkyBoxSRVArray[0]->Release();
	if (g_pSkyBoxSRVArray[1]) g_pSkyBoxSRVArray[1]->Release();
	if (g_pSkyBoxSRVArray[2]) g_pSkyBoxSRVArray[2]->Release();
	if (g_pSkyBoxSRVArray[3]) g_pSkyBoxSRVArray[3]->Release();
    if( g_pZBufferDSView ) g_pZBufferDSView->Release();
    if( g_pZBufferSRView ) g_pZBufferSRView->Release();
    if( g_pZBuffer ) g_pZBuffer->Release();
	
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

//--------------------------------------------------------------------------------------
// Render a scene
//--------------------------------------------------------------------------------------
void Render()
{
	D3DXMATRIX World, World2, View, Projection, InvView, InvProjection;
	
	g_DebugCamera.update(0.02); //kamerê debug posuwamy w kierunku ustalonym z klawiatury	
	//zapisujemy macierze do karty graficznej
	View = g_pActiveCamera->getViewMatrix();
	g_pViewVariable->SetMatrix((float*) View);
	D3DXMatrixInverse( &InvView, NULL, &View );
	g_pInvViewVariable->SetMatrix( (float*) InvView);
	g_pCameraPosVariable->SetFloatVector((float*) g_pActiveCamera->getCameraPosition());
	g_pCameraUpVariable->SetFloatVector((float*) g_pActiveCamera->getCameraUp());
	g_pCameraRightVariable->SetFloatVector((float*) g_pActiveCamera->getCameraRight());
	g_pCameraDirVariable->SetFloatVector((float*) g_pActiveCamera->getCameraDirection());
	if (g_pActiveCamera->isProjectionDirty())
	{
		g_Projection = g_pActiveCamera->getProjectionMatrix();
		g_pProjectionVariable->SetMatrix( ( float* ) g_Projection );
		D3DXMatrixInverse( &InvProjection, NULL, &g_Projection);
		g_pInvProjectionVariable->SetMatrix( (float*) InvProjection);
	}
	g_pViewProjectionVariable->SetMatrix((float*) (View*g_Projection));
	
	g_pScalarVariable->SetFloat(fabsf(sinf(GetTickCount() / 800.0f)));
	
	g_pd3dDevice->RSSetViewports(1, &g_RenderTargetViewport);
	g_pd3dDevice->OMSetRenderTargets(1, &g_pRenderTargetView, g_pZBufferDSView);
	float ClearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	g_pd3dDevice->ClearRenderTargetView( g_pRenderTargetView, ClearColor );
	g_pd3dDevice->ClearDepthStencilView( g_pZBufferDSView, D3D10_CLEAR_DEPTH, 1.0f, 0 ); // Clear the depth buffer to 1.0 (max depth)

	UINT stride, offset = 0;	
	g_pd3dDevice->IASetInputLayout( g_pSimpleVertexLayout );
    stride = sizeof( SimpleVertex );

//CZWOROSCIAN
    g_pd3dDevice->IASetVertexBuffers( 0, 1, &g_pCzworoscianVertexBuffer, &stride, &offset );
    g_pd3dDevice->IASetIndexBuffer( g_pCzworoscianIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
    g_pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	D3DXMatrixRotationZ(&World2, GetTickCount() / 100.0f);
	D3DXMatrixTranslation(&World, 0.0f, 10.0f * sinf(GetTickCount() / 500.0f), 0.0f);
	D3DXMatrixMultiply(&World, &World2, &World);
//D3DXMatrixIdentity(&World);
    g_pWorldVariable->SetMatrix( ( float* )&World );
	g_pColorMap0Variable->SetResource(g_pTexture0SRV);
	g_pColorMap1Variable->SetResource(g_pTexture1SRV); 
	g_pSimpleTechnique->GetPassByIndex(0)->Apply(0);
    g_pd3dDevice->DrawIndexed( 6, 0, 0 ); //3 * liczba trójk¹tow

//PODLOGA
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pPodlogaVertexBuffer, &stride, &offset);
	g_pd3dDevice->IASetIndexBuffer(g_pPodlogaIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
//	D3DXMatrixRotationY(&World, GetTickCount() / 500.0f);
D3DXMatrixIdentity(&World);
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pColorMap0Variable->SetResource(g_pTexture0SRV);
	g_pColorMap1Variable->SetResource(g_pTexture1SRV);
	g_pSimpleTechnique->GetPassByIndex(0)->Apply(0);
	g_pd3dDevice->DrawIndexed(6, 0, 0); //3 * liczba trójk¹tow

//SKYBOX
 	g_pSkyMixVariable->SetFloat(fabsf(sinf(GetTickCount() / 2000.0f))); //ustalamy wsp. mieszania dwóch tekstur zgodnie z aktualnym czasem
	g_pSkyBox1Variable->SetResource(g_pSkyBoxSRVArray[0]);
	g_pSkyBox2Variable->SetResource(g_pSkyBoxSRVArray[1]);
	D3DXVECTOR3 f = g_pActiveCamera->getCameraPosition(); //œrodek skybox'a zawsze w lokalizacji kamery (poruszaj¹c siê nie zbli¿amy siê do nieba)
	D3DXMatrixTranslation(&World, f.x, f.y, f.z);
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pd3dDevice->IASetInputLayout(g_pSkyBoxVertexLayout);
	stride = sizeof(SkyBoxVertex);
	offset = 0;
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pSkyBoxVertexBuffer, &stride, &offset);
	g_pd3dDevice->IASetIndexBuffer(g_pSkyBoxIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pSkyBoxTechnique->GetPassByIndex(0)->Apply(0);
	//g_pd3dDevice->DrawIndexed(60, 0, 0);

	//SKYBOX A
	g_pSkyMixVariable->SetFloat(fabsf(sinf(GetTickCount() / 2000.0f))); //ustalamy wsp. mieszania dwóch tekstur zgodnie z aktualnym czasem
	g_pSkyBox1Variable->SetResource(g_pSkyBoxSRVArray[0]);
	g_pSkyBox2Variable->SetResource(g_pSkyBoxSRVArray[0]);
	f = g_pActiveCamera->getCameraPosition(); //œrodek skybox'a zawsze w lokalizacji kamery (poruszaj¹c siê nie zbli¿amy siê do nieba)
	D3DXMatrixTranslation(&World, f.x, f.y, f.z);
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pd3dDevice->IASetInputLayout(g_pSkyBoxVertexLayout);
	stride = sizeof(SkyBoxVertex);
	offset = 0;
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pSkyBoxVertexBufferA, &stride, &offset);
	g_pd3dDevice->IASetIndexBuffer(g_pSkyBoxIndexBufferA, DXGI_FORMAT_R32_UINT, 0);
	g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pSkyBoxTechnique->GetPassByIndex(0)->Apply(0);
	g_pd3dDevice->DrawIndexed(30, 0, 0);
	g_pSwapChain->Present(0, 0);     // Present our back buffer to our front buffer
}
