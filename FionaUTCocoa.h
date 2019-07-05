//
//  FionaUTCocoa.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/7/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

@interface FionaUTWindow: NSWindow
{
	
}
- (BOOL) canBecomeKeyWindow;
@end

@interface FionaUTView : NSOpenGLView
{
	BOOL contextUpdated;
}
- (BOOL) acceptsFirstMouse:(NSEvent *)theEvent;
- (void) dealloc;
- (void) timer:(id)ud;
@property(assign) BOOL contextUpdated;
@end

