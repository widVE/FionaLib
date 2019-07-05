//Kevin Ponto
// 8/3/2013
// Linux Fiona windowing stuff

#include <stdlib.h>
#include "FionaUT.h"
#include <sys/time.h>

#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glxext.h>
#include <X11/X.h>
#include <X11/extensions/xf86vmode.h>
#include <unistd.h>


#define TITLE "OpenGL in X11"

#define DEBUGPRINT

static bool GLEWInited = false;
static bool GLUTMainLoopInited = false;

static bool PrintFPS = false;

/* original desktop mode which we save so we can restore it later */   
XF86VidModeModeInfo     desktopMode; 


//*****************************************************************
//
//From http://www.opengl.org/sdk/docs/man2/xhtml/glXIntro.xml
//
//*****************************************************************
int singleBufferAttributess[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE,      1,   /* Request a single buffered color buffer */
    GLX_GREEN_SIZE,    1,   /* with the maximum number of color bits  */
    GLX_BLUE_SIZE,     1,   /* for each component                     */
    None
};

int doubleBufferAttributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER,  True,  /* Request a double-buffered color buffer with */
    GLX_RED_SIZE,      1,     /* the maximum number of bits per component    */
    GLX_GREEN_SIZE,    1, 
    GLX_BLUE_SIZE,     1,
    None
};
static Bool WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
    return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}

//*****************************************************************
void  __FionaUTInitNative		(void)
{
//not sure what to do here?
//try calling glutInit
	DEBUGPRINT("init\n");
	printf("in init window 0: %p\n", fionaWinConf[0].window);
 	
}

void  __FionaUTMakeFillScreen	(void *_win,int)
{
	DEBUGPRINT("full screen\n");
	//not sure what to do here
	// glutFullScreen();
}

int __FionaUTGetW(void* _win)
{
	FionaUTWin* win = (FionaUTWin *)_win;
	return win->w;
}
int __FionaUTGetH(void* _win)
{
	FionaUTWin* win = (FionaUTWin *)_win;
	return win->h;
}

Display *	__FionaUTGetDeviceContext (WIN win, bool secondGPU)
{
	FionaUTWin* w = (FionaUTWin*)fionaWinConf[0].window;
	return w->dsp;	
}

void HandleEvents()
{
	//assume the events come from window 0
	FionaUTWin* win = (FionaUTWin*)fionaWinConf[0].window;
	_FionaUTJoystick(0, jvec3( 0,0, 0));
	while (XPending(win->dsp) > 0)        
	{ 
	
		XEvent event;
		XNextEvent(win->dsp,&event);
		if(XPending(win->dsp)>=0)
		{
		int button;
		switch(event.xbutton.button){
			case Button1:
				button = GLUT_LEFT_BUTTON;
			break;
			case Button3:
				button = GLUT_RIGHT_BUTTON;
			break;
			default:
				button = GLUT_LEFT_BUTTON;
			break;
		}

		switch (event.type) {
			//mouse event
			case ButtonPress:
			{
				
				_FionaUTMouseUp(win, button, event.xbutton.x, event.xbutton.y );
				break;
			}
			case ButtonRelease:
			{
				_FionaUTMouseDown(win, button, event.xbutton.x, event.xbutton.y);
				break;
			}
			
		      case KeyPress : 
			{
		
				int keycode = event.xkey.keycode;
				printf("Key Pressed %d\n", keycode); 
				switch( keycode )
				{
					case 111: _FionaUTJoystick(0, jvec3( 0,0, fionaConf.navigationSpeed)); break;  // up arrow
					case 116: _FionaUTJoystick(0, jvec3( 0,0,-fionaConf.navigationSpeed)); break;  // down arrow
					case 113: _FionaUTJoystick(0, jvec3(-fionaConf.navigationSpeed,0, 0)); break;  // left arrow
					case 114: _FionaUTJoystick(0, jvec3( fionaConf.navigationSpeed,0, 0)); break;  // right arrow
					case 65: _FionaUTJoystick(0, jvec3( 0,0, 0)); break;  // right arrow
					case 119: _FionaUTJoystick(0, jvec3(0.f,-fionaConf.navigationSpeed,0.f)); break; //delete key
					case 118: _FionaUTJoystick(0, jvec3(0.f, fionaConf.navigationSpeed, 0.f)); break; //insert key
					case 9: FionaUTExit(); break;  // exit
				}
				//make it ascii
				char buf[2];
		                int len;
		                KeySym keysym_return;
		                len = XLookupString(&event.xkey, buf, 1, &keysym_return, NULL);
		               
		                if ( len == 0 ){
		                        buf[0] = '\0';
		                }
				
				//add in wand button spoof
				switch( buf[0] )
				{
					case '`': _FionaUTWandButton(5, 0); break;
					case '1': _FionaUTWandButton(1, 0); break;
					case '2': _FionaUTWandButton(0, 0); break;
					case '3': _FionaUTWandButton(2, 0); break;
					case '4': _FionaUTWandButton(3, 0); break;
				}
				_FionaUTKeyboard(win,buf[0]);
				break;
			}
		      case KeyRelease :
			{
				int keycode = event.xkey.keycode;
				/*
				//is there another event?
				if (XPending(win->dsp) > 0)
				{
					XEvent nextEvent;
					XPeekEvent(win->dsp,&nextEvent);
					int nextKeycode = nextEvent.xkey.keycode;
			
					//this is auto-repeat
					if ((keycode == nextKeycode) && (nextEvent.type == KeyPress))
					{		
						printf("Key Held %d\n", keycode); 
						break;
					}
				}
				*/
				//printf("Key Released %d\n", keycode); 
				switch( keycode )
				{
					case 111: _FionaUTJoystick(0, jvec3( 0,0, 0)); break;  // up arrow
					case 116: _FionaUTJoystick(0, jvec3( 0,0, 0)); break;  // down arrow
					case 113: _FionaUTJoystick(0, jvec3( 0,0, 0)); break;  // left arrow
					case 114: _FionaUTJoystick(0, jvec3( 0,0, 0)); break;  // right arrow
					case 119: _FionaUTJoystick(0, jvec3(0.f,0.f,0.f)); break; //delete key
					case 118: _FionaUTJoystick(0, jvec3(0.f,0.f, 0.f)); break; //insert key
				}
			
			break;
			}
			//case Expose:
			case ConfigureNotify:            
				//printf("reshape event\n");
				/* call resizeGL only if our window-size changed */
				if ((event.xconfigure.width != __FionaUTGetW(win)) ||
				    (event.xconfigure.height != __FionaUTGetH(win)))
				{
				    int width = event.xconfigure.width;
				    int height = event.xconfigure.height;
				    _FionaUTReshape(win, width, height);
				}
                        break;
		      case EnterNotify : DEBUGPRINT("Enter\n"); break;
		  }
		}
	}
}


void  __FionaUTMainLoop			(int _framerate)
{
	if( _framerate>=1 ) fionaConf.framerate = _framerate;
	
	//MSG msg;
	long int last, cur;
	long int freq;
	//QueryPerformanceFrequency(&freq);
	//float step = freq.QuadPart/(fionaConf.framerate+1);
	float fr = fionaConf.framerate;
	if(fr == 0.f)
	{
		fr = 60.f;
	}

	float step  = 1.0 / fr;

	bool inited = false;

	float startTime = FionaUTTime();

	int numFrameCounter=0;
	float prevCounterFrameTime=startTime;	

	while(1)
	//for (int i=0; i < 100000; i++)
	{
		/*
		if( PeekMessage(&msg,NULL,0,0,1) )
		{
			if(msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}*/
		//FionaUTSleep(10);

		
		
		//get the events
		HandleEvents();


		_FionaUTIdle();
	
		float curTime = FionaUTTime();
		//printf("main loop window 0: %p\n", fionaWinConf[0].window);
		//QueryPerformanceCounter(&cur);
		if( fionaNetSlave==NULL )
		//if( !inited || cur.QuadPart-last.QuadPart>=step )
		if (!inited || curTime-startTime > step)
		{
			startTime = curTime;
			inited =true;
			DEBUGPRINT("head: %f %f %f\n", fionaConf.headPos.x,fionaConf.headPos.y,fionaConf.headPos.z) ;
			//DEBUGPRINT("rot: %f %f %f\n", fionaConf.headRot.yaw()*(180/3.14),fionaConf.headRot.pitch()*(180/3.14), fionaConf.headRot.roll()*(180/3.14)) ;			
			_FionaUTFrame();
			//printf("post-FionaUTFrame window 0: %p\n", fionaWinConf[0].window);
			
			if (PrintFPS)
			{
				numFrameCounter++;
				float dTime = curTime - prevCounterFrameTime;
				if (dTime > 5)
				{				
					printf("frame at rate %f\n", numFrameCounter / dTime);
					prevCounterFrameTime = curTime;
					numFrameCounter=0;
				}
			}
		}
	}
	FionaUTExit(0);
}

float FionaUTTime(void) {
	struct timeval tv;
	float s=0;
	if(!gettimeofday(&tv, NULL)) s=tv.tv_usec/1000000.0f+tv.tv_sec%1000;
	return s;
}
void FionaUTSleep(float t)
{
	if( t>=1.0f ){ sleep((int)t); t-=(int)t; }
	usleep(t*1000000);
}

void  __FionaUTMakeCurrent(void* _win, bool secondGPU) {
	FionaUTWin* win = (FionaUTWin *)_win;
	DEBUGPRINT("make context current\n");
	glXMakeCurrent(win->dsp, win->xwin, win->cxt);
	//new version
	//glXMakeContextCurrent( win->dsp, win->glxWin, win->glxWin, win->cxt );
	//XSetInputFocus(win->dsp, win->xwin,  RevertToParent, CurrentTime);
	
}

CTX __FionaUTGetContext(void* _win, bool secondContext){
	DEBUGPRINT("get context\n");
	FionaUTWin* win = (FionaUTWin *)_win;
	DEBUGPRINT("FionaUTGetContext window: %p\n", win);
	//printf("getting context\n");
	return win->cxt;
	
}
void  __FionaUTSwapBuffer(void* _win) {
	FionaUTWin* win = (FionaUTWin *)_win;
	//SwapBuffers(win->cxt);
	DEBUGPRINT("swap buffers\n");

	glXMakeCurrent(win->dsp, win->xwin, win->cxt);
	glFlush();
 	glXSwapBuffers( win->dsp, win->xwin );	
}

//note I stole this from CGLX
typedef struct GLInfo_S{
	char vendor[128];
	char renderer[128];
	char version[32];
	char clientvendor[64];
	char clientversion[32];
} *GLInfo_P;

GLInfo_S glinfo;

bool getGLInfo(Display *dpy, int screen)
{

	Window win;
   	int attribSingle[] = {	GLX_RGBA,
      						GLX_RED_SIZE, 1,
      						GLX_GREEN_SIZE, 1,
      						GLX_BLUE_SIZE, 1,
      						None };
   	int attribDouble[] = {	GLX_RGBA,
      						GLX_RED_SIZE, 1,
      						GLX_GREEN_SIZE, 1,
      						GLX_BLUE_SIZE, 1,
      						GLX_DOUBLEBUFFER,
      						None };

   	XSetWindowAttributes attr;
   	unsigned long mask;
   	Window root;
   	GLXContext ctx;
   	XVisualInfo *visinfo;
   	int width = 100, height = 100;

   	root = RootWindow(dpy, screen);

   	visinfo = glXChooseVisual(dpy, screen, attribSingle);
   	if (!visinfo) {
      	visinfo = glXChooseVisual(dpy, screen, attribDouble);
     	if (!visinfo) {
         	fprintf(stderr, "Error: couldn't find RGB GLX visual\n");
         	return (false);
      	}
   	}

   	attr.background_pixel 	= 0;
   	attr.border_pixel 		= 0;
   	attr.colormap 			= XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   	attr.event_mask 		= StructureNotifyMask | ExposureMask;
   	mask 					= CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   	win 					= XCreateWindow(dpy, root, 0, 0, width, height,
			       							0, visinfo->depth, InputOutput,
			       							visinfo->visual, mask, &attr);

   	ctx 					= glXCreateContext( dpy, visinfo, NULL, true );
   	if (!ctx) {
     	fprintf(stderr, "Error: glXCreateContext failed\n");
      	XDestroyWindow(dpy, win);
      	return (false);
   	}
	if (glXMakeCurrent(dpy, win, ctx)) {
		// ===================
      	// general information
      	// ===================
      	strcpy(glinfo.vendor,(const char *)glGetString(GL_VENDOR));
      	strcpy(glinfo.renderer,(const char *)glGetString(GL_RENDERER));
      	strcpy(glinfo.version,(const char *)glGetString(GL_VERSION));
      	//const char *glExtensions 		= (const char *) glGetString(GL_EXTENSIONS);
      	// =======
      	// server
      	// =======
    	//const char *serverVendor 		= glXQueryServerString(dpy, screen, GLX_VENDOR);
      	//const char *serverVersion 		= glXQueryServerString(dpy, screen, GLX_VERSION);
      	//const char *serverExtensions 	= glXQueryServerString(dpy, screen, GLX_EXTENSIONS);
      	// =======
      	// client
      	// =======
      	strcpy(glinfo.clientvendor,glXGetClientString(dpy, GLX_VENDOR));
      	strcpy(glinfo.clientversion,glXGetClientString(dpy, GLX_VERSION));
		// YYY unused
      	const char *clientExtensions 	= glXGetClientString(dpy, GLX_EXTENSIONS);
		
      	//const char *glxExtensions 		= glXQueryExtensionsString(dpy, screen);

		//printf("Extentions:\n %s\n",clientExtensions);

   	}

	// delete visinfo // new valgrid
	free (visinfo);

   	glXDestroyContext(dpy, ctx);
   	XDestroyWindow(dpy, win);



	return(true);
}

void  __FionaUTCloseWindow		(void)
{
	/*for(int i=0;i<MAX_WINDOW;i++) 
	{
		if(fionaWinConf[i].window!=NULL)
		{
			WIN win = fionaWinConf[i].window;
			__FionaUTMakeCurrent(win);
			glXDestroyContext(info[i].hrc);
			//ReleaseDC(info[i].hwnd, info[i].hdc);
			XDestroyWindow(info[i].hwnd);
		}
	}*/
}

int _invalid_window_handler(Display *dsp, XErrorEvent *err) {
	printf("ERROR: INVALID WINDOW HANDLER\n");
	return 0;
}


void *__FionaUTCreateWindow(const char* name,int x,int y,int w,int h,int mode)
{
	printf("*********************CREATING WINDOW********************\n");
	DEBUGPRINT("create window %s: %d %d %d %d\n", name, x, y, w, h);
	if( !GLEWInited )
	{ 
		glewInit(); GLEWInited=true; 

	}

	
	fionaWinConf[fionaCurWindow].winw=w;
	fionaWinConf[fionaCurWindow].winh=h;
	
	//lets make a new struct
	FionaUTWin *win = new FionaUTWin(name, x, y, w, h);
	
	//im guessing here on the full screen thing from cglx
	//mode=1;
	bool fullscreen = (mode != 0);
	if (!fullscreen)
	{
		win->dsp = XOpenDisplay(getenv("DISPLAY"));
	}	
	else
	{
		win->dsp = XOpenDisplay(":0.0");
	}
	if ( win->dsp == NULL ) {
        	printf( "Unable to open a connection to the X server\n" );
        	exit( EXIT_FAILURE );
   	 }

	//second option is screen number.  not sure how to set it. Is it fionaCurWindow?
	win->screen=DefaultScreen(win->dsp);

	//do we need this?
	//root = DefaultRootWindow(win->dsp);

	/* Request a suitable framebuffer configuration - try for a double 
	** buffered configuration first */
	
	int                   numReturned;
	win->fbConfigs = glXChooseFBConfig( win->dsp, win->screen,
		                   doubleBufferAttributes, &numReturned );

	/* this would be bad...let us not go there
	if ( win->fbConfigs == NULL ) {  
		win->fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
		                     singleBufferAttributess, &numReturned );
		swapFlag = False;
	}
	*/
	   //info = XGetVisualInfo(disp, VisualNoMask, &templ, &unused);
	   //con = glXCreateContext(disp, info, NULL, true);

/*
	getGLInfo(win->dsp, win->screen);
	
	std::cout << "---------------------------------------------" << std::endl;
	std::cout << "(II) GL vendor      ....... "<< glinfo.vendor << std::endl;
	std::cout << "(II) GL renderer    ....... "<< glinfo.renderer << std::endl;
	std::cout << "(II) GL version     ....... "<< glinfo.version << std::endl;
	std::cout << "(II) GL cl. vendor  ....... "<< glinfo.clientvendor << std::endl;
	std::cout << "(II) GL cl. version ....... "<< glinfo.clientversion << std::endl;
*/
		
	//attribute list for single buffer
	GLint attrListSgl[32], attrListDbl[32];
	// the following is the X setting
	// single buffer
	int iNumAttr = 0;
	attrListSgl[iNumAttr++] = GLX_RGBA;
	attrListSgl[iNumAttr++] = GLX_RED_SIZE;
	attrListSgl[iNumAttr++] = 4;
	attrListSgl[iNumAttr++] = GLX_GREEN_SIZE;
	attrListSgl[iNumAttr++] = 4;
	attrListSgl[iNumAttr++] = GLX_BLUE_SIZE;
	attrListSgl[iNumAttr++] = 4;
	attrListSgl[iNumAttr++] = GLX_DEPTH_SIZE;
	attrListSgl[iNumAttr++] = 16;
	attrListSgl[iNumAttr++] = None; 

	// double buffer
	iNumAttr = 0;
	attrListDbl[iNumAttr++]	= GLX_RGBA; // was RGBA
	attrListDbl[iNumAttr++]	= GLX_DOUBLEBUFFER;
	attrListDbl[iNumAttr++] = GLX_RED_SIZE;
	attrListDbl[iNumAttr++] = 4;
	attrListDbl[iNumAttr++] = GLX_BLUE_SIZE;
	attrListDbl[iNumAttr++] = 4;
	attrListDbl[iNumAttr++] = GLX_DEPTH_SIZE;
	attrListDbl[iNumAttr++] = 16;
	attrListDbl[iNumAttr++] = None;

	//cglx way
	int glxMajor, glxMinor, vmMajor, vmMinor;
	glXQueryVersion(win->dsp, &glxMajor, &glxMinor);                                     
	printf("GLX-Version %d.%d\n", glxMajor, glxMinor); 


	win->vi =glXChooseVisual(win->dsp, win->screen, attrListDbl);
	//other version
	//win->vi = glXGetVisualFromFBConfig( win->dsp, win->fbConfigs[0] );

	//create a gl context?
	//	GLXContext glXCreateContext(Display *  dpy,  XVisualInfo *  vis,  GLXContext  shareList,  Bool  direct);
	//printf("create gl context\n");
	//win->cxt = glXCreateContext(win->dsp, win->vi, NULL, GL_TRUE);	
                          
	/* create a GLX context */                                                          
	win->cxt = glXCreateContext(win->dsp, win->vi, 0, GL_TRUE);  
	
	if (glXIsDirect(win->dsp, win->cxt))//&&glXIsDirect(cvi[dspID]->vis.dpy, cvi[dspID]->vis.ctx2))
		printf("direct draw is on\n");
	else
		printf("direct draw is off\n");

	printf("create color map\n");
 	win->cmap =	XCreateColormap(win->dsp, RootWindow(win->dsp,win->screen), win->vi->visual, AllocNone);

	//now some window options
	unsigned long valuemask;
	//this is probably the headnode options
	//CWOverrideRedirect for fullscreen
	valuemask=CWBorderPixel | CWColormap | CWEventMask ;
	//if (fullscreen)
	//	valuemask=CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
	//set some attributes

	win->attr.colormap			= win->cmap;
	win->attr.border_pixel 		= 0;
	win->attr.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;
	win->attr.override_redirect = True;

	if (fullscreen)
	{           
		/* switch to fullscreen */
		XF86VidModeModeInfo **modes;             
		int modeNum, bestMode=0;  
		int dpyWidth, dpyHeight;

		XF86VidModeQueryVersion(win->dsp, &vmMajor, &vmMinor);
		printf("XF86 VideoMode extension version %d.%d\n", vmMajor, vmMinor);
		XF86VidModeGetAllModeLines(win->dsp, win->screen, &modeNum, &modes);       
		/* save desktop-resolution before switching modes */                 
		//GLWin.deskMode = *modes[0];
		desktopMode = *modes[0];                                      
		/* look for mode with requested resolution */                        
		for (int i = 0; i < modeNum; i++)                                        
		{                                                                    
			if ((modes[i]->hdisplay == h) && (modes[i]->vdisplay == w))
		  	  bestMode = i;                                                   
		}     

		XF86VidModeSwitchToMode(win->dsp, win->screen, modes[bestMode]);
		XF86VidModeSetViewPort(win->dsp, win->screen, 0, 0);            
		dpyWidth = modes[bestMode]->hdisplay;                     
		dpyHeight = modes[bestMode]->vdisplay;                    
		printf("resolution %dx%d\n", dpyWidth, dpyHeight);        
		XFree(modes);                                             

		/* set window attributes */
		win->attr.override_redirect = True;
		win->attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
		    StructureNotifyMask;   
                    
		win->xwin = XCreateWindow(win->dsp, RootWindow(win->dsp, win->vi->screen),    
		    0, 0, dpyWidth, dpyHeight, 0, win->vi->depth, InputOutput, win->vi->visual,
		    CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,   
		    &win->attr);                                                       
		XWarpPointer(win->dsp, None, win->xwin, 0, 0, 0, 0, 0, 0);               
			XMapRaised(win->dsp, win->xwin);                                 
		XGrabKeyboard(win->dsp, win->xwin, True, GrabModeAsync,                  
		    GrabModeAsync, CurrentTime);                                     
		XGrabPointer(win->dsp, win->xwin, True, ButtonPressMask,                 
		    GrabModeAsync, GrabModeAsync, win->xwin, None, CurrentTime); 

		 _FionaUTReshape(win, dpyWidth, dpyHeight);      
	}                                                                        
	else                                                                     
	{                                                  
		/* create a window in window mode*/                                  
		win->attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;                                             
		win->xwin = XCreateWindow(win->dsp, RootWindow(win->dsp, win->vi->screen),     
		    0, 0, w, h, 0, win->vi->depth, InputOutput, win->vi->visual,      
		    CWBorderPixel | CWColormap | CWEventMask, &win->attr);             
		
		/* only set window title and handle wm_delete_events if in windowed mode */
		Atom wmDelete = XInternAtom(win->dsp, "WM_DELETE_WINDOW", true);                 
		XSetWMProtocols(win->dsp, win->xwin, &wmDelete, 1);    
		                        
		XSetStandardProperties(win->dsp, win->xwin, TITLE,                             
		   TITLE, None, NULL, 0, NULL);                                                     
		XMapRaised(win->dsp, win->xwin);                                         
	}       

	/*

	__FionaUTMakeCurrent(win);
	unsigned int  		borderDummy;
	Window   			winDummy;

	
	printf("set geometry\n");
	 XGetGeometry(win->dsp, win->xwin, &winDummy,
				&win->x, &win->y,
		 		&win->w, &win->h,
				&borderDummy, (unsigned int *)&win->vi->depth);
	*/

	
	printf("returning %p\n", win);
	return win;
}

