//
//  FionaUTCocoa.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/7/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//
#include "FionaUT.h"
#import <Cocoa/Cocoa.h>
#include <sys/time.h>
#include "FionaUTCocoa.h"

NSOpenGLContext* sharedContext=NULL;
inline float msX(NSView* v,NSEvent* e) {
	return[v convertPoint:[e locationInWindow]fromView:nil].x;
}
inline float msY(NSView* v,NSEvent* e) {
	return[v convertPoint:[e locationInWindow]fromView:nil].y;
}

@implementation FionaUTWindow
- (BOOL) canBecomeKeyWindow { return YES; }
@end


@implementation FionaUTView
@synthesize contextUpdated;
// OpenGL Pixel Format Function: Generate default pixel format
+ (NSOpenGLPixelFormat*)defaultPixelFormat
{
	NSOpenGLPixelFormatAttribute attributesForStereo [] = {
		NSOpenGLPFADoubleBuffer, NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAMultisample, NSOpenGLPFASampleAlpha,
		NSOpenGLPFASampleBuffers, 1, NSOpenGLPFASamples, fionaConf.multisample,
		NSOpenGLPFAStereo,
		(NSOpenGLPixelFormatAttribute)nil };
	NSOpenGLPixelFormatAttribute attributes [] = {
		NSOpenGLPFADoubleBuffer, NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAMultisample, NSOpenGLPFASampleAlpha,
		NSOpenGLPFASampleBuffers, 1, NSOpenGLPFASamples, fionaConf.multisample,
		(NSOpenGLPixelFormatAttribute)nil };
	return (fionaGlobalMode&GLUT_STEREO)
			?[[(NSOpenGLPixelFormat *)[NSOpenGLPixelFormat alloc]
			 initWithAttributes:attributesForStereo] autorelease]
			:[[(NSOpenGLPixelFormat *)[NSOpenGLPixelFormat alloc]
			   initWithAttributes:attributes] autorelease];
}
// Common initializer for view class
- (void) __init
{
	[self setContextUpdated:false];
	NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(_surfaceNeedsUpdate:)
			   name:NSViewGlobalFrameDidChangeNotification object:self];
	[[self window] makeFirstResponder:self];
}
// Initialization function of class
- (id) initWithCoder:(NSCoder*)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) [self __init]; return self;
}
- (id) initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) [self __init]; return self;
}
// Deallocation function. Nothing special.
- (void) dealloc{
	NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
	[nc removeObserver:self name:NSViewGlobalFrameDidChangeNotification
				object:self];
	[super dealloc];
}

- (BOOL) acceptsFirstMouse:(NSEvent*)ev {return YES;}
- (BOOL) acceptsFirstResponder          {return YES;}


// Rendering function: check the context sharing between windows
- (void) drawRect:(NSRect)dirtyRect {
//	[super drawRect:dirtyRect];
	if( ![self contextUpdated] )
	{
		if( sharedContext!=NULL )
		{
			NSOpenGLContext* ctx;
			ctx = [[[NSOpenGLContext alloc]
					initWithFormat:[FionaUTView defaultPixelFormat]
					shareContext:sharedContext] autorelease];
			[self setOpenGLContext:ctx];
		}
		[self setContextUpdated:YES];

		int swap_interval = 0;
		if( fionaNetSlave || fionaConf.appType == FionaConfig::DEVLAB )
			swap_interval=1;
		[[self openGLContext] setValues:&swap_interval forParameter:NSOpenGLCPSwapInterval];
	}
	NSOpenGLContext* context = [self openGLContext];
	if( sharedContext==NULL ) sharedContext = context;
//	_FionaUTDisplay(self, context);
//	_FionaUTFrame();
}

// Callback method for window size change
- (void)_surfaceNeedsUpdate:(NSNotification*)notification {
	[[self openGLContext] update];
	_FionaUTReshape(self,[self frame].size.width,[self frame].size.height);
}
- (void)setFrame:(NSRect)frameRect {
	[super setFrame:frameRect];
	_FionaUTReshape(self,[self frame].size.width,[self frame].size.height);
}


// Callback method for various System UI events
- (void) keyDown:(NSEvent *)ev {
	//	unsigned int mod =  [ev modifierFlags];
	switch( [ev keyCode] )
	{
		case 126: _FionaUTJoystick(0, vec3( 0,0, 1)); break;  // up arrow
		case 125: _FionaUTJoystick(0, vec3( 0,0,-1)); break;  // down arrow
		case 123: _FionaUTJoystick(0, vec3(-1,0, 0)); break;  // left arrow
		case 124: _FionaUTJoystick(0, vec3( 1,0, 0)); break;  // right arrow
	}
	switch( [[ev characters] characterAtIndex:0] )
	{
		case ' ': _FionaUTWandButton(5, 1); break;
		case '1': _FionaUTWandButton(1, 1); break;
		case '2': _FionaUTWandButton(0, 1); break;
		case '3': _FionaUTWandButton(2, 1); break;
		case '4': _FionaUTWandButton(3, 1); break;
	}
	_FionaUTKeyboard(self, [[ev characters] characterAtIndex:0]);
}
- (void) keyUp:(NSEvent *)ev {
	//	unsigned int mod =  [ev modifierFlags];
	switch( [ev keyCode] )
	{
		case 126: _FionaUTJoystick(0, vec3( 0,0, 0)); break;  // up arrow
		case 125: _FionaUTJoystick(0, vec3( 0,0, 0)); break;  // down arrow
		case 124: _FionaUTJoystick(0, vec3( 0,0, 0)); break;  // left arrow
		case 123: _FionaUTJoystick(0, vec3( 0,0, 0)); break;  // right arrow
	}
	switch( [[ev characters] characterAtIndex:0] )
	{
		case '`': _FionaUTWandButton(5, 0); break;
		case '1': _FionaUTWandButton(1, 0); break;
		case '2': _FionaUTWandButton(0, 0); break;
		case '3': _FionaUTWandButton(2, 0); break;
		case '4': _FionaUTWandButton(3, 0); break;
	}
}
- (void) mouseEntered		:(NSEvent*)ev {}
- (void) mouseExited		:(NSEvent*)ev {}
- (void) mouseDown			:(NSEvent*)ev {
	_FionaUTMouseDown(self, GLUT_LEFT_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) mouseUp			:(NSEvent*)ev {
	_FionaUTMouseUp  (self, GLUT_LEFT_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) mouseDragged		:(NSEvent*)ev {
	_FionaUTMouseDrag(self, msX(self,ev), msY(self,ev));
}
- (void) mouseMoved			:(NSEvent*)ev {
	_FionaUTMouseMove(self, msX(self,ev), msY(self,ev));
}
- (void) rightMouseDown		:(NSEvent*)ev {
	_FionaUTMouseDown(self, GLUT_RIGHT_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) rightMouseUp			:(NSEvent*)ev {
	_FionaUTMouseUp  (self, GLUT_RIGHT_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) rightMouseDragged		:(NSEvent*)ev {
	_FionaUTMouseDrag(self, msX(self,ev), msY(self,ev));
}
- (void) otherMouseDown		:(NSEvent*)ev {
	_FionaUTMouseDown(self, GLUT_MIDDLE_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) otherMouseUp		:(NSEvent*)ev {
	_FionaUTMouseUp  (self, GLUT_MIDDLE_BUTTON, msX(self,ev), msY(self,ev));
}
- (void) otherMouseDragged	:(NSEvent*)ev {
	_FionaUTMouseDrag(self, msX(self,ev), msY(self,ev));
}
/*
- (void) magnifyWithEvent	:(NSEvent*)ev {
	if(ctrl->scroll1D([ev magnification])) [self setNeedsDisplay:YES];
}
- (void) rotateWithEvent	:(NSEvent*)ev {
	if(ctrl->touchRot([ev rotation])) [self setNeedsDisplay:YES];
}
- (void) scrollWheel		:(NSEvent*)ev {
	float dx = (float)[ev deltaX]/(float)[self frame].size.width;
	float dy =-(float)[ev deltaY]/(float)[self frame].size.height;
	if( ctrl->scroll2D(dx,dy) ) [self setNeedsDisplay:YES];
}
*/

- (void) timer:(id)ud
{
	static bool inited = false;
	static float last = 0;
	float cur = FionaUTTime();
	float step = (1/(fionaConf.framerate+1.0f));
	_FionaUTIdle();
	if( last-cur>500 ) last-=1000;
	if( !fionaNetSlave ) if( !inited || cur-last>=step)
	{
//		std::cout<<(cur)<<" ("<<((cur-last)*1000.0f)<<")";
		if( !inited )	inited=true, last=cur-step;
		else			last=cur;
		_FionaUTFrame();
	}
}

@end

// We need Native Windowing Function as follows
void* __FionaUTCreateWindow(const char* name,int x,int y,int w,int h,int mode)
{
	NSWindow* win = [[FionaUTWindow alloc]
					 initWithContentRect:NSMakeRect(x, y, w, h)
					 styleMask:((mode<0)?NSBorderlessWindowMask
								:(NSTitledWindowMask
								|NSClosableWindowMask
								|NSMiniaturizableWindowMask
								|NSResizableWindowMask))
					  backing:NSBackingStoreNonretained
						defer:NO];
	[win autorelease];
	[win makeKeyAndOrderFront:win];
	FionaUTView* view = [[FionaUTView alloc] initWithFrame:NSMakeRect(0,0,w,h)];
	[view autorelease];
	[win setContentView:view];
	if(mode>=0) [win display];
	[win setTitle:[NSString stringWithUTF8String:name]];
	[NSApp activateIgnoringOtherApps:YES];
//	if( mode==-1 )	__FionaUTMakeFillScreen(view,0);
	if( mode > 0 )	__FionaUTMakeFillScreen(view,mode-1);

	return view;
}
void  __FionaUTInitNative(void)
{
	[NSAutoreleasePool new];
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

void  __FionaUTMainLoop(int _framerate)
{
	if( _framerate>=1 ) fionaConf.framerate = _framerate;
	FionaUTView* view = (FionaUTView*)_FionaUTFirstWindow();
	if( view == NULL ) { printf("Cannot run without any window\n"); return; }
	[NSTimer scheduledTimerWithTimeInterval:0.001f target:view
								   selector:@selector(timer:)
								   userInfo:nil repeats:YES];
	[NSApp run];
	FionaUTExit();
}
void  __FionaUTCloseWindow(void) {
}
void  __FionaUTSwapBuffer(void* win) {
	[[(FionaUTView*)win openGLContext] flushBuffer];
}
CTX   __FionaUTGetContext(void* win) {
	return [(FionaUTView*)win openGLContext];
}
void  __FionaUTMakeCurrent(void* win) {
	[[(FionaUTView*)win openGLContext] makeCurrentContext];
}
int   __FionaUTGetW(WIN win) {
	return [(NSView*)win frame].size.width;
}
int   __FionaUTGetH(WIN win) {
	return [(NSView*)win frame].size.height;
}


void  __FionaUTMakeFillScreen(void* win, int i)
{
	NSMutableDictionary* dic = [NSMutableDictionary dictionaryWithObject:
								[NSNumber numberWithBool:YES]
								forKey:NSFullScreenModeAllScreens];

	[dic setObject:[NSNumber numberWithInt:
					NSApplicationPresentationAutoHideDock
					|NSApplicationPresentationAutoHideMenuBar]
			forKey:NSFullScreenModeApplicationPresentationOptions];

	NSArray* screens = [NSScreen screens];
	
	[(FionaUTView*)win enterFullScreenMode:[screens objectAtIndex:i]
							   withOptions:dic];
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
void  __FionaUTMakeWideScreen	(WIN hwnd)
{
}
