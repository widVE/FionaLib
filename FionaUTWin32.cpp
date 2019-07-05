#include "FionaUT.h"

#include <GL/wglew.h>
#include <string>
const char _CLASSNAME[]= "FionaUTClass";
const char _CLASSNAME2[]= "FionaUTClass2";

struct WINDOW_INFO
{
	HWND hwnd;
	HDC hdc;
	HGLRC hrc;
	HDC hdc2;	//second GPU affinity device context...
	HGLRC hrc2;	//second context made from hdc2...
};

static WINDOW_INFO info[MAX_WINDOW];
static HGLRC sharedRC=NULL;

static bool GLEWInited = false;

#define ATTRIB(a) attributes[where++]=(a)
#define ATTRIB_VAL(a,v) {ATTRIB(a); ATTRIB(v);}

static void ___FillPFD( PIXELFORMATDESCRIPTOR *ppfd, HDC hdc)
{
	int flags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SWAP_EXCHANGE;
	if( fionaGlobalMode & GLUT_DOUBLE ) flags |= PFD_DOUBLEBUFFER;
	if( fionaGlobalMode & GLUT_STEREO ) flags |= PFD_STEREO;
	
	/* Specify which pixel format do we opt for... */
	ZeroMemory(ppfd,sizeof(PIXELFORMATDESCRIPTOR));
	ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	ppfd->nVersion = 1;
	ppfd->dwFlags = flags;
	
	ppfd->iPixelType = PFD_TYPE_RGBA;
	ppfd->cRedBits=8;
	ppfd->cGreenBits=8;
	ppfd->cBlueBits=8;
	ppfd->cAlphaBits = 8;// (fionaGlobalMode & GLUT_ALPHA) ? 8 : 0;
	ppfd->cColorBits = 32;
	ppfd->cAccumBits = (fionaGlobalMode & GLUT_ACCUM ) ? 1 : 0;
	
	/* Hmmm, or 32/0 instead of 24/8? */
	ppfd->cDepthBits = 32;
	ppfd->cStencilBits = 0;
	//ppfd->cDepthBits = 24;
	//ppfd->cStencilBits = 8;
	ppfd->iLayerType = PFD_MAIN_PLANE;  
	ppfd->cColorBits = (BYTE) GetDeviceCaps( hdc, BITSPIXEL );
}

static int ___IsExtensionSupported( HDC hdc, const char *extension )
{
	const char *pWglExtString;
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetEntensionsStringARB =
	(PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
	if ( wglGetEntensionsStringARB == NULL ) return FALSE;
	pWglExtString = wglGetEntensionsStringARB( hdc );
	return ( pWglExtString != NULL ) && ( strstr(pWglExtString, extension) != NULL );
}

static void ___FillPixelFormatAttributes( int *attributes, const PIXELFORMATDESCRIPTOR *ppfd, int nSample=4 )
{
	int where = 0;
	
	ATTRIB_VAL( WGL_DRAW_TO_WINDOW_ARB, GL_TRUE );
	ATTRIB_VAL( WGL_SUPPORT_OPENGL_ARB, GL_TRUE );
	ATTRIB_VAL( WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB );
	
	ATTRIB_VAL( WGL_COLOR_BITS_ARB, ppfd->cColorBits );
	ATTRIB_VAL( WGL_ALPHA_BITS_ARB, ppfd->cAlphaBits );
	ATTRIB_VAL( WGL_DEPTH_BITS_ARB, ppfd->cDepthBits );
	ATTRIB_VAL( WGL_STENCIL_BITS_ARB, ppfd->cStencilBits );
	
	ATTRIB_VAL( WGL_DOUBLE_BUFFER_ARB, (fionaGlobalMode & GLUT_DOUBLE ) != 0 );
	ATTRIB_VAL( WGL_STEREO_ARB, (fionaGlobalMode & GLUT_STEREO) != 0 );
	ATTRIB_VAL( WGL_SAMPLE_BUFFERS_ARB, GL_TRUE );
	ATTRIB_VAL( WGL_SAMPLES_ARB, nSample );
	ATTRIB( 0 );
}


LRESULT	CALLBACK __FionaUTWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DWORD WINAPI __FionaInputThreadFunc(LPVOID lpParam)
{
	//This loops handles inputs from the command window.
	while(true)
	{
		////The correct format for commands is action=value
		std::string sInput;
		std::getline(std::cin, sInput);
		fionaCommandQueue.push(sInput);
	}
}

static void __FionaUTCreateWindowCB(HWND hwnd);


// First Create function is called
static void __FionaUTCreateWindowCB(HWND hwnd)
{
	GLboolean success;

	HDC hDC; HGLRC hRC=NULL;
	int i=0; for(i=0;i<MAX_WINDOW;i++) if(info[i].hwnd==NULL) break;
	info[i].hwnd = hwnd;
	info[i].hdc = hDC = GetDC( hwnd );

	// Begin of hack
	PIXELFORMATDESCRIPTOR pfd;
	PIXELFORMATDESCRIPTOR* ppfd = &pfd;
	int pixelformat;
	___FillPFD( ppfd, hDC );
	pixelformat = ChoosePixelFormat( hDC, ppfd );
	printf("Window DC Pixel format choosen: %d\n",pixelformat);

	//TESTING openGL 3.0...
	/*int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};*/
	
	//testing debug_arb..

	int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB/*, WGL_CONTEXT_DEBUG_BIT_ARB, WGL_CONTEXT_PROFILE_MASK, WGL_CONTEXT_CORE_PROFILE_BIT_ARB*/,
		0
	};

	// windows hack for multismapling/sRGB 
	if ( fionaGlobalMode & GLUT_MULTISAMPLE )
	{
		HWND hWnd;
		HINSTANCE instance = GetModuleHandle(NULL);
		HGLRC rc, rc_before=wglGetCurrentContext();
		HDC hdc, hDC_before=wglGetCurrentDC();
		WNDCLASS wndCls;

		// create a dummy window 
		ZeroMemory(&wndCls, sizeof(wndCls));
		wndCls.lpfnWndProc = DefWindowProc;
		wndCls.hInstance = instance;
		wndCls.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wndCls.lpszClassName = "FIONA_dummy";
		RegisterClass( &wndCls );

		hWnd=CreateWindow("FIONA_dummy", "", WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW , 0,0,0,0, 0, 0, instance, 0 );
		hdc=GetDC(hWnd);
		SetPixelFormat( hdc, pixelformat, ppfd );

		if(wglewIsSupported("WGL_ARB_create_context"))
		{
			rc = wglCreateContextAttribsARB(hdc, NULL, attributes);
		}
		else
		{
			rc = wglCreateContext( hdc );
		}
		if( rc==0 ) printf("WARNING: Dummy GLRC Creation failed. No Multisampling Supported!\n");
		wglMakeCurrent(hdc, rc);

		if ( ___IsExtensionSupported( hdc, "WGL_ARB_multisample" ) )
		{
			PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBProc =
				(PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
			if ( wglChoosePixelFormatARBProc )
			{
				printf("Setting multisample extension\n");
				int attributes[100], iPixelFormat;
				float fAttributes[] = { 0, 0 };
				UINT numFormats;
				___FillPixelFormatAttributes( attributes, ppfd, fionaConf.multisample );
				BOOL bValid = wglChoosePixelFormatARBProc(hdc, attributes, fAttributes, 1, &iPixelFormat, &numFormats);
				if ( bValid && numFormats > 0 ) pixelformat = iPixelFormat;
			}
		}
		else
		{
			printf("WARNING: WGL_ARB_multisample Extension not supported\n");
		}

		wglMakeCurrent(hDC_before, rc_before);
		wglDeleteContext(rc);
		ReleaseDC(hWnd, hdc);
		DestroyWindow(hWnd);
		UnregisterClass("FIONA_dummy", instance);
	}
	success = ( pixelformat != 0 ) && ( SetPixelFormat( hDC, pixelformat, ppfd ) );
	
	// End of hack
	if(wglewIsSupported("WGL_ARB_create_context"))
	{
		if(fionaConf.twoWindows && _FionaUTIsCAVEMachine())
		{
			info[i].hrc = hRC = wglCreateContext( hDC );
			if(info[i].hrc)
			{
				printf("Successfully created 2nd window context\n");
			}
		}
		else
		{
			info[i].hrc = hRC = wglCreateContextAttribsARB(hDC, NULL, attributes);
			printf("*****in attribs create context\n");
		}
	}
	else
	{
		//if (!fionaConf.useSecondGPU || fionaConf.appType == FionaConfig::HEADNODE)
		{
			info[i].hrc = hRC = wglCreateContext(hDC);
			if (info[i].hrc)
			{
				printf("Successfully created 1st openGL context\n");
			}
		}
	}

	//if (!fionaConf.useSecondGPU || fionaConf.appType == FionaConfig::HEADNODE)
	{
		if (hRC == NULL && sharedRC == NULL)
		{
			printf("Cannot create GLRC: Fatal error\n");
			FionaUTExit(1);
		}

		if (hRC == NULL)
		{
			hRC = sharedRC; // Cannot make HRC.. Let's share
		}
		else if (sharedRC == NULL)
		{
			sharedRC = hRC; // It is the first RC.. set it as the shared one
		}
		else // We have shared RC and new RC.. we need to set them to share lists
		{
			if (!fionaConf.twoWindows)
			{
				wglShareLists(sharedRC, hRC);
			}
		}

		// Test phase
		wglMakeCurrent(0, 0);
		success = wglMakeCurrent(hDC, hRC);
		if (fionaConf.twoWindows && hwnd == fionaWinConf[1].window)
		{
			glewInit();
		}

		if (!success) printf("wglMakeCurrent failed!!!!\n");
		if (fionaGlobalMode & GLUT_MULTISAMPLE)
		{
			glEnable(GL_MULTISAMPLE);
			int iNumSamples;
			glGetIntegerv(GL_SAMPLES, &iNumSamples);
			if (iNumSamples == 0) printf("WARNING: Multisampling is not working..\n       See previous warning or Just something is wrong.\n");
			if (iNumSamples < fionaConf.multisample) printf("NOTICE: Number of sampling has been set to lower than you specified %d vs %d\n", iNumSamples, fionaConf.multisample);
		}

		if (!GLEWInited)
		{
			glewInit();
			GLEWInited = true;
		}
	}

	if (fionaConf.useSecondGPU)
	{
		if (_FionaUTIsCAVEMachine())
		{
			UINT GPUIdx;
			UINT displayDeviceIdx;
			GPU_DEVICE gpuDevice;
			HGPUNV hGPU;
			HGPUNV gpuMask[2];
			bool bDisplay, bPrimary;

			gpuDevice.cb = sizeof(gpuDevice);
			GPUIdx = 0;
			printf("Enumerating GPUs...\n");

			while (wglEnumGpusNV(GPUIdx, &hGPU)) // First call this function to get a handle to the gpu
			{
				printf("Device# %d:\n", GPUIdx);

				bDisplay = false;
				bPrimary = false;

				// Now get the detailed information about this device:
				// how many displays it's attached to
				displayDeviceIdx = 0;
				while (wglEnumGpuDevicesNV(hGPU, displayDeviceIdx, &gpuDevice))
				{
					bDisplay = true;
					bPrimary |= (gpuDevice.Flags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;

					printf(" Display# %d:\n", displayDeviceIdx);
					printf("  Name: %s\n", gpuDevice.DeviceName);
					printf("  String: %s\n", gpuDevice.DeviceString);

					if (gpuDevice.Flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
					{
						printf("  Attached to the desktop: LEFT=%d, RIGHT=%d, TOP=%d, BOTTOM=%d\n",
							gpuDevice.rcVirtualScreen.left, gpuDevice.rcVirtualScreen.right, gpuDevice.rcVirtualScreen.top, gpuDevice.rcVirtualScreen.bottom);
					}
					else
					{
						printf("  Not attached to the desktop\n");
					}

					// See if it's the primary GPU
					if (gpuDevice.Flags & DISPLAY_DEVICE_PRIMARY_DEVICE)
					{
						printf("  This is the PRIMARY Display Device\n");
					}
					displayDeviceIdx++;
				}

				if (!bPrimary) {
					gpuMask[0] = hGPU;
					gpuMask[1] = NULL;

					HDC srcDC = wglCreateAffinityDCNV(gpuMask);
					if (!srcDC)
					{
						printf("Couldn't create 2nd affinity DC\n");
						//return false;
					}

					PIXELFORMATDESCRIPTOR pfd2;
					PIXELFORMATDESCRIPTOR* ppfd2 = &pfd2;

					info[0].hdc2 = srcDC;
					___FillPFD(ppfd2, info[0].hdc2);
					int pixelformat2 = ChoosePixelFormat(info[0].hdc2, ppfd2);
					printf("2nd Pixel format choosen: %d\n", pixelformat2);
					if (SetPixelFormat(info[0].hdc2, pixelformat2, ppfd2))
					{
						info[0].hrc2 = wglCreateContext(info[0].hdc);
						if (info[0].hrc2 != 0)
						{
							//can't share lists across graphics cards...
							//__FionaUTMakeCurrent(info[0].hwnd, true);
							/*wglMakeCurrent(0,0);
							//BOOL shared = wglShareLists(info[0].hrc2, hrcTest);*/
							printf("Successfully created OpenGL Context on 2nd GPU!\n");
						}
					}
					else
					{
						printf("Couldn't set pixel format for second GPU!\n");
					}
				}

				GPUIdx++;
			}
		}

		if (!GLEWInited)
		{
			glewInit();
			GLEWInited = true;
		}
	}

	if(fionaConf.multiGPU || fionaConf.twoWindows)
	{
		if(_FionaUTIsCAVEMachine())
		{
			UINT GPUIdx;
			UINT displayDeviceIdx;
			GPU_DEVICE gpuDevice;
			HGPUNV hGPU;
			HGPUNV gpuMask[2];
			bool bDisplay, bPrimary;

			gpuDevice.cb = sizeof(gpuDevice);
			GPUIdx = 0;

			while(wglEnumGpusNV(GPUIdx, &hGPU)) // First call this function to get a handle to the gpu
			{
				printf("Device# %d:\n", GPUIdx);

				bDisplay = false;
				bPrimary = false;

				// Now get the detailed information about this device:
				// how many displays it's attached to
				displayDeviceIdx = 0;
				while(wglEnumGpuDevicesNV(hGPU, displayDeviceIdx, &gpuDevice))
				{			
					bDisplay = true;
					bPrimary |= (gpuDevice.Flags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;

					printf(" Display# %d:\n", displayDeviceIdx);
					printf("  Name: %s\n", gpuDevice.DeviceName);
					printf("  String: %s\n", gpuDevice.DeviceString);
                
					if(gpuDevice.Flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
					{
						printf("  Attached to the desktop: LEFT=%d, RIGHT=%d, TOP=%d, BOTTOM=%d\n",
							gpuDevice.rcVirtualScreen.left, gpuDevice.rcVirtualScreen.right, gpuDevice.rcVirtualScreen.top, gpuDevice.rcVirtualScreen.bottom);
					}
					else
					{
						printf("  Not attached to the desktop\n");
					}
                  
					// See if it's the primary GPU
					if(gpuDevice.Flags & DISPLAY_DEVICE_PRIMARY_DEVICE)
					{
						printf("  This is the PRIMARY Display Device\n");
					}
					displayDeviceIdx++;
				}

				if(bPrimary) { //primary GPU create the destination here, for Win7 default dc is bound to primary , so not really needed

					if(fionaConf.twoWindows && hwnd != info[1].hwnd)
					{
						// Create the destination on the primary
						gpuMask[0] = hGPU;
						gpuMask[1] = NULL;

						HDC hdcTest = wglCreateAffinityDCNV(gpuMask);
						if(!hdcTest)
						{
							printf("Coudln't create 1st affinity DC\n");
							//MessageBox(state->_main._hWnd, "wglCreateAffinityDCNV failed", "Error", MB_OK); 
							//return false;
						}
					
						PIXELFORMATDESCRIPTOR pfd2;
						PIXELFORMATDESCRIPTOR* ppfd2 = &pfd2;
						___FillPFD( ppfd2, hdcTest );

						int pixelformat2 = ChoosePixelFormat( hdcTest, ppfd2 );
						printf("1st Pixel format choosen: %d\n",pixelformat2);
						printf(" This is the destination GPU\n");
						if(SetPixelFormat(hdcTest, pixelformat2, ppfd2))
						{
							HGLRC hrcTest = wglCreateContext(hdcTest);
							if(hrcTest != 0)
							{
								printf("Made first affinity DC\n");
								info[i].hrc = hrcTest;
							}
						}

						wglMakeCurrent(hDC, 0);
						wglMakeCurrent(hDC, info[i].hrc);
						glewInit();
						break;
					}
					else if(fionaConf.multiGPU)
					{
						gpuMask[0] = hGPU;
						gpuMask[1] = NULL;

						HDC srcDC = wglCreateAffinityDCNV(gpuMask);
						if(!srcDC)
						{
							printf("Couldn't create 2nd affinity DC\n");
							//return false;
						}
						else
						{
													
							PIXELFORMATDESCRIPTOR pfd2;
							PIXELFORMATDESCRIPTOR* ppfd2 = &pfd2;
							printf("Made first affinity DC\n");
							/*___FillPFD( ppfd2, srcDC );
							int pixelformat2 = ChoosePixelFormat( srcDC, ppfd2 );
							printf("1st Affinity Pixel format choosen: %d\n",pixelformat2);
							if(SetPixelFormat(srcDC, pixelformat2, ppfd2))
							{
								info[0].hrc = wglCreateContext(srcDC);
								if(info[0].hrc != 0)
								{
									//can't share lists across graphics cards...
									printf("Successfully created 1st affinity openGL context!\n");
								}
							}*/
						}
					}
				}
				else  { //this is not the primary GPU so make it the source GPU
					// Create the source on any other
					//if we have two windows option set, make sure we're setting this info for the 2nd window..
					if(!fionaConf.twoWindows || hwnd == info[1].hwnd)
					{
						gpuMask[0] = hGPU;
						gpuMask[1] = NULL;

						HDC srcDC = wglCreateAffinityDCNV(gpuMask);
						if(!srcDC)
						{
							printf("Couldn't create 2nd affinity DC\n");
							//return false;
						}
						
						PIXELFORMATDESCRIPTOR pfd2;
						PIXELFORMATDESCRIPTOR* ppfd2 = &pfd2;

						if(fionaConf.twoWindows)
						{
							//info[1].hdc = srcDC;
							___FillPFD( ppfd2, srcDC );
							int pixelformat2 = ChoosePixelFormat( srcDC, ppfd2 );
							printf("2nd Pixel format choosen: %d\n",pixelformat2);
							if(SetPixelFormat(srcDC, pixelformat2, ppfd2))
							{
								HGLRC hrcTest = wglCreateContext(srcDC);
								if(hrcTest != 0)
								{
									info[1].hrc = hrcTest;
									printf("Successfully created 2nd window context!\n");
									wglMakeCurrent(hDC, 0);
									wglMakeCurrent(hDC, info[1].hrc);
									glewInit();
									break;
								}
							}
							else
							{
								printf("Couldn't set pixel format for second window!\n");
							}
						}
						else
						{
							info[0].hdc2 = srcDC;
							___FillPFD( ppfd2, info[0].hdc2 );
							int pixelformat2 = ChoosePixelFormat( info[0].hdc2, ppfd2 );
							printf("2nd Pixel format choosen: %d\n",pixelformat2);
							if(SetPixelFormat(info[0].hdc2, pixelformat2, ppfd2))
							{
								info[0].hrc2 = wglCreateContext(info[0].hdc2);
								if(info[0].hrc2 != 0)
								{
									//can't share lists across graphics cards...
									//__FionaUTMakeCurrent(info[0].hwnd, true);
									/*wglMakeCurrent(0,0);
									//BOOL shared = wglShareLists(info[0].hrc2, hrcTest);*/
									printf("Successfully created 2nd openGL context!\n");
								}
							}
							else
							{
								printf("Couldn't set pixel format for second context!\n");
							}

							printf(" This is the source GPU\n");
						}
					}
				}
        
				GPUIdx++;
			}
		}
	}

	if( fionaNetSlave!=NULL || fionaConf.appType==FionaConfig::DEVLAB )
	{
		if( wglSwapIntervalEXT!=NULL ) 
		{
			//Ross 5/20/2015 - according to https://www.opengl.org/wiki/Swap_Interval - this turns on vsync... (setting it to 1)
			//but only if the hardware setting in the control panel is set to "use application setting"
			//printf("Set swap interval to 0...\n");
			//wglSwapIntervalEXT(0);
		}
	}

	if(fionaConf.multiGPU)
	{
		wglMakeCurrent( hDC, hRC );
	}

	//we always want to setup the hardware sync when in the CAVE.. this avoids tearing issues.
	if(_FionaUTIsCAVEMachine() && fionaConf.hardwareSync)
	{
		printf("Setting up nVidia Quadro Hardware Sync\n");
		unsigned int group=0, barrier=0;
		wglQueryMaxSwapGroupsNV(hDC,&group,&barrier);
		printf("Max Swap Group: %d (%d)\n", group, barrier );
		BOOL bound = wglJoinSwapGroupNV(hDC,1);
		if(bound)
		{
			printf("Successfully joined swap group %u\n", 1);
		}

		BOOL boundBarrier = wglBindSwapBarrierNV(1,1);
		if(boundBarrier)
		{
			printf("Successfully bound to barrier %u\n", 1);
		}

		if(fionaConf.appType == FionaConfig::CAVE3_WIN8)
		{
			wglResetFrameCountNV(hDC);
		}
	}
	
	int vidMemSize = __FionaUTGetVideoMemorySizeBytes(true, 0);
	int dedicatedSize = __FionaUTGetVideoMemorySizeBytes(false, GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX);
	printf("Total Video memory: %d MB\n", vidMemSize/1024);
	printf("Dedicated Video memory: %d MB\n", dedicatedSize/1024);

	const GLubyte *glVersionString = glGetString(GL_VERSION);
	int glVersion[2] = {0, 0};
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	printf("Fiona is Using OpenGL version: %d.%d!\n", glVersion[0], glVersion[1]);

	//wglMakeCurrent(NULL, NULL);
	_FionaUTCreated		(hwnd);
}

DWORD WINAPI __FionaRenderWallThreadFunc(LPVOID dataPtr)
{
	bool inited = false;

	LARGE_INTEGER last, cur;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	float step = freq.QuadPart/(fionaConf.framerate+1);

	while(!fionaDone)
	{
		QueryPerformanceCounter(&cur);

		if( !inited || cur.QuadPart-last.QuadPart>=step )
		{
			last = cur;
			if(!inited)
			{
				printf("CREATING second window!!\n");

				if (fionaConf.appType == FionaConfig::CAVE1_DUALPIPE || fionaConf.appType == FionaConfig::CAVE2_DUALPIPE || fionaConf.appType == FionaConfig::CAVE3_DUALPIPE || 
					fionaConf.appType == FionaConfig::CAVE4_DUALPIPE || fionaConf.appType == FionaConfig::CAVE5_DUALPIPE || fionaConf.appType == FionaConfig::CAVE6_DUALPIPE)
				{
					printf("Making 2nd Window!!!\n");
					fionaWinConf[1].winw=1920;
					fionaWinConf[1].winh=1920;
					HWND hWnd2 = CreateWindow(_CLASSNAME, "fiona2", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 1920, 0, 1920, 1920, NULL, NULL, GetModuleHandle(NULL), NULL);
					fionaWinConf[1].window = hWnd2;
					//UpdateWindow(hWnd);
					//we want a graphics thread that draws to the 2nd window only..
					//fionaConf.graphicsThread = true;
				}
					
				inited =true;
				__FionaUTCreateWindowCB(fionaWinConf[1].window);
				ShowWindow(fionaWinConf[1].window, SW_SHOWNORMAL);
				UpdateWindow(fionaWinConf[1].window);
				SetForegroundWindow(fionaWinConf[1].window);
				wglMakeCurrent(info[1].hdc, info[1].hrc);
			}

			/*fionaRenderCycleCount=0;
			if( fionaConf.frameFunc ) 
			{
				fionaConf.frameFunc(FionaUTTime());
			}*/

			if(fionaWinConf[1].window!=NULL)
			{
				WIN win = fionaWinConf[1].window;
			
				glClearColor(0.f, 1.f, 0.f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				//_FionaUTDisplay(win,__FionaUTGetContext(win));

				//glFinish();

				//printf("Finished 2nd window frame\n");
			}
					
			/*if(fionaConf.postFunc)
			{
				fionaConf.postFunc();
			}*/

			// Swap the frame buffers
			//__FionaUTSwapBuffer(fionaWinConf[1].window);
			//wglMakeCurrent(0,0);
			//__FionaUTMakeCurrent(win, false);
			SwapBuffers(info[1].hdc);
		}
	}

	return 0;
}

DWORD WINAPI __FionaRenderThreadFunc(LPVOID dataPtr)
{
	if(fionaConf.graphicsThread)
	{
		bool inited = false;

		LARGE_INTEGER last, cur;
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		float step = freq.QuadPart/(fionaConf.framerate+1);

		if(!inited)
		{
			HWND hWnd = fionaWinConf[fionaCurWindow].window;
			__FionaUTCreateWindowCB(hWnd);
			UpdateWindow(hWnd);
			SetForegroundWindow(hWnd);
		}

		//we should wait here until some sort of 'go' signal comes across to all nodes...

		while(!fionaDone)
		{
			QueryPerformanceCounter(&cur);

			//if( fionaNetSlave==NULL )
			//{
				if( !inited || cur.QuadPart-last.QuadPart>=step )
				{
					last = cur;
					inited =true;
					fionaRenderCycleCount=0;
					if( fionaConf.frameFunc ) 
					{
						fionaConf.frameFunc(FionaUTTime());
					}

					bool bDraw = true;
					if(fionaConf.appType == FionaConfig::HEADNODE)
					{
						bDraw = fionaConf.renderHeadNode;
					}

					for(int i=0;i<MAX_WINDOW;i++) 
					{
						if(fionaWinConf[i].window!=NULL)
						{
							WIN win = fionaWinConf[i].window;
			
							__FionaUTMakeCurrent(win);
			
							if(bDraw)
							{
								_FionaUTDisplay(win,__FionaUTGetContext(win));
							}
							else
							{
								glClearColor(fionaConf.backgroundColor.x,
								 fionaConf.backgroundColor.y,
								 fionaConf.backgroundColor.z,
								 0);

								glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);//|GL_STENCIL_BUFFER_BIT);
							}
							//glFinish();
						}
					}

					if(fionaConf.postFunc)
					{
						fionaConf.postFunc();
					}
	
					/*if(fionaConf.appType == FionaConfig::CAVE3)
					{
						GLuint fc = 0;
						wglQueryFrameCountNV(info[0].hdc, &fc);
						printf("frame count: %u\n", fc);
					}*/

					// Swap the frame buffers
					for(int i=0;i<MAX_WINDOW;i++) 
					{
						if(fionaWinConf[i].window!=NULL)
						{
							__FionaUTSwapBuffer(fionaWinConf[i].window);
						}
					}
					//_FionaUTFrame();
				}
			//}
		}

		FionaUTCleanupGraphics();

		fionaRenderDone = true;
	}

	return 0;
}

void  __FionaUTInitNative		(void)
{
	for(int i=0;i<MAX_WINDOW;i++)
	{
		info[i].hwnd=NULL;
		info[i].hdc=NULL;
		info[i].hrc=NULL;
		info[i].hrc2=NULL;
		info[i].hdc2=NULL;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= /*CS_HREDRAW | CS_VREDRAW |*/ CS_OWNDC;
	wcex.lpfnWndProc	= __FionaUTWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(hInstance, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= _CLASSNAME;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, IDI_APPLICATION);

	int ret = RegisterClassEx(&wcex);
	if( ret == 0 ) printf("Register class failed\n");

	//start input thread...
	//DWORD threadId;
	//CreateThread(NULL, 0, __FionaInputThreadFunc, 0, 0, &threadId);
}

int __FionaUTFindWindow(HWND w)
{
	for(int i=0;i<MAX_WINDOW;i++)
	{
		if(info[i].hwnd==w)
		{
			return i;
		}
	}
	return -1;
}

// MULTICAST
// typedefs for the extension functions
typedef void (GLAPIENTRY * PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC) (GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid *data);
typedef void (GLAPIENTRY * PFNGLLGPUCOPYIMAGESUBDATANVXPROC) (GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLuint srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLuint dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);
typedef void (GLAPIENTRY * PFNGLLGPUINTERLOCKNVXPROC) (void);

// function pointers for the extension functions
PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC glLGPUNamedBufferSubDataNVX(nullptr);
PFNGLLGPUCOPYIMAGESUBDATANVXPROC glLGPUCopyImageSubDataNVX(nullptr);
PFNGLLGPUINTERLOCKNVXPROC glLGPUInterlockNVX(nullptr);

// defines for the extension 
#ifndef LGPU_SEPARATE_STORAGE_BIT_NVX
#define LGPU_SEPARATE_STORAGE_BIT_NVX 0x0800
#endif
#ifndef GL_MAX_LGPU_GPUS_NVX
#define GL_MAX_LGPU_GPUS_NVX          0x92BA
#endif

// We need Native Windowing Function as follows
HWND __FionaUTCreateWindow		(const char* name,int x,int y,int w,int h,int mode)
{
	/*HWND hWnd;

	// mode == -1 is for fullscreen mode with user-specified position and size
	// mode > 0 fullscreen at a screen (mode-1)
	// mode ==0 windowed mode
	fionaWinConf[fionaCurWindow].winw=w;
	fionaWinConf[fionaCurWindow].winh=h;
	if( mode!=0 )
	{
		hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
							x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
		// NOTE: if mode>0, I want to make a window filling mode-1 th screen
		if( mode>0 ) __FionaUTMakeFillScreen(hWnd,mode-1);
	}
	else
	{
		//   but it is a little tidious in Win32..
		hWnd = CreateWindow(_CLASSNAME, name, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
							x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
	}

	ShowWindow(hWnd,SW_SHOWNORMAL);

	__FionaUTCreateWindowCB(hWnd);
	UpdateWindow(hWnd);
	
	SetWindowPos(hWnd, HWND_TOPMOST, x, y, w,h,SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	SetForegroundWindow(hWnd);

	return hWnd;*/

	//this is older code, throwing this back in to see if it is better than above..
	HWND hWnd;
	if( mode==-1 )
	{
		if(fionaConf.useSecondViewerNodes || (fionaConf.twoWindows && _FionaUTIsCAVEMachine()))
		{
			if (fionaConf.appType == FionaConfig::CAVE1_DUALPIPE || fionaConf.appType == FionaConfig::CAVE2_DUALPIPE || fionaConf.appType == FionaConfig::CAVE3_DUALPIPE ||
				fionaConf.appType == FionaConfig::CAVE4_DUALPIPE || fionaConf.appType == FionaConfig::CAVE5_DUALPIPE || fionaConf.appType == FionaConfig::CAVE6_DUALPIPE) {
				
				fionaWinConf[fionaCurWindow].winw = 1920;
				fionaWinConf[fionaCurWindow].winh = 1920;
				hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1920, 1920, NULL, NULL, GetModuleHandle(NULL), NULL);
			}
			else
			{
				fionaWinConf[fionaCurWindow].winw = 1920 * 2;
				fionaWinConf[fionaCurWindow].winh = 1016;
				if (fionaConf.appType == FionaConfig::CAVE1 || fionaConf.appType == FionaConfig::CAVE2 || fionaConf.appType == FionaConfig::CAVE3)
				{
					hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1920 * 2, 1016, NULL, NULL, GetModuleHandle(NULL), NULL);
				}
				else if (fionaConf.appType == FionaConfig::CAVE4 || fionaConf.appType == FionaConfig::CAVE5 || fionaConf.appType == FionaConfig::CAVE6)
				{
					hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 3840, 0, 1920 * 2, 1016, NULL, NULL, GetModuleHandle(NULL), NULL);
				}
			}

			/*if(fionaConf.twoWindows)
			{
				if(fionaConf.appType == FionaConfig::CAVE1 || fionaConf.appType == FionaConfig::CAVE2 ||fionaConf.appType == FionaConfig::CAVE3)
				{
					printf("Making 2nd Window!!!\n");
					fionaWinConf[1].winw=1920*2;
					fionaWinConf[1].winh=1016;
					HWND hWnd2 = CreateWindow(_CLASSNAME, "fiona2", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 3840, 0, 1920*2, 1016, NULL, NULL, GetModuleHandle(NULL), NULL);
					fionaWinConf[1].window = hWnd2;
					//UpdateWindow(hWnd);
					//we want a graphics thread that draws to the 2nd window only..
					//fionaConf.graphicsThread = true;
				}
			}*/
		}
		else if (fionaConf.appType == FionaConfig::CAVE1_DUALPIPE || fionaConf.appType == FionaConfig::CAVE2_DUALPIPE || fionaConf.appType == FionaConfig::CAVE3_DUALPIPE ||
			fionaConf.appType == FionaConfig::CAVE4_DUALPIPE || fionaConf.appType == FionaConfig::CAVE5_DUALPIPE || fionaConf.appType == FionaConfig::CAVE6_DUALPIPE) {
			//fionaWinConf[fionaCurWindow].winw = 1920 * 2;
			//fionaWinConf[fionaCurWindow].winh = 1920;
			hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, fionaWinConf[fionaCurWindow].winx, fionaWinConf[fionaCurWindow].winy, fionaWinConf[fionaCurWindow].winw, fionaWinConf[fionaCurWindow].winh, NULL, NULL, GetModuleHandle(NULL), NULL);
		}
		else
		{
			fionaWinConf[fionaCurWindow].winw=1920*4;
			fionaWinConf[fionaCurWindow].winh=1016;
			hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
								0, 0, 1920*4, 1016, NULL, NULL, GetModuleHandle(NULL), NULL);
		}
	}
	else
	{
		// NOTE: if mode>0, I want to make a window filling mode-1 th screen
		//   but it is a little tidious in Win32..
		if (fionaConf.borderlessWindow)
		{
				hWnd = CreateWindow(_CLASSNAME, name, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
					x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
		}
		else
		{
			if (fionaConf.appType == FionaConfig::HEADNODE)
			{
				hWnd = CreateWindow(_CLASSNAME, name, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
					0, 0, 1280, 800, NULL, NULL, GetModuleHandle(NULL), NULL);
			}
			else
			{
				hWnd = CreateWindow(_CLASSNAME, name, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
					x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
			}
		}
	}

	if (mode > 0)
	{
#ifndef ENABLE_VIVE	//vive has the headset mirror..
		__FionaUTMakeFillScreen(hWnd, mode - 1);
#endif
	}
	else
	{
		RECT rcClient, rcWind;
		POINT ptDiff;
		GetClientRect(hWnd, &rcClient);
		GetWindowRect(hWnd, &rcWind);
		ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
		ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
		//MoveWindow(hWnd, rcWind.left, rcWind.top, w + ptDiff.x, h + ptDiff.y, TRUE);
		fionaWinConf[0].winw = w - ptDiff.x;
		fionaWinConf[0].winh = h - ptDiff.y;
		printf("Window: %d %d %d %d\n", fionaWinConf[0].winw, fionaWinConf[0].winh, ptDiff.x, ptDiff.y);
		if (fionaConf.fboSameAsWindow && !fionaConf.useFBO)
		{
			fionaConf.FBOWidth = fionaWinConf[0].winw;// +ptDiff.x;
			fionaConf.FBOHeight = fionaWinConf[0].winh;// +ptDiff.y;
			printf("FBO: %d %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
		}
		ShowWindow(hWnd, SW_SHOWNORMAL);
/*#ifndef ENABLE_VIVE
		if (fionaConf.fboSameAsWindow)
		{
			fionaConf.FBOWidth = fionaWinConf[0].winw;
			fionaConf.FBOHeight = fionaWinConf[0].winh;
		}
		ShowWindow(hWnd, SW_SHOWNORMAL);
#endif*/
	}

	if(!fionaConf.graphicsThread)
	{
		__FionaUTCreateWindowCB(hWnd);
		
		UpdateWindow(hWnd);
		//::SetWindowPos(hWnd, HWND_TOPMOST, x, y, w,h,SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
		//::SetActiveWindow(hWnd);
		SetForegroundWindow(hWnd);
		printf("Bringing window to front\n");
		BringWindowToTop(hWnd);
		//FreeConsole();
		//ShowCursor(0);
	}
	else
	{
		printf("Using separate graphics thread...\n");
		DWORD renderthreadId;
		CreateThread(NULL, 0, __FionaRenderThreadFunc, 0, 0, &renderthreadId);
	}

	if(fionaConf.twoWindows && _FionaUTIsCAVEMachine())
	{
		//ShowWindow(fionaWinConf[1].window,SW_SHOWNORMAL);
		//__FionaUTCreateWindowCB(fionaWinConf[1].window);

		//DWORD renderthreadId;
		//CreateThread(NULL, 0, __FionaRenderWallThreadFunc, 0, 0, &renderthreadId);
	}

	/*if (fionaConf.appType != FionaConfig::OCULUS)
	{
#ifndef ENABLE_VIVE
		if (fionaConf.fboSameAsWindow || fionaConf.appType == FionaConfig::HEADNODE)
		{
			fionaConf.FBOWidth = fionaWinConf[0].winw;
			fionaConf.FBOHeight = fionaWinConf[0].winh;
		}
#endif
	}*/

	//printf("%d %d*******\n", fionaConf.FBOWidth, fionaConf.FBOHeight);

	/*printf("Testing for multi-cast extension...\n");

	// MULTICAST
	// look for the extension in the extensions list
	GLint numExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	bool found = false;
	for (GLint i = 0; i < numExtensions && !found; ++i)
	{
		std::string name((const char*)glGetStringi(GL_EXTENSIONS, i));
		if (name == "GL_NVX_linked_gpu_multicast")
		{
			std::cout << "Extension " << name << " found!\n";
			found = true;
		}
	}

	if (!found)
	{
		printf("Multicast extension not found, aborting!\n\n");
	}

	// get pointers to the extension functions
	glLGPUNamedBufferSubDataNVX = (PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC)wglGetProcAddress("glLGPUNamedBufferSubDataNVX");
	glLGPUCopyImageSubDataNVX = (PFNGLLGPUCOPYIMAGESUBDATANVXPROC)wglGetProcAddress("glLGPUCopyImageSubDataNVX");
	glLGPUInterlockNVX = (PFNGLLGPUINTERLOCKNVXPROC)wglGetProcAddress("glLGPUInterlockNVX");

	if (glLGPUNamedBufferSubDataNVX == nullptr
		|| glLGPUCopyImageSubDataNVX == nullptr
		|| glLGPUInterlockNVX == nullptr)
	{
		printf("\n\nGL_NVX_linked_gpu_multicast not supported, aborting!\n\n");
	}

	// query the number of GPUs available in this system
	GLint numGPUs = 0;
	glGetIntegerv(GL_MAX_LGPU_GPUS_NVX, &numGPUs);
	std::cout << "GPUs found: " << numGPUs << "\n";*/

	return hWnd;
}


void  __FionaUTMakeFillScreen	(WIN hwnd,int)
{
	printf("IN FILL SCREEN\n");
	SetWindowLong(hwnd,GWL_STYLE,WS_POPUP);
	ShowWindow(hwnd,SW_SHOWMAXIMIZED);
}

void  __FionaUTMakeWideScreen	(WIN hwnd)
{
	SetWindowLong(hwnd,GWL_STYLE,WS_POPUP);
	ShowWindow(hwnd,SW_SHOWMAXIMIZED);
	MoveWindow(hwnd,0,0,1920*2,1016,TRUE);
}

void  __FionaUTSwapBuffer		(WIN win)
{
	int i = __FionaUTFindWindow(win); 
	if(i<0) 
	{
		return;
	}
	
	if(fionaConf.multiGPU)
	{
		SwapBuffers(info[i].hdc);
	}
	else
	{
		//3/10/2015 - re-add below two lines if ever need multi-gpus again..
		//wglMakeCurrent(0,0);
		//__FionaUTMakeCurrent(win, false);
		SwapBuffers(info[i].hdc);
	}
}

HDC	__FionaUTGetDeviceContext (WIN win, bool secondGPU)
{
	int i=__FionaUTFindWindow(win); 
	
	if(i<0) 
	{
		return 0;
	}

	if(secondGPU)
	{
		return info[i].hdc2;
	}

	return info[i].hdc;
}

CTX __FionaUTGetContext(WIN win, bool secondGPU)
{
	int i=__FionaUTFindWindow(win); 
	
	if(i<0) 
	{
		return FIONA_INVALID_CTX;
	}
	
	if(secondGPU)
	{
		return info[i].hrc2;
	}

	return info[i].hrc;
}

void  __FionaUTMainLoop(int _framerate)
{
	if( _framerate>=1 ) 
	{	
		fionaConf.framerate = _framerate;
	}

	MSG msg;
	//LARGE_INTEGER last, cur;
	//LARGE_INTEGER freq;
	//QueryPerformanceFrequency(&freq);
	//float step = freq.QuadPart/(fionaConf.framerate+1);

	bool inited = false;

	while(!fionaDone)// && !fionaRenderDone)
	{
		if(scene && !scene->isRunning())
		{
			break;
		}

		//Sleep(0);
		if( PeekMessage(&msg,NULL,0,0,1) )
		{
			if(msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_FionaUTIdle();
		
		//QueryPerformanceCounter(&cur);

		if( fionaNetSlave==NULL )
		{
			//if( !inited || cur.QuadPart-last.QuadPart>=step )
			//{
			//	last = cur;
			//	inited =true;
				_FionaUTFrame();
			//}
		}

		/*if(fionaConf.appType == FionaConfig::CAVE3)
		{
			GLuint fc = 0;
			wglQueryFrameCountNV(info[0].hdc, &fc);
			printf("frame count: %u\n", fc);
		}*/
	}
	
	FionaUTExit(0);
}

void  __FionaUTCloseWindow		(void)
{
	for(int i=0;i<MAX_WINDOW;i++) 
	{
		if(fionaWinConf[i].window!=NULL)
		{
			WIN win = fionaWinConf[i].window;
			//__FionaUTMakeCurrent(win);
			if(fionaConf.multiGPU || fionaConf.useSecondGPU)
			{
				if(_FionaUTIsCAVEMachine() || info[i].hrc2 != 0)
				{
					wglDeleteContext(info[i].hrc2);
					wglDeleteDCNV(info[i].hdc2);
				}
				//ReleaseDC(info[i].hwnd, info[i].hdc2);
			}
			wglDeleteContext(info[i].hrc);
			ReleaseDC(info[i].hwnd, info[i].hdc);
			DestroyWindow(info[i].hwnd);
		}
	}
}

BOOL __FionaUTSetContext(WIN win, bool secondContext)
{
	int i=__FionaUTFindWindow(win); 
	
	if(i<0) 
	{
		return FALSE;
	}

	BOOL ret = FALSE;
	//ret = wglMakeCurrent(0,0);
	//if(ret == FALSE)
	//{
	//	printf("Failed to not make anything current...\n");
	//}
	//ret = FALSE;
	if(secondContext)
	{
		ret = wglMakeCurrent(info[i].hdc, info[i].hrc2);
		if(ret == FALSE)
		{
			printf("Failed to set second context\n");
		}
	}
	else
	{
		ret = wglMakeCurrent(info[i].hdc, info[i].hrc);
		if(ret == FALSE)
		{
			printf("Failed to set first context\n");
		}
	}

	return ret;
}

void  __FionaUTMakeCurrent		(WIN win, bool secondGPU)
{
	int i=__FionaUTFindWindow(win); 
	
	if(i<0) 
	{
		return;
	}

	if(!secondGPU)
	{
		if(fionaConf.multiGPU)
		{
			if(_FionaUTIsCAVEMachine())
			{
				wglMakeCurrent(info[i].hdc2, 0);
			}
		}

		BOOL mc = wglMakeCurrent(info[i].hdc,info[i].hrc);
		fionaActiveContext = info[i].hrc;
		if(mc == FALSE)
		{
			printf("FAILED SETTING 1st CONTEXT\n");
		}
	}
	else
	{
		if(fionaConf.multiGPU)
		{
			if(_FionaUTIsCAVEMachine())
			{
				wglMakeCurrent(info[i].hdc, 0);
			}
		}

		fionaActiveContext = info[i].hrc2;
		
		BOOL mc = wglMakeCurrent(info[i].hdc2,info[i].hrc2);
		if(mc == FALSE)
		{
			printf("FAILED SETTING 2ND CONTEXT\n");
		}
	}
}

int __FionaUTGetW(WIN win)
{
	RECT rect; 
	GetWindowRect(win, &rect);
	//GetClientRect(win,&rect); 
	return rect.right-rect.left;
}

int __FionaUTGetH(WIN win)
{
	RECT rect; 
	GetWindowRect(win, &rect);
	//GetClientRect(win,&rect); 
	return rect.bottom-rect.top;
}

static bool __fiona__clockInited=false;
static LARGE_INTEGER __fiona__first_time;
static LARGE_INTEGER __fiona__frequency;
float FionaUTTime(void) {
	if( !__fiona__clockInited )
	{
		QueryPerformanceCounter(&__fiona__first_time);
		QueryPerformanceFrequency(&__fiona__frequency);
		__fiona__clockInited=true;
	}
	LARGE_INTEGER cur;
	QueryPerformanceCounter(&cur);
	LONGLONG diff = cur.QuadPart - __fiona__first_time.QuadPart;
	return diff/(float)(__fiona__frequency.QuadPart);
}

void FionaUTSleep(float t)
{
	Sleep(t*1000);
}

LRESULT	CALLBACK __FionaUTWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	bool simulate = false;
	switch (message)
	{
	case WM_CREATE:
//		__FionaUTCreateWindowCB(hWnd);
		return 0;
	case WM_SIZE:
		_FionaUTReshape(hWnd,LOWORD(lParam),HIWORD(lParam));
		return 0;
	case WM_PAINT:
		BeginPaint(hWnd,&ps);
		EndPaint(hWnd,&ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		_FionaUTMouseDown(hWnd, GLUT_LEFT_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(0, 1);
		}
		return 0;
	case WM_LBUTTONUP:
		_FionaUTMouseUp  (hWnd, GLUT_LEFT_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(0, 0);
		}
		return 0;
	case WM_MBUTTONDOWN:
		_FionaUTMouseDown(hWnd, GLUT_MIDDLE_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(2, 1);
		}
		return 0;
	case WM_MBUTTONUP:
		_FionaUTMouseUp  (hWnd, GLUT_MIDDLE_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(2, 0);
		}
		return 0;
	case WM_RBUTTONDOWN:
		_FionaUTMouseDown(hWnd, GLUT_RIGHT_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(1, 1);
		}
		return 0;
	case WM_RBUTTONUP:
		_FionaUTMouseUp  (hWnd, GLUT_RIGHT_BUTTON, LOWORD(lParam),HIWORD(lParam) );
		if(simulate)
		{
			_FionaUTWandButton(1, 0);
		}
		return 0;
	case WM_MOUSEMOVE:
		if( wParam& MK_LBUTTON )
			_FionaUTMouseDrag(hWnd, LOWORD(lParam),HIWORD(lParam) );
		else
			_FionaUTMouseMove(hWnd, LOWORD(lParam),HIWORD(lParam) );
		return 0;
/*	case WM_MOUSEHWHEEL:
		app = findApp(hWnd);
		if( app==NULL ) return DefWindowProc(hWnd, message, wParam, lParam);
		app->mouseHWheel(hWnd, GET_WHEEL_DELTA_WPARAM(wParam),wParam);
		break;
	case WM_MOUSEWHEEL:
		app = findApp(hWnd);
		if( app==NULL ) return DefWindowProc(hWnd, message, wParam, lParam);
		app->mouseWheel(hWnd, GET_WHEEL_DELTA_WPARAM(wParam),wParam);
		break;
		//ROSS TODO - Better input system here for testing...
*/	case WM_KEYDOWN:

		if( (lParam& (1 << 30) )==0 )
		{
			if(wParam == VK_END)
			{
				_FionaUTWandButton(2, 1);
			}
			if(wParam == VK_PRIOR)
			{
				_FionaUTWandButton(1, 1);
			}
			if(wParam == VK_HOME)
			{
				_FionaUTWandButton(0, 1);
			}
			if(wParam == VK_NEXT)
			{
				_FionaUTWandButton(3, 1);
			}
			if(wParam == VK_SPACE)
			{
				_FionaUTWandButton(5, 1);
			}
			if(wParam == VK_TAB)
			{
				_FionaUTWandButton(4, 1);
			}

			switch(wParam)
			{
			case VK_UP    : _FionaUTJoystick(0, jvec3( 0,0, -fionaConf.navigationSpeed)); break;  // up arrow
				case VK_DOWN  : _FionaUTJoystick(0, jvec3( 0,0, fionaConf.navigationSpeed)); break;  // down arrow
				case VK_LEFT  : _FionaUTJoystick(0, jvec3(-fionaConf.navigationSpeed,0, 0)); break;  // left arrow
				case VK_RIGHT : _FionaUTJoystick(0, jvec3( fionaConf.navigationSpeed,0, 0)); break;  // right arrow
				case VK_INSERT : _FionaUTJoystick(0, jvec3(0, fionaConf.navigationSpeed, 0)); break;
				case VK_DELETE : _FionaUTJoystick(0, jvec3(0, -fionaConf.navigationSpeed, 0)); break;
			}
			_FionaUTKeyboard(hWnd,wParam);
		}
		return 0;
		
	case WM_KEYUP:
		{
			if(wParam == VK_END)
			{
				_FionaUTWandButton(2, 0);
			}
			if(wParam == VK_PRIOR)
			{
				_FionaUTWandButton(1, 0);
			}
			if(wParam == VK_HOME)
			{
				_FionaUTWandButton(0, 0);
			}
			if(wParam == VK_NEXT)
			{
				_FionaUTWandButton(3, 0);
			}
			if(wParam == VK_SPACE)
			{
				_FionaUTWandButton(5, 0);
			}
			
			if(wParam == VK_TAB)
			{
				_FionaUTWandButton(4, 0);
			}

			switch(wParam)
			{
				case VK_UP    : _FionaUTJoystick(0, jvec3( 0, 0, 0)); break;  // up arrow
				case VK_DOWN  : _FionaUTJoystick(0, jvec3( 0, 0, 0)); break;  // down arrow
				case VK_LEFT  : _FionaUTJoystick(0, jvec3( 0, 0, 0)); break;  // left arrow
				case VK_RIGHT : _FionaUTJoystick(0, jvec3( 0, 0, 0)); break;  // right arrow
				case VK_INSERT : _FionaUTJoystick(0, jvec3(0, 0, 0)); break;
				case VK_DELETE : _FionaUTJoystick(0, jvec3(0, 0, 0)); break;
			}
			return 0;
		}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}