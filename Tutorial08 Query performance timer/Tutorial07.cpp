//--------------------------------------------------------------------------------------
// Grafika komputerowa czasu rzeczywistego I/II, Uniwersytet Opolski, 2015-2016
//--------------------------------------------------------------------------------------
#include <PxPhysicsAPI.h> //Single header file to include all features of PhysX API 

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include "resource.h"
#include <time.h>
#include <string>

#include "Camera.h" 
#include "Object.h"
#include "Terrain.h"
#include "SunController.h"
#include "SkyController.h"
#include "MoveController.h"
#include "TimeCounter.h"
#include "Logger.h"
#include "HeightField.h"

//-------Loading PhysX libraries (_x64 lub _x86)----------//
#ifdef _DEBUG //If in 'Debug' load libraries for debug mode 
#pragma comment(lib, "PhysX3DEBUG_x64.lib")				//Always be needed  
#pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")		//Always be needed
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")		//PhysX extended library 
#pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib") //For PVD only 

#else //Else load libraries for 'Release' mode
#pragma comment(lib, "PhysX3_x64.lib")	
#pragma comment(lib, "PhysX3Common_x64.lib") 
#pragma comment(lib, "PhysX3Extensions.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDK.lib")
#endif

using namespace std;
using namespace physx;

Logger *logger = new Logger("log.txt");


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SkyBoxVertex //wierzcho³ek skybox'a
{
	D3DXVECTOR3 Pos; // (x,y,z)
	D3DXVECTOR3 Tex; // (u,v,w) - wspó³rzêdne w teksturze kubicznej
};

struct DirectionalLight //œwiat³o kierunkowe (s³oñce, ksiê¿yc)
{
	D3DXVECTOR4 color;
	D3DXVECTOR3 direction;
};

struct Material //sk³adowe i wspó³czynniki w omawianym modelu oœwietlenia
{
	float ambient, diffuse, specular, shininess;
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
ID3D10RenderTargetView*             g_pRenderTargetView = NULL;

D3D10_VIEWPORT						g_RenderTargetViewport;
D3D10_VIEWPORT						g_ShadowMapViewport;

ID3D10ShaderResourceView*           g_pTexture0SRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTexture1SRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTexture2SRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTexture3SRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTexture4SRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTextureAOSRV = NULL; //widok na teksturê z pliku
ID3D10ShaderResourceView*           g_pTextureSpecularSRV = NULL; //widok na teksturê z pliku

ID3D10ShaderResourceView*           g_pSkyBoxSRVArray[4]; //widoki na tekstury SkyBox'a z pliku

ID3D10Texture2D*					g_pZBuffer = NULL; //tekstura (pamiêæ) na z-bufor
ID3D10DepthStencilView*				g_pZBufferDSView = NULL; //widok jako depth stencil (z-bufor)
ID3D10ShaderResourceView*			g_pZBufferSRView = NULL; //widok jako tekstura (zmienna) shadera

ID3D10Texture2D*					g_pShadowMap = NULL; //jak wy¿ej, dla shadow-mapy
ID3D10DepthStencilView*				g_pShadowMapDSView = NULL;
ID3D10ShaderResourceView*			g_pShadowMapSRView = NULL;

ID3D10Effect*                       g_pEffect = NULL;
ID3D10EffectTechnique*              g_pShadowTechnique = NULL;
ID3D10EffectTechnique*              g_pSkyBoxTechnique = NULL;
ID3D10EffectTechnique*              g_pNormalMap1Technique = NULL;
ID3D10EffectTechnique*              g_pNormalMap2Technique = NULL;
ID3D10EffectTechnique*              g_pNormalMap3Technique = NULL;
ID3D10EffectTechnique*              g_pNormalMap4Technique = NULL;
ID3D10EffectTechnique*              g_pTextureTechnique = NULL;

//zmienne shaderów:
ID3D10EffectMatrixVariable*         g_pWorldVariable = NULL; //macierz W
ID3D10EffectMatrixVariable*         g_pViewVariable = NULL; //macierz V
ID3D10EffectMatrixVariable*         g_pProjectionVariable = NULL; //macierz P
ID3D10EffectMatrixVariable*         g_pInvWorldVariable = NULL; //macierz odwrotna do W
ID3D10EffectMatrixVariable*         g_pInvViewVariable = NULL; //macierz odwrotna do V
ID3D10EffectMatrixVariable*         g_pInvProjectionVariable = NULL; //macierz odwrotna do P
ID3D10EffectMatrixVariable*         g_pViewProjectionVariable = NULL; //iloczyn VP
ID3D10EffectMatrixVariable*         g_pWorldViewProjectionVariable = NULL; //iloczyn WVP

ID3D10EffectMatrixVariable*         g_pLightViewProjectionVariable = NULL; //macierz przekszta³cenia z W do kamery ustawionej w Ÿródle œwiat³a
ID3D10EffectVectorVariable*         g_pCameraPosVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraUpVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraDirVariable = NULL;
ID3D10EffectVectorVariable*         g_pCameraRightVariable = NULL;

ID3D10EffectVariable*				g_pLightVariable = NULL; //parametry œwiat³a

ID3D10EffectShaderResourceVariable* g_pColorMapVariable = NULL; //tekstura z kolorem
ID3D10EffectShaderResourceVariable* g_pNormalMapVariable = NULL; //tekstura z map¹ wektorów normalnych
ID3D10EffectShaderResourceVariable* g_pShadowMapVariable = NULL; //tekstura z map¹ cieni
ID3D10EffectShaderResourceVariable* g_pDepthMapVariable = NULL; //tekstura z map¹ g³êbokoœci
ID3D10EffectShaderResourceVariable* g_pSpecularMapVariable = NULL; //tekstura z map¹ specular
ID3D10EffectShaderResourceVariable* g_pAOMapVariable = NULL; //tekstura z map¹ AO
ID3D10EffectShaderResourceVariable* g_pSkyBox1Variable = NULL; //niebo 1
ID3D10EffectShaderResourceVariable* g_pSkyBox2Variable = NULL; //niebo 2

ID3D10EffectScalarVariable*			g_pSkyMixVariable = NULL; //wspó³czynnik mieszania dwóch tekstur nieba

ID3D10InputLayout*                  g_pVertexLayout = NULL; //nasz standardowy layout dla obiektów
ID3D10InputLayout*                  g_pShadowVertexLayout = NULL; //layout dla techniki generowania mapy cieni
ID3D10InputLayout*                  g_pSkyBoxVertexLayout = NULL; //layout dla skybox'a

ID3D10Buffer*                       g_pSkyBoxVertexBuffer = NULL;
ID3D10Buffer*                       g_pSkyBoxIndexBuffer = NULL;

ID3D10Buffer*                       g_pHeightFieldVertexBuffer = NULL;
ID3D10Buffer*                       g_pHeightFieldIndexBuffer = NULL;

D3DXVECTOR4							g_AmbientLight;
DirectionalLight					g_DirectionalLight;
Material							g_Material;

//Game objects
Camera								g_TPPCamera; //kamera TPP
Camera								g_DebugCamera; //kamera trybu debug (obs³ugiwana z klawiatury)
int									g_ActiveCamera = 0; //0 - TPP / 1 - free debug cam
Camera*								g_pActiveCamera = &g_TPPCamera;

D3DXMATRIX							g_Projection;

TextureManager						g_TM; //obiekt zarz¹dza teksturami, obiekty posiadaj¹ tylko indeksy do tekstur w TM
Object								*g_pBall; //obiekty
Object								*g_pPlayer1;
Object								*g_pPlayer2;
Object								*g_pPlayer3;

MoveController						g_Player1Controller;
MoveController						g_Player2Controller;
MoveController						g_Player3Controller;

Terrain								g_Terrain;
SunController						g_Sun;
SkyController						g_Sky(4, 10000);

DWORD								g_TimeStart = 0;

// PhysX Global variables
PxPhysics*				g_PhysicsSDK = NULL;			//Instance of PhysX SDK
static PxFoundation*			g_Foundation = NULL;			//Instance of singleton foundation SDK class
static PxDefaultErrorCallback	g_DefaultErrorCallback;		//Instance of default implementation of the error callback
static PxDefaultAllocator		g_DefaultAllocatorCallback;	//Instance of default implementation of the allocator interface required by the SDK

PxScene							*g_PxScene = NULL;				//Instance of PhysX Scene				
PxRigidDynamic					*g_PxBall = NULL;
PxRigidDynamic					*g_PxPlayer1 = NULL;
PxRigidDynamic					*g_PxPlayer2 = NULL;
PxRigidDynamic					*g_PxPlayer3 = NULL;

PxRigidDynamic					*g_PxBramka = NULL;	// obszar trigger'a

PxRigidDynamic					*g_PxCamera = NULL; //kula reprezentuj¹ca kamerê			 
PxRigidDynamic					*g_PxKinematic = NULL;	//kula odtwarzaj¹ca ruchy gracza (w pewnej wysokoœci), ci¹gn¹ca na "sprê¿ynie" kulê kamery		 
PxRigidDynamic					*g_PxHeightfield = NULL; //terrain

														 //--------------------------------------------------------------------------------------
														 // Forward declarations
														 //--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
bool RegisterInputDevices(HWND &hWnd);
HRESULT InitDevice();
void CleanupDevice();
void Render();
void InitPhysX();
void ShutdownPhysX();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Every ~16ms PhysX scene is simulated (pushed forward by 1/60s). Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	srand(time(NULL));

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	RegisterInputDevices(g_hWnd);

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	InitPhysX();  //Initialize PhysX then create scene and actors

	DWORD NextUpdateTick;
	DWORD TicksToSkip = 1000 / 60;

	g_TimeStart = NextUpdateTick = GetTickCount();
	g_Sky.Time0 = 0;
	g_Sky.Time1 = g_Sky.DT;

	// Main message loop
	MSG msg = { 0 };
	while (1)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WM_QUIT == msg.message) break;
		}
		if (WM_QUIT == msg.message) break;

		int loops = 0;
		while ((GetTickCount() > NextUpdateTick) && (loops < 10)) //jeœli nadszed³ czas na symulacjê fizyki i nie by³o wiêcej ni¿ 10 symulacji bez renderowania, to:
		{
			D3DXVECTOR3 DxRight;
			//if (g_Player1Controller.getRightDirection(DxRight)) //weŸ oœ obrotu Playera1 (zmodyfikowan¹ zgodnie z aktualnym stanem klawiatury)
			//{
			//	PxVec3 PxRight;
			//	PxRight.x = DxRight.x;
			//	PxRight.y = DxRight.y;
			//	PxRight.z = DxRight.z;
			//	g_PxPlayer1->addTorque(2 * PxRight, PxForceMode::eIMPULSE); //przy³ó¿ moment obrotowy (impuls) zgodnie z osi¹ obrotu
			//}
			//-------
			TimeCounter *time = new TimeCounter();
			time->reset();
			//-------
			g_PxScene->simulate(1.0f / 60.0f);	//Symulacja œwiata fizyki o 1/60 sek
			g_PxScene->fetchResults(true); //Odczyt wyników symulacji
			//-------
			time->stop();

			logger->log("Czas symulacji: " + std::to_string((time->getStart())));
			
			//PxVec3 Lin = g_PxPlayer1->getLinearVelocity(); //pobieramy aktualn¹ (po symulacji) oœ obrotu Playera1 ...
			//g_Player1Controller.Update(Lin.x, Lin.y, Lin.z); //... i updatujemy jego kontroler

			NextUpdateTick += TicksToSkip; //ustawiamy moment kolejnej symulacji (jeœli od razu jesteœmy spóŸnieni, to kolejna pêtla symulacji (max 10 pêtli bez renderowania))
			loops++;
		}
		Render(); //renderujemy scenê
	}

	CleanupDevice();
	ShutdownPhysX();

	return (int)msg.wParam;
}

void inline HandleRawInput(HWND &hWnd, HRAWINPUT &lParam)
{
	//get raw input data buffer size
	UINT dbSize;
	GetRawInputData(lParam, RID_INPUT, NULL, &dbSize, sizeof(RAWINPUTHEADER));

	//allocate memory for raw input data and get data
	BYTE* buffer = new BYTE[dbSize];
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dbSize, sizeof(RAWINPUTHEADER));
	RAWINPUT* raw = (RAWINPUT*)buffer;

	// Handle Keyboard Input
	//---------------------------------------------------------------------------
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		switch (raw->data.keyboard.Message)
		{
			//key up
		case WM_KEYUP:
			switch (raw->data.keyboard.VKey)
			{
			case 'T': g_DebugCamera.setMovementToggle(0, 0);
				break;

			case 'G': g_DebugCamera.setMovementToggle(1, 0);
				break;

			case 'F': g_DebugCamera.setMovementToggle(3, 0);
				break;

			case 'H': g_DebugCamera.setMovementToggle(2, 0);
				break;

			case 'W': g_Player1Controller.setMovementToggle(0, 0);
				break;

			case 'S': g_Player1Controller.setMovementToggle(1, 0);
				break;

			case 'A': g_Player1Controller.setMovementToggle(3, 0);
				break;

			case 'D': g_Player1Controller.setMovementToggle(2, 0);
				break;

			case 'R': if (g_ActiveCamera == 0) //zmiana aktywnej kamery
			{
				g_ActiveCamera = 1;
				g_pActiveCamera = &g_DebugCamera;
				D3DXVECTOR3 u = g_TPPCamera.getCameraPosition();
				g_DebugCamera.update(u.x, u.y, u.z); //kamera debug zaczyna w aktualnej pozycji TPP
			}
					  else
					  {
						  g_ActiveCamera = 0;
						  g_pActiveCamera = &g_TPPCamera;
					  }
					  break;
			}
			break;

			//key down
		case WM_KEYDOWN:
			switch (raw->data.keyboard.VKey)
			{
			case VK_ESCAPE: PostQuitMessage(0); //sam sobie wyœlij wiadomoœæ
				break;

			case 'T': g_DebugCamera.setMovementToggle(0, 1);
				break;

			case 'G': g_DebugCamera.setMovementToggle(1, 1);
				break;

			case 'F': g_DebugCamera.setMovementToggle(3, 1);
				break;

			case 'H': g_DebugCamera.setMovementToggle(2, 1);
				break;

			case 'W': g_Player1Controller.setMovementToggle(0, 1);
				break;

			case 'S': g_Player1Controller.setMovementToggle(1, 1);
				break;

			case 'A': g_Player1Controller.setMovementToggle(3, 1);
				break;

			case 'D': g_Player1Controller.setMovementToggle(2, 1);
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
		g_TPPCamera.adjustHeadingPitch(0.0025f * raw->data.mouse.lLastX, 0.0025f * raw->data.mouse.lLastY);
		g_DebugCamera.adjustHeadingPitch(0.0025f * raw->data.mouse.lLastX, 0.0025f * raw->data.mouse.lLastY);
	}

	//free allocated memory
	delete[] buffer;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_INPUT:
	{
		HandleRawInput(hWnd, (HRAWINPUT&)lParam);
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//register keyboard mouse as input devices
bool RegisterInputDevices(HWND &hWnd)
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

	if (RegisterRawInputDevices(inputDevices, 2, sizeof(inputDevices[0])) == FALSE)
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc; //metoda obs³ugi komunikatów
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Rafa³ Pawo³ka - Praca Mgr", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

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
#ifdef _DEBUG
	//    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

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

	// Opis tekstury (dla z-buffer i shadowmapy)
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
	hr = g_pd3dDevice->CreateTexture2D(&descTex, NULL, &g_pZBuffer);
	if (FAILED(hr))
		return hr;
	descTex.Width = 1024; //rozdzielczoœæ mapy cieni
	descTex.Height = 1024;
	hr = g_pd3dDevice->CreateTexture2D(&descTex, NULL, &g_pShadowMap);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"ShadowMap txt.", L"Error", MB_OK);
		return hr;
	}

	//Widoki typu depth stencil (do u¿ytku przez kartê jako zbuffer) 
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pZBuffer, &descDSV, &g_pZBufferDSView);
	if (FAILED(hr))
		return hr;
	descTex.Width = 1024;
	descTex.Height = 1024;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pShadowMap, &descDSV, &g_pShadowMapDSView);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"ShadowMap dsv.", L"Error", MB_OK);
		return hr;
	}

	//Widoki typu ShaderResource (do u¿ytku przez kartê jako zmienne shadera)
	D3D10_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.Format = DXGI_FORMAT_R32_FLOAT;
	descSRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = descTex.MipLevels;
	descSRV.Texture2D.MostDetailedMip = 0;
	hr = g_pd3dDevice->CreateShaderResourceView(g_pZBuffer, &descSRV, &g_pZBufferSRView);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateShaderResourceView(g_pShadowMap, &descSRV, &g_pShadowMapSRView);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"ShadowMap srv.", L"Error", MB_OK);
		return hr;
	}

	// Setup viewports
	g_RenderTargetViewport.Width = width;
	g_RenderTargetViewport.Height = height;
	g_RenderTargetViewport.MinDepth = 0.0f;
	g_RenderTargetViewport.MaxDepth = 1.0f;
	g_RenderTargetViewport.TopLeftX = 0;
	g_RenderTargetViewport.TopLeftY = 0;

	g_ShadowMapViewport.Width = 1024;
	g_ShadowMapViewport.Height = 1024;
	g_ShadowMapViewport.MinDepth = 0.0f;
	g_ShadowMapViewport.MaxDepth = 1.0f;
	g_ShadowMapViewport.TopLeftX = 0;
	g_ShadowMapViewport.TopLeftY = 0;

	// Create the effect
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
	hr = D3DX10CreateEffectFromFile(L"Tutorial07.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL,
		NULL, &g_pEffect, NULL, NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"Problem z plikem FX", L"Error", MB_OK);
		return hr;
	}

	// Obtain the technique
	g_pShadowTechnique = g_pEffect->GetTechniqueByName("Shadow");
	g_pSkyBoxTechnique = g_pEffect->GetTechniqueByName("SkyBox");
	g_pNormalMap1Technique = g_pEffect->GetTechniqueByName("NormalMap1");
	g_pNormalMap2Technique = g_pEffect->GetTechniqueByName("NormalMap2");
	g_pNormalMap3Technique = g_pEffect->GetTechniqueByName("NormalMap3");
	g_pNormalMap4Technique = g_pEffect->GetTechniqueByName("NormalMap4");
	g_pTextureTechnique = g_pEffect->GetTechniqueByName("Texture");

	// Obtain the variables
	g_pWorldVariable = g_pEffect->GetVariableByName("World")->AsMatrix();
	g_pViewVariable = g_pEffect->GetVariableByName("View")->AsMatrix();
	g_pProjectionVariable = g_pEffect->GetVariableByName("Projection")->AsMatrix();
	g_pInvWorldVariable = g_pEffect->GetVariableByName("InvWorld")->AsMatrix();
	g_pInvViewVariable = g_pEffect->GetVariableByName("InvView")->AsMatrix();
	g_pInvProjectionVariable = g_pEffect->GetVariableByName("InvProjection")->AsMatrix();
	g_pViewProjectionVariable = g_pEffect->GetVariableByName("ViewProjection")->AsMatrix();
	g_pWorldViewProjectionVariable = g_pEffect->GetVariableByName("WorldViewProjection")->AsMatrix();
	g_pLightViewProjectionVariable = g_pEffect->GetVariableByName("LightViewProjection")->AsMatrix();

	g_pColorMapVariable = g_pEffect->GetVariableByName("txColorMap")->AsShaderResource();
	g_pShadowMapVariable = g_pEffect->GetVariableByName("txShadowMap")->AsShaderResource();
	g_pNormalMapVariable = g_pEffect->GetVariableByName("txNormalMap")->AsShaderResource();
	g_pDepthMapVariable = g_pEffect->GetVariableByName("txDepthMap")->AsShaderResource();
	g_pSpecularMapVariable = g_pEffect->GetVariableByName("txSpecularMap")->AsShaderResource();
	g_pAOMapVariable = g_pEffect->GetVariableByName("txAOMap")->AsShaderResource();
	g_pSkyBox1Variable = g_pEffect->GetVariableByName("txSkyBox1")->AsShaderResource();
	g_pSkyBox2Variable = g_pEffect->GetVariableByName("txSkyBox2")->AsShaderResource();
	g_pSkyMixVariable = g_pEffect->GetVariableByName("SkyMix")->AsScalar();


	g_pCameraPosVariable = g_pEffect->GetVariableByName("CameraPos")->AsVector();
	g_pCameraUpVariable = g_pEffect->GetVariableByName("CameraUp")->AsVector();
	g_pCameraDirVariable = g_pEffect->GetVariableByName("CameraDir")->AsVector();
	g_pCameraRightVariable = g_pEffect->GetVariableByName("CameraRight")->AsVector();

	g_pLightVariable = g_pEffect->GetVariableByName("light");

	//CREATE LIGHTS AND MATERIALS
	//--------------------------------------------------------------------------------
	g_AmbientLight = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	ID3D10EffectVariable* pVar = g_pEffect->GetVariableByName("ambientLight");
	pVar->SetRawValue(&g_AmbientLight, 0, 16); //Przypisywanie wartoœci zmiennym shadera "na ¿ywca"

	g_Material.ambient = 0.1f;
	g_Material.diffuse = 0.5f;
	g_Material.specular = 0.5f;
	g_Material.shininess = 30;
	pVar = g_pEffect->GetVariableByName("material");
	pVar->SetRawValue(&g_Material, 0, sizeof(Material));

	g_DirectionalLight.color = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f); //kolor

																	// Load the Texture
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"weave_diffuse.jpg", NULL, NULL, &g_pTexture0SRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"weave_normal.jpg", NULL, NULL, &g_pTexture1SRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Medievil Stonework - Color Map.png", NULL, NULL, &g_pTexture2SRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Medievil Stonework - Normal Map.png", NULL, NULL, &g_pTexture3SRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Medievil Stonework - Specular Map.png", NULL, NULL, &g_pTextureSpecularSRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Medievil Stonework - AO Map.png", NULL, NULL, &g_pTextureAOSRV, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"TestNormalMap.png", NULL, NULL, &g_pTexture4SRV, NULL);
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
	hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"sky_18_cube.dds", NULL, NULL, &g_pSkyBoxSRVArray[3], NULL);
	if (FAILED(hr))
		return hr;

	//SkyBox
	D3D10_PASS_DESC PassDesc;
	// Define the input layout
	D3D10_INPUT_ELEMENT_DESC SkyBoxlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	int numElements = sizeof(SkyBoxlayout) / sizeof(SkyBoxlayout[0]);

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

	D3D10_BUFFER_DESC bd;
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SkyBoxVertex) * 12;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA InitData;
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

	//HeightField
	g_Terrain.HeightMapLoad("Teren.bmp", 3.0, 3.0, 10.0);

	// Create vertex buffer
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(HeightFieldVertex) * g_Terrain.NumVertices;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = g_Terrain.v;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pHeightFieldVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Create index buffer
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DWORD) * g_Terrain.NumFaces * 3;
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = g_Terrain.indices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pHeightFieldIndexBuffer);
	if (FAILED(hr))
		return hr;

	// Standardowa definicja formatu wierzcho³ka dla obiektu wczytanego z pliku obj
	const D3D10_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	numElements = sizeof(layout) / sizeof(layout[0]);

	// Create the input layout for normal render
	g_pNormalMap1Technique->GetPassByIndex(0)->GetDesc(&PassDesc);
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pVertexLayout);
	if (FAILED(hr))
		return hr;

	// Create the input layout for shadow render
	g_pShadowTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pShadowVertexLayout);
	if (FAILED(hr))
		return hr;

	//GAME OBJECTS
	g_TPPCamera.setPerspectiveProjectionLH(45.0, width / (FLOAT)height, 0.1f, 250.0f);
	g_DebugCamera.setPerspectiveProjectionLH(45.0, width / (FLOAT)height, 0.1f, 250.0f);

	g_pBall = new Object();
	g_pBall->LoadObjModel(g_pd3dDevice, "KulaLoPoly1.obj", g_TM, FALSE); //TS Normals

	g_pPlayer1 = new Object();
	g_pPlayer1->LoadObjModel(g_pd3dDevice, "KulaLoPoly2.obj", g_TM, FALSE); //OS Normals

	g_pPlayer2 = new Object();
	g_pPlayer2->LoadObjModel(g_pd3dDevice, "KulaLoPoly2.obj", g_TM, FALSE); //OS Normals

	g_pPlayer3 = new Object();
	g_pPlayer3->LoadObjModel(g_pd3dDevice, "KulaLoPoly2.obj", g_TM, FALSE); //OS Normals

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pd3dDevice) g_pd3dDevice->ClearState();

	if (g_pZBufferDSView) g_pZBufferDSView->Release();
	if (g_pZBufferSRView) g_pZBufferSRView->Release();
	if (g_pZBuffer) g_pZBuffer->Release();
	if (g_pShadowMapDSView) g_pShadowMapDSView->Release();
	if (g_pShadowMapSRView) g_pShadowMapSRView->Release();
	if (g_pShadowMap) g_pShadowMap->Release();

	if (g_pTexture0SRV) g_pTexture0SRV->Release();
	if (g_pTexture1SRV) g_pTexture1SRV->Release();
	if (g_pTexture2SRV) g_pTexture2SRV->Release();
	if (g_pTexture3SRV) g_pTexture3SRV->Release();
	if (g_pTexture4SRV) g_pTexture4SRV->Release();
	if (g_pSkyBoxSRVArray[0]) g_pSkyBoxSRVArray[0]->Release();
	if (g_pSkyBoxSRVArray[1]) g_pSkyBoxSRVArray[1]->Release();
	if (g_pSkyBoxSRVArray[2]) g_pSkyBoxSRVArray[2]->Release();
	if (g_pSkyBoxSRVArray[3]) g_pSkyBoxSRVArray[3]->Release();

	if (g_pEffect) g_pEffect->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

void ConnectPVD()
{
	// check if PvdConnection manager is available on this platform
	if (g_PhysicsSDK->getPvdConnectionManager() == NULL)
		return;

	// setup connection parameters
	const char*     pvd_host_ip = "127.0.0.1";  // IP of the PC which is running PVD
	int             port = 5425;         // TCP port to connect to, where PVD is listening
	unsigned int    timeout = 100;          // timeout in milliseconds to wait for PVD to respond,
											// consoles and remote PCs need a higher timeout.
	PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerExt::getAllConnectionFlags();

	debugger::comm::PvdConnection* theConnection = PxVisualDebuggerExt::createConnection(g_PhysicsSDK->getPvdConnectionManager(), pvd_host_ip, port, timeout, connectionFlags);
}

class CTI : // Implements PxSimulationEventCallback
	public PxSimulationEventCallback
{
public:
	CTI(void) {}
	virtual ~CTI(void) {}

	void onTrigger(PxTriggerPair* pairs, PxU32 count) //obs³uga informacji o trigerach
	{
		for (PxU32 i = 0; i < count; i++)
		{
			// ignore pairs when shapes have been deleted
			if (pairs[i].flags & (PxTriggerPairFlag::eDELETED_SHAPE_TRIGGER | PxTriggerPairFlag::eDELETED_SHAPE_OTHER))
				continue;

			if (pairs[i].triggerActor == g_PxBramka)
			{
				if (pairs[i].otherActor == g_PxBall)
				{
					if (pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
						//wejœcie obiektu g_PxBall do strefy triggera wyznaczonej przez g_PxBramka
						int s = 0;
					else
						//opuszczenie strefy triggera
						int s = 1;
				}
			}
		}
	}

	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{ //obs³uga informacji o zderzeniach obiektów
		const PxU32 bufferSize = 64;
		PxContactPairPoint contacts[bufferSize];
		for (PxU32 i = 0; i < nbPairs; i++) //rozpatrujemy kolejne pary zderzonych obiektów
		{
			const PxContactPair& cp = pairs[i];

			if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				if ((pairHeader.actors[0] == g_PxBall) || (pairHeader.actors[1] == g_PxBall)) //jeœli jednym z obiektów jest g_PxBall ...
				{
					PxActor* otherActor = (g_PxBall == pairHeader.actors[0]) ? pairHeader.actors[1] : pairHeader.actors[0]; //.. to odczytujemy ten drugi
					if (otherActor == g_PxPlayer1) // jeœli tym drugim jest g_PxPlayer1...
					{
						// ...to odczytujemy info o zderzeniu
						PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
						for (PxU32 j = 0; j < nbContacts; j++)
						{
							PxVec3 point = contacts[j].position; //punkt kontaktu
							PxVec3 impulse = contacts[j].impulse; //si³a
																  //printf("P %d : %.3f %.3f %.3f %.3f %.3f %.3f\n", j, point.x, point.y, point.z, impulse.x, impulse.y, impulse.z);
						}
						break;
					}
				}
			}
		}
	}
	//inne raportowane przez PhysX zdarzenia:
	void onConstraintBreak(PxConstraintInfo*, PxU32) {} //zerwanie po³¹czenia
	void onWake(PxActor**, PxU32) {} //wybudzenie aktora
	void onSleep(PxActor**, PxU32) {} //uœpienie aktora
};

PxFilterFlags customFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// all initial and persisting reports for everything, with per-point data
	pairFlags = PxPairFlag::eCONTACT_DEFAULT
		| PxPairFlag::eTRIGGER_DEFAULT
		| PxPairFlag::eNOTIFY_CONTACT_POINTS
		| PxPairFlag::eCCD_LINEAR; //Set flag to enable CCD (Continuous Collision Detection) 

	return PxFilterFlag::eDEFAULT;
}

void InitPhysX()
{
	g_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_DefaultAllocatorCallback, g_DefaultErrorCallback);
	if (!g_Foundation)
	{
		exit(1);
	}

	g_PhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *g_Foundation, PxTolerancesScale());
	if (g_PhysicsSDK == NULL)
	{
		exit(1);
	}

	ConnectPVD(); //po³¹czenie z PhysXVisualDebugger

				  //-------------- Creating scene --------------------------
	PxSceneDesc sceneDesc(g_PhysicsSDK->getTolerancesScale());	//Descriptor class for scenes 

	sceneDesc.gravity = PxVec3(0.0f, -5.0f, 0.0f);		//Setting gravity
	sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);	//Creates default CPU dispatcher for the scene
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
	sceneDesc.filterShader = customFilterShader; //filtr informacji z PhysX'a
	sceneDesc.simulationEventCallback = new CTI(); //obiekt przetwarzaj¹cy informacje z PhysX'a
	g_PxScene = g_PhysicsSDK->createScene(sceneDesc);				//Creates a scene 


																	//Creating PhysX material
	PxMaterial* material = g_PhysicsSDK->createMaterial(0.5, 0.5, 0.8); //Creating a PhysX material

																		//1-Creating static plane	 
	PxTransform planePos = PxTransform(PxVec3(0.0f, -0.5f, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));	//Position and orientation(transform) for plane actor  
	PxRigidStatic* plane = g_PhysicsSDK->createRigidStatic(planePos);								//Creating rigid static actor	
	plane->createShape(PxPlaneGeometry(), *material);												//Defining geometry for plane actor
	g_PxScene->addActor(*plane);																	//Adding plane actor to PhysX scene


																									//2-Creating dynamic actors																		 									 
	PxSphereGeometry	sphereGeometry(1);
	PxTransform		spherePos1(PxVec3(6.0f, 28.0f, 0.0f));
	g_PxBall = PxCreateDynamic(*g_PhysicsSDK, spherePos1, sphereGeometry, *material, 0.02f);
	g_PxBall->setLinearDamping(0.1);
	g_PxBall->setAngularDamping(2);
	g_PxScene->addActor(*g_PxBall);

	PxTransform		spherePos2(PxVec3(-6.0f, 32.0f, 0.0f));
	g_PxPlayer1 = PxCreateDynamic(*g_PhysicsSDK, spherePos2, sphereGeometry, *material, 0.02f);
	g_PxPlayer1->setLinearDamping(0.05);
	g_PxPlayer1->setAngularDamping(0.05);
	g_PxScene->addActor(*g_PxPlayer1);

	

	PxTransform		spherePos3(PxVec3(-12.0f, 22.0f, 0.0f));
	g_PxPlayer2 = PxCreateDynamic(*g_PhysicsSDK, spherePos3, sphereGeometry, *material, 0.02f);
	g_PxPlayer2->setLinearDamping(0.05);
	g_PxPlayer2->setAngularDamping(0.05);
	g_PxScene->addActor(*g_PxPlayer2);

	PxTransform		spherePos4(PxVec3(12.0f, 42.0f, 0.0f));
	g_PxPlayer3 = PxCreateDynamic(*g_PhysicsSDK, spherePos4, sphereGeometry, *material, 0.02f);
	g_PxPlayer3->setLinearDamping(0.05);
	g_PxPlayer3->setAngularDamping(0.05);
	g_PxScene->addActor(*g_PxPlayer3);


	PxTransform		bramkaPos(PxVec3(0.0f, 1.0f, 0.0f));
	PxBoxGeometry bramkaGeometry(1.0f, 1.0f, 1.0f);
	g_PxBramka = PxCreateDynamic(*g_PhysicsSDK, bramkaPos, bramkaGeometry, *material, 0.8f);
	PxShape* triggerShape;
	g_PxBramka->getShapes(&triggerShape, 1);
	triggerShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
	triggerShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	g_PxBramka->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true); //b³¹d w SDK ???
	g_PxScene->addActor(*g_PxBramka);

	PxTransform		cameraPos(PxVec3(0.0f, 24.0f, -6.0f));
	PxSphereGeometry cameraGeometry(0.3);
	g_PxCamera = PxCreateDynamic(*g_PhysicsSDK, cameraPos, cameraGeometry, *material, 0.01f);
	g_PxCamera->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
	g_PxCamera->setLinearDamping(5);
	g_PxCamera->setAngularDamping(5);
	g_PxScene->addActor(*g_PxCamera);


	PxTransform		kinematicPos(PxVec3(0.0f, 24.0f, 0.0f));
	PxSphereGeometry kinematicGeometry(0.2);
	g_PxKinematic = PxCreateDynamic(*g_PhysicsSDK, kinematicPos, kinematicGeometry, *material, 0.01f);
	g_PxKinematic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	g_PxScene->addActor(*g_PxKinematic);

	
	//create a joint (sprê¿yna miêdzy kamer¹ PxKinematik)
	PxDistanceJoint* j = PxDistanceJointCreate(*g_PhysicsSDK, g_PxKinematic, PxTransform::createIdentity(), g_PxCamera, PxTransform::createIdentity());
	j->setDamping(10);
	j->setStiffness(100);
	j->setMinDistance(5);
	j->setMaxDistance(12);
	j->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
	j->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
	j->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
	j->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);
	

	//heightfield

	HeightField *hf = new HeightField(g_Terrain, *g_PhysicsSDK, *material, g_Terrain.hminfo.terrainWidth, g_Terrain.hminfo.terrainHeight);
	g_PxScene->addActor(*(hf->g_pxHeightField));

	{	//PxHeightFieldSample* samples = (PxHeightFieldSample*)malloc(sizeof(PxHeightFieldSample)*(g_Terrain.NumVertices));
	//
	//	for (int i = 0; i < g_Terrain.NumVertices; i++)
	//{
	//	samples[i].height = g_Terrain.hminfo.heightMap[i].y;
	//	samples[i].clearTessFlag();
	//} 
	//

	//PxHeightFieldDesc hfDesc;
	//hfDesc.format = PxHeightFieldFormat::eS16_TM;
	//hfDesc.nbColumns = g_Terrain.hminfo.terrainWidth;
	//hfDesc.nbRows = g_Terrain.hminfo.terrainHeight;
	//hfDesc.samples.data = samples;
	//hfDesc.samples.stride = sizeof(PxHeightFieldSample);

	//PxHeightField* aHeightField = g_PhysicsSDK->createHeightField(hfDesc);

	//PxHeightFieldGeometry hfGeom(aHeightField, PxMeshGeometryFlags(), g_Terrain.dy / 255.0, g_Terrain.dx, g_Terrain.dz);

	//
	//PxTransform  TerrainPos(PxVec3(-g_Terrain.dx*(g_Terrain.hminfo.terrainWidth - 1) / 2, 0.0f, g_Terrain.dz*(g_Terrain.hminfo.terrainHeight - 1) / 2), PxQuat(3.1415 / 2.0, PxVec3(0, 1, 0)));
	//g_PxHeightfield = g_PhysicsSDK->createRigidDynamic(TerrainPos);
	//g_PxHeightfield->setRigidDynamicFlag(PxRigidDynamicFlag::eKINEMATIC, true);
	////PxShape* aHeightFieldShape = g_PxHeightfield->createShape(hfGeom, *material);
	}

	

}

void ShutdownPhysX()				//Shutdown PhysX
{
	g_PhysicsSDK->release();			//Removes any actors, particle systems, and constraint shaders from this scene
	g_Foundation->release();			//Destroys the instance of foundation SDK
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	D3DXMATRIX World, World2, View, Projection, InvView, InvProjection;
	UINT stride, offset = 0;
	DWORD ActTime = GetTickCount();

	//update position of kinematic actor acc. to Player 1 position
	PxMat44 kPos = PxMat44(g_PxPlayer1->getGlobalPose());
	PxVec3 asd;
	asd.x = kPos(0, 3);
	asd.y = kPos(1, 3) + 3.0f; //odtwarza ruch kulki - jest zawsze ponad kul¹ o 3 jednostki
	asd.z = kPos(2, 3);
	PxTransform kino(asd);
	g_PxKinematic->setKinematicTarget(kino);

	//update camera position & direction (view & projection)
	PxMat44 tsPos = PxMat44(g_PxCamera->getGlobalPose());
	g_TPPCamera.update(tsPos(0, 3), tsPos(1, 3), tsPos(2, 3)); // kamera TPP ustawiona w lokalizacji œrodka obiektu fizycznego PxCamera
	if (g_ActiveCamera) g_DebugCamera.update(0.05); //kamerê debug posuwamy w kierunku ustalonym z klawiatury

													//zapisujemy macierze do karty graficznej
	View = g_pActiveCamera->getViewMatrix();
	g_pViewVariable->SetMatrix((float*)View);
	D3DXMatrixInverse(&InvView, NULL, &View);
	g_pInvViewVariable->SetMatrix((float*)InvView);
	g_pCameraPosVariable->SetFloatVector((float*)g_pActiveCamera->getCameraPosition());
	g_pCameraUpVariable->SetFloatVector((float*)g_pActiveCamera->getCameraUp());
	g_pCameraRightVariable->SetFloatVector((float*)g_pActiveCamera->getCameraRight());
	g_pCameraDirVariable->SetFloatVector((float*)g_pActiveCamera->getCameraDirection());
	if (g_pActiveCamera->isProjectionDirty())
	{
		g_Projection = g_pActiveCamera->getProjectionMatrix();
		g_pProjectionVariable->SetMatrix((float*)g_Projection);
		D3DXMatrixInverse(&InvProjection, NULL, &g_Projection);
		g_pInvProjectionVariable->SetMatrix((float*)InvProjection);
	}
	g_pViewProjectionVariable->SetMatrix((float*)(View*g_Projection));

	//update camera in PhysX Visual Debuger
	PxVec3 pEye = PxVec3(g_pActiveCamera->getCameraPosition().x, g_pActiveCamera->getCameraPosition().y, -g_pActiveCamera->getCameraPosition().z);
	PxVec3 pDir = PxVec3(g_pActiveCamera->getCameraDirection().x, g_pActiveCamera->getCameraDirection().y, -g_pActiveCamera->getCameraDirection().z);
	PxVec3 pUp = PxVec3(g_pActiveCamera->getCameraUp().x, g_pActiveCamera->getCameraUp().y, -g_pActiveCamera->getCameraUp().z);
	g_PhysicsSDK->getVisualDebugger()->updateCamera("CAMERA", pEye, pUp, pEye + pDir);

	//update sun position & get light direction, view and projection
	/*g_Sun.Update(ActTime);
	g_Sun.getLightDirection(g_DirectionalLight.direction);
	g_pLightVariable->SetRawValue(&g_DirectionalLight, 0, sizeof(DirectionalLight));*/

	// Initialize LightViewProjection matrices for ShadowMap rendering
	D3DXMATRIX mtmp, mtmp2, lvp;
	D3DXVECTOR3 LEye = -60.0f * g_DirectionalLight.direction; //-60 * wektor jednostkowy kierunku padania œwiat³a
	D3DXVECTOR3 LAt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 Up;
	//wyznaczenie Up na podstawie Dir i UpWorld (Ortogonalizacja Grama-Schmidta)
	D3DXVECTOR3 UpWorld(0.0, 1.0, 0.0);
	float d = D3DXVec3Dot(&g_DirectionalLight.direction, &UpWorld);
	Up = UpWorld - g_DirectionalLight.direction * d;
	D3DXVec3Normalize(&Up, &Up);

	D3DXMatrixLookAtLH(&mtmp, &LEye, &LAt, &Up);
	D3DXMatrixOrthoLH(&mtmp2, 20, 20, 0.1f, 500.0f);
	D3DXMatrixMultiply(&lvp, &mtmp, &mtmp2);
	g_pLightViewProjectionVariable->SetMatrix((float*)&lvp);
	//ALBO
	//g_pLightViewProjectionVariable->SetMatrix(( float* ) g_Sun.getLightViewProjection(g_pActiveCamera, &g_Terrain) ); //dostosowuje przestrzeñ widzian¹ przez kamerê ORTHO do sceny

	//---RENDER SHADOW-MAP---
	g_pd3dDevice->RSSetViewports(1, &g_ShadowMapViewport);
	g_pd3dDevice->OMSetRenderTargets(0, 0, g_pShadowMapDSView);
	g_pd3dDevice->ClearDepthStencilView(g_pShadowMapDSView, D3D10_CLEAR_DEPTH, 1.0f, 0); // Clear the depth buffer to 1.0 (max depth)	

	D3DXMatrixIdentity(&World);
	g_pWorldVariable->SetMatrix((float*)&World);
	stride = sizeof(HeightFieldVertex);
	offset = 0;
	g_pd3dDevice->IASetInputLayout(g_pShadowVertexLayout);
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pHeightFieldVertexBuffer, &stride, &offset);
	g_pd3dDevice->IASetIndexBuffer(g_pHeightFieldIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pShadowTechnique->GetPassByIndex(0)->Apply(0);
	g_pd3dDevice->DrawIndexed(3 * g_Terrain.NumFaces, 0, 0);

	PxMat44 sPos = PxMat44(g_PxBall->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pBall->Render(g_pd3dDevice, g_TM, g_pShadowTechnique, g_pShadowVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer1->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer1->Render(g_pd3dDevice, g_TM, g_pShadowTechnique, g_pShadowVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer2->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer2->Render(g_pd3dDevice, g_TM, g_pShadowTechnique, g_pShadowVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer3->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer3->Render(g_pd3dDevice, g_TM, g_pShadowTechnique, g_pShadowVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	//---RENDER SCENE---
	g_pd3dDevice->RSSetViewports(1, &g_RenderTargetViewport);
	g_pd3dDevice->OMSetRenderTargets(1, &g_pRenderTargetView, g_pZBufferDSView);
	g_pd3dDevice->ClearDepthStencilView(g_pZBufferDSView, D3D10_CLEAR_DEPTH, 1.0f, 0); // Clear the depth buffer to 1.0 (max depth)
																					   //   float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // red, green, blue, alpha
																					   //   g_pd3dDevice->ClearRenderTargetView( g_pRenderTargetView, ClearColor ); // Nie czyœcimy bufora koloru, bo i tak zamalujemy ca³y ekran niebem
	g_pShadowMapVariable->SetResource(g_pShadowMapSRView); //z-bufor jako tekstura shadow-map 

	g_pColorMapVariable->SetResource(g_pTexture2SRV); //0, 2, 4
	g_pNormalMapVariable->SetResource(g_pTexture3SRV); //1, 3, 4
	g_pSpecularMapVariable->SetResource(g_pTextureSpecularSRV); //0, 2, 4
	g_pAOMapVariable->SetResource(g_pTextureAOSRV); //1, 3, 4	
	D3DXMatrixIdentity(&World);
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pInvWorldVariable->SetMatrix((float*)&World);
	stride = sizeof(HeightFieldVertex);
	offset = 0;
	g_pd3dDevice->IASetInputLayout(g_pVertexLayout);
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pHeightFieldVertexBuffer, &stride, &offset);
	g_pd3dDevice->IASetIndexBuffer(g_pHeightFieldIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pNormalMap4Technique->GetPassByIndex(0)->Apply(0);
	g_pd3dDevice->DrawIndexed(3 * g_Terrain.NumFaces, 0, 0);

	sPos = PxMat44(g_PxBall->getGlobalPose()); //odczytujemy po³o¿enie i rotacjê obiektu na scenie PhysX...
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World); //... i renderujemy w tym po³o¿eniu obiekt graficzny
	g_pBall->Render(g_pd3dDevice, g_TM, g_pNormalMap3Technique, g_pVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer1->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer1->Render(g_pd3dDevice, g_TM, g_pTextureTechnique, g_pVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer2->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer2->Render(g_pd3dDevice, g_TM, g_pNormalMap2Technique, g_pVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	sPos = PxMat44(g_PxPlayer3->getGlobalPose());
	memcpy(&World._11, sPos.front(), 4 * 4 * sizeof(float));
	g_pWorldVariable->SetMatrix((float*)&World);
	g_pPlayer3->Render(g_pd3dDevice, g_TM, g_pNormalMap2Technique, g_pVertexLayout, g_pWorldVariable, g_pInvWorldVariable, g_pColorMapVariable, g_pNormalMapVariable, &World);

	//render skybox
	g_Sky.Update(ActTime - g_TimeStart); //ustalamy wsp. mieszania dwóch tekstur zgodnie z aktualnym czasem
	g_pSkyMixVariable->SetFloat(g_Sky.t);
	g_pSkyBox1Variable->SetResource(g_pSkyBoxSRVArray[g_Sky.T1]);
	g_pSkyBox2Variable->SetResource(g_pSkyBoxSRVArray[g_Sky.T2]);

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
	g_pd3dDevice->DrawIndexed(60, 0, 0);

	g_pd3dDevice->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	g_pDepthMapVariable->SetResource(g_pZBufferSRView);

	//tu bêdziemy renderowaæ z wykorzystaniem aktuaknego zbufora jako tekstury wejœciowej shadera (woda, ogieñ, dym, itp.)

	g_pShadowMapVariable->SetResource(0);
	g_pDepthMapVariable->SetResource(0);
	g_pSwapChain->Present(0, 0);     // Present our back buffer to our front buffer
}
