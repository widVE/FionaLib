//
//  FionaUTRender.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/8/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#define BIG_OPENGL_OPT

#include "FionaUT.h"
#ifndef LINUX_BUILD
#include "GL/wglew.h"	//for nvcopyimage
#endif
#include <Kit3D/jtrans.h>

#include "glm/common.hpp"
#include "glm/gtx/matrix_interpolation.hpp"

int fionaRenderCycleCount = 0;
bool fionaRenderCycleLeft = false;
bool fionaRenderCycleRight= false;

const FionaViewport VP_FBO(0,0,1,1,0,0,1,1);

static GLuint	extFBOTexture = 0;
static bool		isInFBO = false;
static bool		isUsingExtFBO = false;

//#define NEW_DISTORTION

#ifdef ENABLE_OCULUS
#include <Kit3D/glslUtils.h>

static GLuint m_quadVertexBuffer = 0;
static GLuint m_quadLeftUVBuffer = 0;
static GLuint m_quadRightUVBuffer = 0;
static GLuint correctionProgram=0;
static GLuint uniformLensCenter=0;
static GLuint uniformScreenCenter=0;
static GLuint uniformScale=0;
static GLuint uniformScaleIn=0;
static GLuint uniformHMDWarpParam=0;
static GLuint uniformChromAbParam=0;
static GLuint uniformTexture0=0;

#ifdef ENABLE_DK2
static GLuint uniformScreenCenterOffset = 0;
static GLuint uniformDistortionClear = 0;
static GLuint uniformTanEyeAngleScale = 0;
static GLuint uniformTanEyeAngleOffset = 0;
static GLuint uniformView = 0;
static GLuint uniformTexM = 0;
#endif

/*static const char* PostProcessVertexShaderSrc =
    "#version 110\n"
    
    "uniform mat4 View;\n"
    "uniform mat4 Texm;\n"
    
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    
    "varying vec2 oTexCoord;\n"
    
    "void main()\n"
    "{\n"
    "   gl_Position = View * Position;\n"
    "   oTexCoord = vec2(Texm * vec4(TexCoord,0,1));\n"
    "}\n";
#else*/
static const char* PostProcessVertexShaderSrc =
    "attribute vec3 Position;\n"
	"attribute vec2 TexCoord;\n"
    "varying vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(Position, 1.0);\n"
	"	oTexCoord = TexCoord;\n"
    "}\n";

static const char* PostProcessFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    "vec2 HmdWarp(vec2 in01)\n"
    "{\n"
    "   vec2 theta = (in01 - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq = theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2 theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
	"   return LensCenter + (Scale * theta1);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "   vec2 tc = HmdWarp(oTexCoord);\n"
    "   if (!all(equal(clamp(tc, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tc)))\n"
    "       gl_FragColor = vec4(0);\n"
    "   else\n"
    "       gl_FragColor = texture2D(Texture0, tc);\n"
    "}\n";

static const char* PostProcessFragTestShaderSrc =
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
	"   gl_FragColor = texture2D(Texture0, oTexCoord);\n"
    "}\n";

#ifdef ENABLE_DK2
// Shader with lens distortion and chromatic aberration correction.
#ifdef NEW_DISTORTION
static const char* PostProcessFullFragShaderSrc =
    "#version 110\n"
    
    "uniform sampler2D Texture;\n"
    "uniform vec3 DistortionClearColor;\n"
    "uniform float EdgeFadeScale;\n"
    "uniform vec2 EyeToSourceUVScale;\n"
    "uniform vec2 EyeToSourceUVOffset;\n"
    "uniform vec2 EyeToSourceNDCScale;\n"
    "uniform vec2 EyeToSourceNDCOffset;\n"
    "uniform vec2 TanEyeAngleScale;\n"
    "uniform vec2 TanEyeAngleOffset;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform vec4 ChromAbParam;\n"

    "varying vec4 oPosition;\n"
    "varying vec2 oTexCoord;\n"

	"void main()\n"
    "{\n"
    // Input oTexCoord is [-1,1] across the half of the screen used for a single eye.
    "   vec2 TanEyeAngleDistorted = oTexCoord * TanEyeAngleScale + TanEyeAngleOffset;\n" // Scales to tan(thetaX),tan(thetaY), but still distorted (i.e. only the center is correct)
    "   float  RadiusSq = TanEyeAngleDistorted.x * TanEyeAngleDistorted.x + TanEyeAngleDistorted.y * TanEyeAngleDistorted.y;\n"
    "   float Distort = 1.0 / ( 1.0 + RadiusSq * ( HmdWarpParam.y + RadiusSq * ( HmdWarpParam.z + RadiusSq * ( HmdWarpParam.w ) ) ) );\n"
    "   float DistortR = Distort * ( ChromAbParam.x + RadiusSq * ChromAbParam.y );\n"
    "   float DistortG = Distort;\n"
    "   float DistortB = Distort * ( ChromAbParam.z + RadiusSq * ChromAbParam.w );\n"
    "   vec2 TanEyeAngleR = DistortR * TanEyeAngleDistorted;\n"
    "   vec2 TanEyeAngleG = DistortG * TanEyeAngleDistorted;\n"
    "   vec2 TanEyeAngleB = DistortB * TanEyeAngleDistorted;\n"

    // These are now in "TanEyeAngle" space.
    // The vectors (TanEyeAngleRGB.x, TanEyeAngleRGB.y, 1.0) are real-world vectors pointing from the eye to where the components of the pixel appear to be.
    // If you had a raytracer, you could just use them directly.

    // Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
    "   vec2 SourceCoordR = TanEyeAngleR * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
	"	SourceCoordR.y = 1.0 - SourceCoordR.y;\n"
    "   vec2 SourceCoordG = TanEyeAngleG * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
	"	SourceCoordG.y = 1.0 - SourceCoordG.y;\n"
    "   vec2 SourceCoordB = TanEyeAngleB * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
	"	SourceCoordB.y = 1.0 - SourceCoordB.y;\n"

    // Find the distance to the nearest edge.
    "   vec2 NDCCoord = TanEyeAngleG * EyeToSourceNDCScale + EyeToSourceNDCOffset;\n"
	"   float EdgeFadeIn = clamp ( EdgeFadeScale, 0.0, 1e5 ) * ( 1.0 - max ( abs ( NDCCoord.x ), abs ( NDCCoord.y ) ) );\n"
    "   if ( EdgeFadeIn < 0.0 )\n"
    "   {\n"
    "       gl_FragColor = vec4(DistortionClearColor.r, DistortionClearColor.g, DistortionClearColor.b, 1.0);\n"
    "       return;\n"
    "   }\n"
    "   EdgeFadeIn = clamp ( EdgeFadeIn, 0.0, 1.0 );\n"

    // Actually do the lookups.
    "   float ResultR = texture2D(Texture, SourceCoordR).r;\n"
    "   float ResultG = texture2D(Texture, SourceCoordG).g;\n"
    "   float ResultB = texture2D(Texture, SourceCoordB).b;\n"

    "   gl_FragColor = vec4(ResultR * EdgeFadeIn, ResultG * EdgeFadeIn, ResultB * EdgeFadeIn, 1.0);\n"
    "}\n";
#else
// Shader with lens distortion and chromatic aberration correction.
static const char* PostProcessFullFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform vec4 ChromAbParam;\n"
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    // Scales input texture coordinates for distortion.
    // ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
    // larger due to aspect ratio.
    "void main()\n"
    "{\n"
    "   vec2  theta = (oTexCoord - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq= theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + "
    "                  HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
    "   \n"
    "   // Detect whether blue texture coordinates are out of range since these will scaled out the furthest.\n"
    "   vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);\n"
    "   vec2 tcBlue = LensCenter + Scale * thetaBlue;\n"
    "   if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tcBlue)))\n"
    "   {\n"
    "       gl_FragColor = vec4(0);\n"
    "       return;\n"
    "   }\n"
    "   \n"
    "   // Now do blue texture lookup.\n"
    "   float blue = texture2D(Texture0, tcBlue).b;\n"
    "   \n"
    "   // Do green lookup (no scaling).\n"
    "   vec2  tcGreen = LensCenter + Scale * theta1;\n"
    "   vec4  center = texture2D(Texture0, tcGreen);\n"
    "   \n"
    "   // Do red scale and lookup.\n"
    "   vec2  thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);\n"
    "   vec2  tcRed = LensCenter + Scale * thetaRed;\n"
    "   float red = texture2D(Texture0, tcRed).r;\n"
    "   \n"
    "   gl_FragColor = vec4(red, center.g, blue, center.a);\n"
    "}\n";
#endif
#endif

#ifdef ENABLE_CV1
// Shader with lens distortion and chromatic aberration correction.
#ifdef NEW_DISTORTION
static const char* PostProcessFullFragShaderSrc =
"#version 110\n"

"uniform sampler2D Texture;\n"
"uniform vec3 DistortionClearColor;\n"
"uniform float EdgeFadeScale;\n"
"uniform vec2 EyeToSourceUVScale;\n"
"uniform vec2 EyeToSourceUVOffset;\n"
"uniform vec2 EyeToSourceNDCScale;\n"
"uniform vec2 EyeToSourceNDCOffset;\n"
"uniform vec2 TanEyeAngleScale;\n"
"uniform vec2 TanEyeAngleOffset;\n"
"uniform vec4 HmdWarpParam;\n"
"uniform vec4 ChromAbParam;\n"

"varying vec4 oPosition;\n"
"varying vec2 oTexCoord;\n"

"void main()\n"
"{\n"
// Input oTexCoord is [-1,1] across the half of the screen used for a single eye.
"   vec2 TanEyeAngleDistorted = oTexCoord * TanEyeAngleScale + TanEyeAngleOffset;\n" // Scales to tan(thetaX),tan(thetaY), but still distorted (i.e. only the center is correct)
"   float  RadiusSq = TanEyeAngleDistorted.x * TanEyeAngleDistorted.x + TanEyeAngleDistorted.y * TanEyeAngleDistorted.y;\n"
"   float Distort = 1.0 / ( 1.0 + RadiusSq * ( HmdWarpParam.y + RadiusSq * ( HmdWarpParam.z + RadiusSq * ( HmdWarpParam.w ) ) ) );\n"
"   float DistortR = Distort * ( ChromAbParam.x + RadiusSq * ChromAbParam.y );\n"
"   float DistortG = Distort;\n"
"   float DistortB = Distort * ( ChromAbParam.z + RadiusSq * ChromAbParam.w );\n"
"   vec2 TanEyeAngleR = DistortR * TanEyeAngleDistorted;\n"
"   vec2 TanEyeAngleG = DistortG * TanEyeAngleDistorted;\n"
"   vec2 TanEyeAngleB = DistortB * TanEyeAngleDistorted;\n"

// These are now in "TanEyeAngle" space.
// The vectors (TanEyeAngleRGB.x, TanEyeAngleRGB.y, 1.0) are real-world vectors pointing from the eye to where the components of the pixel appear to be.
// If you had a raytracer, you could just use them directly.

// Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
"   vec2 SourceCoordR = TanEyeAngleR * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
"	SourceCoordR.y = 1.0 - SourceCoordR.y;\n"
"   vec2 SourceCoordG = TanEyeAngleG * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
"	SourceCoordG.y = 1.0 - SourceCoordG.y;\n"
"   vec2 SourceCoordB = TanEyeAngleB * EyeToSourceUVScale + EyeToSourceUVOffset;\n"
"	SourceCoordB.y = 1.0 - SourceCoordB.y;\n"

// Find the distance to the nearest edge.
"   vec2 NDCCoord = TanEyeAngleG * EyeToSourceNDCScale + EyeToSourceNDCOffset;\n"
"   float EdgeFadeIn = clamp ( EdgeFadeScale, 0.0, 1e5 ) * ( 1.0 - max ( abs ( NDCCoord.x ), abs ( NDCCoord.y ) ) );\n"
"   if ( EdgeFadeIn < 0.0 )\n"
"   {\n"
"       gl_FragColor = vec4(DistortionClearColor.r, DistortionClearColor.g, DistortionClearColor.b, 1.0);\n"
"       return;\n"
"   }\n"
"   EdgeFadeIn = clamp ( EdgeFadeIn, 0.0, 1.0 );\n"

// Actually do the lookups.
"   float ResultR = texture2D(Texture, SourceCoordR).r;\n"
"   float ResultG = texture2D(Texture, SourceCoordG).g;\n"
"   float ResultB = texture2D(Texture, SourceCoordB).b;\n"

"   gl_FragColor = vec4(ResultR * EdgeFadeIn, ResultG * EdgeFadeIn, ResultB * EdgeFadeIn, 1.0);\n"
"}\n";
#else
// Shader with lens distortion and chromatic aberration correction.
static const char* PostProcessFullFragShaderSrc =
"uniform vec2 LensCenter;\n"
"uniform vec2 ScreenCenter;\n"
"uniform vec2 Scale;\n"
"uniform vec2 ScaleIn;\n"
"uniform vec4 HmdWarpParam;\n"
"uniform vec4 ChromAbParam;\n"
"uniform sampler2D Texture0;\n"
"varying vec2 oTexCoord;\n"
"\n"
// Scales input texture coordinates for distortion.
// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
// larger due to aspect ratio.
"void main()\n"
"{\n"
"   vec2  theta = (oTexCoord - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
"   float rSq= theta.x * theta.x + theta.y * theta.y;\n"
"   vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + "
"                  HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
"   \n"
"   // Detect whether blue texture coordinates are out of range since these will scaled out the furthest.\n"
"   vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);\n"
"   vec2 tcBlue = LensCenter + Scale * thetaBlue;\n"
"   if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tcBlue)))\n"
"   {\n"
"       gl_FragColor = vec4(0);\n"
"       return;\n"
"   }\n"
"   \n"
"   // Now do blue texture lookup.\n"
"   float blue = texture2D(Texture0, tcBlue).b;\n"
"   \n"
"   // Do green lookup (no scaling).\n"
"   vec2  tcGreen = LensCenter + Scale * theta1;\n"
"   vec4  center = texture2D(Texture0, tcGreen);\n"
"   \n"
"   // Do red scale and lookup.\n"
"   vec2  thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);\n"
"   vec2  tcRed = LensCenter + Scale * thetaRed;\n"
"   float red = texture2D(Texture0, tcRed).r;\n"
"   \n"
"   gl_FragColor = vec4(red, center.g, blue, center.a);\n"
"}\n";
#endif
#endif

static void createOculusCorrection(void)
{
#ifdef ENABLE_DK2
	correctionProgram = loadProgram(std::string(PostProcessVertexShaderSrc), std::string(PostProcessFullFragShaderSrc), false);
#ifdef NEW_DISTORTION
	uniformLensCenter=glGetUniformLocation(correctionProgram, "EdgeFadeScale");
	uniformScreenCenter=glGetUniformLocation(correctionProgram, "EyeToSourceNDCScale");
	uniformScreenCenterOffset=glGetUniformLocation(correctionProgram, "EyeToSourceNDCOffset");
	uniformScale=glGetUniformLocation(correctionProgram, "EyeToSourceUVScale");
	uniformScaleIn=glGetUniformLocation(correctionProgram, "EyeToSourceUVOffset");
	uniformHMDWarpParam=glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam=glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0=glGetUniformLocation(correctionProgram, "Texture");
	uniformDistortionClear = glGetUniformLocation(correctionProgram, "DistortionClearColor");
	uniformTanEyeAngleScale = glGetUniformLocation(correctionProgram, "TanEyeAngleScale");
	uniformTanEyeAngleOffset = glGetUniformLocation(correctionProgram, "TanEyeAngleOffset");
	uniformView = glGetUniformLocation(correctionProgram, "View");
	uniformTexM = glGetUniformLocation(correctionProgram, "Texm");
#else
	uniformLensCenter=glGetUniformLocation(correctionProgram, "LensCenter");
	uniformScreenCenter=glGetUniformLocation(correctionProgram, "ScreenCenter");
	uniformScale=glGetUniformLocation(correctionProgram, "Scale");
	uniformScaleIn=glGetUniformLocation(correctionProgram, "ScaleIn");
	uniformHMDWarpParam=glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam=glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0=glGetUniformLocation(correctionProgram, "Texture0");
#endif
#endif
#ifdef ENABLE_CV1
	correctionProgram = loadProgram(std::string(PostProcessVertexShaderSrc), std::string(PostProcessFullFragShaderSrc), false);
#ifdef NEW_DISTORTION
	uniformLensCenter = glGetUniformLocation(correctionProgram, "EdgeFadeScale");
	uniformScreenCenter = glGetUniformLocation(correctionProgram, "EyeToSourceNDCScale");
	uniformScreenCenterOffset = glGetUniformLocation(correctionProgram, "EyeToSourceNDCOffset");
	uniformScale = glGetUniformLocation(correctionProgram, "EyeToSourceUVScale");
	uniformScaleIn = glGetUniformLocation(correctionProgram, "EyeToSourceUVOffset");
	uniformHMDWarpParam = glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam = glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0 = glGetUniformLocation(correctionProgram, "Texture");
	uniformDistortionClear = glGetUniformLocation(correctionProgram, "DistortionClearColor");
	uniformTanEyeAngleScale = glGetUniformLocation(correctionProgram, "TanEyeAngleScale");
	uniformTanEyeAngleOffset = glGetUniformLocation(correctionProgram, "TanEyeAngleOffset");
	uniformView = glGetUniformLocation(correctionProgram, "View");
	uniformTexM = glGetUniformLocation(correctionProgram, "Texm");
#else
	uniformLensCenter = glGetUniformLocation(correctionProgram, "LensCenter");
	uniformScreenCenter = glGetUniformLocation(correctionProgram, "ScreenCenter");
	uniformScale = glGetUniformLocation(correctionProgram, "Scale");
	uniformScaleIn = glGetUniformLocation(correctionProgram, "ScaleIn");
	uniformHMDWarpParam = glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam = glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0 = glGetUniformLocation(correctionProgram, "Texture0");
#endif
#endif
}
#endif

// Drawing rectangle with textured
// The first four parameters are for the area to cover (-1 to 1)
// The last four parameters are area of the source image (0 to 1)
static void drawRect(float l, float r, float b, float t,
					 float tl, float tr, float tb, float tt)
{
#ifndef ENABLE_OCULUS
	glBegin(GL_QUADS);
	glTexCoord2f(tl, tb); glVertex2f(l, b);
	glTexCoord2f(tr, tb); glVertex2f(r, b);
	glTexCoord2f(tr, tt); glVertex2f(r, t);
	glTexCoord2f(tl, tt); glVertex2f(l, t);
	glEnd();
#else

	static const GLfloat g_quad_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			1.0f,  1.0f, 0.0f
	};

	static const GLfloat q_tex_buffer_data_left[] = {
		0.f, 0.f,
		0.5f, 0.f,
		0.f, 1.f,
		0.f, 1.f,
		0.5f, 0.f,
		0.5f, 1.f };

	static const GLfloat q_tex_buffer_data_right[] = {
		0.5f, 0.f,
		1.f, 0.f,
		0.5f, 1.f,
		0.5f, 1.f,
		1.f, 0.f,
		1.f, 1.f };

	if(m_quadVertexBuffer == 0)
	{
		glGenBuffers(1, &m_quadVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	}

	if(m_quadLeftUVBuffer == 0)
	{
		glGenBuffers(1, &m_quadLeftUVBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadLeftUVBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(q_tex_buffer_data_left), q_tex_buffer_data_left, GL_STATIC_DRAW);
	}
	
	if(m_quadRightUVBuffer == 0)
	{
		glGenBuffers(1, &m_quadRightUVBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadRightUVBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(q_tex_buffer_data_left), q_tex_buffer_data_right, GL_STATIC_DRAW);
	}

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVertexBuffer);
  glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
  );

  if(fionaRenderCycleLeft)
  {
	  glEnableVertexAttribArray(1);
	  glBindBuffer(GL_ARRAY_BUFFER, m_quadLeftUVBuffer);
	  glVertexAttribPointer(
			1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
	  );
  }
  else if(fionaRenderCycleRight)
  {
	  glEnableVertexAttribArray(1);
	  glBindBuffer(GL_ARRAY_BUFFER, m_quadRightUVBuffer);
	  glVertexAttribPointer(
			1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
	  );
  }

	// Draw the triangles !
  glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
#endif
}

// Simple helper function to create FBO and corresponding texture
static bool createFBO(int w, int h)
{
#ifdef LINUX_BUILD
	glewInit();
#endif
	printf("Creating FBO of size %dx%d\n", w, h);
	glGenFramebuffers(1, &fionaConf.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
	
	glGenTextures(1, &fionaConf.img);
	glBindTexture(GL_TEXTURE_2D, fionaConf.img);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);//LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);//LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glGenTextures(1, &fionaConf.depth);
	glBindTexture(GL_TEXTURE_2D, fionaConf.depth);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fionaConf.img, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fionaConf.depth, 0);
	
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	if( status != GL_FRAMEBUFFER_COMPLETE )
	{
		printf("FBO: Something wrong..\n");
		return false;
	}
	
	printf("MADE FRAMEBUFFER OBJECT!\n");
	//glEnable(GL_MULTISAMPLE);
	return true;
}

tran caveProjectionWall(const FionaWall &wall, const jvec3& e)
{
	//this version doesn't include the eye position within the projection matrix.
	jvec3 vr = (wall.vLB - wall.vRB).normalize();
	jvec3 vu = (wall.vLB - wall.vLT).normalize();
	jvec3 vn = (vr * vu).normalize();

	//compute screen corner vectors
	jvec3 va = wall.vLB - e;
	jvec3 vb = wall.vRB - e;
	jvec3 vc = wall.vLT - e;

	//find the distance from the eye to screen plane
	float n = fionaConf.nearClip;
	float f = fionaConf.farClip;
	float d = va % vn;
	float nod = n / d;
	float l = (vr % va) * nod;
	float r = (vr % vb) * nod;
	float b = (vu % va) * nod;
	float t = (vu % vc) * nod;

	return tran(2.0f * n / (r - l), 0.f, (r + l) / (r - l), 0.f,
		0.f, 2.0f * n / (t - b), (t + b) / (t - b), 0.f,
		0.f, 0.f, -(f + n) / (f - n), (-2.0f * f * n) / (f - n),
		0.f, 0.f, -1.f, 0.f);
}

tran caveProjection(const jvec3& sz, const jvec3& e)
{
	//this version does include the eye position within the projection matrix and a lot of hyun joon's crazy math.
	float f=fionaConf.farClip, n=fionaConf.nearClip; // far and near clip depth
	float w=ABS(sz.x/2), h=ABS(sz.y/2), d=ABS(sz.z/2);

	/*printf("*********************\n%2f, %2f, %2f, %2f\n%2f, %2f, %2f, %2f\n, %2f, %2f, %2f, %2f\n, %2f, %2f, %2f, %2f\n",
				d/w+e.z/w, 0.f,       -e.x/w,      -e.x*d/w,
			  0.f,         d/h+e.z/h, -e.y/h,      -e.y*d/h,
			  0.f,         0.f,         -1.f*(-f+e.z+n)/(-f+e.z-n),(2.f*n*f+e.z*(-f-n)+e.z*e.z)/(-f+e.z-n),
			  0.f,         0.f,         -1.f, e.z);*/

	// Cave projection matrix
	//printf("%f\n", d / w);
	return tran(d/w+e.z/w, 0.f,			-e.x/w,							-e.x*d/w,
			  0.f,         d/h+e.z/h,	-e.y/h,							-e.y*d/h,
			  0.f,         0.f,         -1.f*(-f+e.z+n)/(-f+e.z-n),		(2.f*n*f+e.z*(-f-n)+e.z*e.z)/(-f+e.z-n),
			  0.f,         0.f,         -1.f,							e.z);
}

tran projectorCalibration(const FionaViewport& vp)
{
	return tran(vp.cScaleX,0,0,vp.cOffsetX,0,vp.cScaleY,0,vp.cOffsetY,0,0,1,0,0,0,0,1);
}

void caveProjectionGLM(const glm::vec3& sz, const glm::vec3& e, glm::mat4 &caveMat)
{
	float f=fionaConf.farClip, n=fionaConf.nearClip; // far and near clip depth
	float w=ABS(sz.x/2), h=ABS(sz.y/2), d=ABS(sz.z/2);

	// Cave projection matrix
	caveMat[0][0]=d/w+e.z/w;
	caveMat[0][1]=0.f;
	caveMat[0][2]=-e.x/w;
	caveMat[0][3]=-e.x*d/w;
	caveMat[1][0]=0.f;
	caveMat[1][1]=d/h+e.z/h;
	caveMat[1][2]=-e.y/h;
	caveMat[1][3]=-e.y*d/h;
	caveMat[2][0]=0.f;
	caveMat[2][1]=0.f;
	caveMat[2][2]=-1.f*(-f+e.z+n)/(-f+e.z-n);
	caveMat[2][3]=(2.f*n*f+e.z*(-f-n)+e.z*e.z)/(-f+e.z-n);
	caveMat[3][0]=0.f;
	caveMat[3][1]=0.f;
	caveMat[3][2]=-1.f;
	caveMat[3][3]=e.z;
}

void projectionCalibrationGLM(const FionaViewport &vp, glm::mat4 &pcMat)
{
	pcMat[0][0] = vp.cScaleX;
	pcMat[0][1] = 0.f;
	pcMat[0][2] = 0.f;
	pcMat[0][3] = vp.cOffsetX;
	pcMat[1][0] = 0.f;
	pcMat[1][1] = vp.cScaleY;
	pcMat[1][2] = 0.f;
	pcMat[1][3] = vp.cOffsetY;
	pcMat[2][0] = 0.f;
	pcMat[2][1] = 0.f;
	pcMat[2][2] = 1.f;
	pcMat[2][3] = 0.f;
	pcMat[3][0] = 0.f;
	pcMat[3][1] = 0.f;
	pcMat[3][2] = 0.f;
	pcMat[3][3] = 1.f;
}

void viewportProjectionGLM(const FionaViewport& vp, glm::mat4 &vpMat)
{
	float x0=vp.sx*2-1, w=vp.sw*2;
	float y0=vp.sy*2-1, h=vp.sh*2;

	vpMat[0][0] = 2.f/w;
	vpMat[0][1] = 0.f;
	vpMat[0][2] = 0.f;
	vpMat[0][3] = -2.f/w*x0-1.f;
	vpMat[1][0] = 0.f;
	vpMat[1][1] = 2.f/h;
	vpMat[1][2] = 0.f;
	vpMat[1][3] = -2.f/h*y0-1.f;
	vpMat[2][0] = 0.f;
	vpMat[2][1] = 0.f;
	vpMat[2][2] = 1.f;
	vpMat[2][3] = 0.f;
	vpMat[3][0] = 0.f;
	vpMat[3][1] = 0.f;
	vpMat[3][2] = 0.f;
	vpMat[3][3] = 1.f;
}

tran viewportProjection(const FionaViewport& vp)
{
	float x0=vp.sx*2-1, w=vp.sw*2;
	float y0=vp.sy*2-1, h=vp.sh*2;
	
	return tran(2/w,0,0,-2/w*x0-1,
				0,2/h,0,-2/h*y0-1,
				0,0,1,0,0,0,0,1);
}

jvec3 _FionaUTCalcEyePosition(int eye, const FionaWall &wall)
{
	jvec3 head = fionaConf.headPos;
	jvec3 eyePos;
	if (eye == 0)
	{
		eyePos = fionaConf.kevinOffset;
	}
	else if (eye == 1)
	{
		eyePos = fionaConf.kevinOffset + fionaConf.lEyeOffset;
	}
	else if (eye == 2)
	{
		eyePos = fionaConf.kevinOffset + fionaConf.rEyeOffset;
	}

	//wall preRot is calculated in FionaWall constructor
	quat preRot = wall.preRot;
	//same with wall cntr
	//head is now in coordinate space relative to wall center
	head = preRot.rot(head)-wall.cntr;

	eyePos = preRot.rot(fionaConf.headRot.rot(eyePos)) + head;
	return eyePos;
}

void _FionaUTCalcMatrices(int eye, int wini, const FionaWall& wall, const FionaViewport& vp, const jvec3& _ep, int w, int h)
{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
	//static float    BodyYaw(3.141592f);
	//static OVR::Vector3f HeadPos(0.0f, 1.6f, -5.0f);

   
  //code below appears in latest SDK.. may need to replace below stuff with it...
	ovrVector3f               eyeOffset[2];
	eyeOffset[0] = fionaConf.oculusEyeRenderDesc[0].HmdToEyeViewOffset;
    eyeOffset[1] = fionaConf.oculusEyeRenderDesc[1].HmdToEyeViewOffset;

	//ovrPosef                  poses[2];

	ovrFrameTiming   ftiming = ovrHmd_GetFrameTiming(fionaConf.oculusHmd, 0);
	ovrTrackingState hmdState = ovrHmd_GetTrackingState(fionaConf.oculusHmd, ftiming.DisplayMidpointSeconds);
	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, eyeOffset, fionaConf.layer.RenderPose);//poses);

	//these could be moved outside of the loop
   /* ovrEyeType oeye = fionaConf.oculusHmd->EyeRenderOrder[eye];
	ovrEyeRenderDesc desc = ovrHmd_GetRenderDesc(fionaConf.oculusHmd, ovrEyeType::ovrEye_Left, fionaConf.oculusHmd->DefaultEyeFov[0]);
	ovrEyeRenderDesc desc2 = ovrHmd_GetRenderDesc(fionaConf.oculusHmd, ovrEyeType::ovrEye_Right, fionaConf.oculusHmd->DefaultEyeFov[1]);
	ovrPosef poses[2];
	//ovrVector3f eyeOffset[2];
	eyeOffset[0] = desc.HmdToEyeViewOffset;
	eyeOffset[1] = desc2.HmdToEyeViewOffset;*/
	//ovrHmd_GetEyePoses(fionaConf.oculusHmd, 0, eyeOffset, poses, 0);
	ovrPosef eyeRenderPose = fionaConf.layer.RenderPose[eye == 2 ? 1 : 0];//ovrHmd_GetHmdPosePerEye(fionaConf.oculusHmd, oeye);

    // Get view and projection matrices
	//OVR::Matrix4f rollPitchYaw       = OVR::Matrix4f::RotationY(BodyYaw);
	//OVR::Matrix4f finalRollPitchYaw  = /*rollPitchYaw * OVR::Matrix4f(eyeRenderPose.Orientation);
	//OVR::Vector3f finalUp            = finalRollPitchYaw.Transform(OVR::Vector3f(0,1,0));
	//OVR::Vector3f finalForward       = finalRollPitchYaw.Transform(OVR::Vector3f(0,0,-1));
	fionaConf.headPos = jvec3(eyeRenderPose.Position.x, eyeRenderPose.Position.y, eyeRenderPose.Position.z);
	fionaConf.headRot = quat(eyeRenderPose.Orientation.w, eyeRenderPose.Orientation.x, eyeRenderPose.Orientation.y, eyeRenderPose.Orientation.z);
	//OVR::Vector3f shiftedEyePos      = HeadPos + /*rollPitchYaw.Transform(*/eyeRenderPose.Position;//);
    //OVR::Matrix4f view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp); 
	//OVR::Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[oeye].Fov, 0.01f, 10000.0f, true);
	//pRender->SetViewport(Recti(EyeRenderViewport[eye]));
	//pRoomScene->Render(pRender, Matrix4f::Translation(EyeRenderDesc[eye].ViewAdjust) * view);
#endif
#endif

#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
	
	ovrEyeRenderDesc eyeRenderDesc[2];
	eyeRenderDesc[0] = ovr_GetRenderDesc(fionaConf.session, ovrEye_Left, fionaConf.hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(fionaConf.session, ovrEye_Right, fionaConf.hmdDesc.DefaultEyeFov[1]);

	ovrVector3f               HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset,
													eyeRenderDesc[1].HmdToEyeOffset };

	if (fionaRenderCycleCount == 0)
	{
		fionaConf.sensorSampleTime = ovr_GetPredictedDisplayTime(fionaConf.session, fionaConf.frameIndex);
		ovrTrackingState hmdState = ovr_GetTrackingState(fionaConf.session, fionaConf.sensorSampleTime, ovrTrue);
		ovr_CalcEyePoses(hmdState.HeadPose.ThePose, HmdToEyeOffset, fionaConf.eyeRenderPose);
	}
	//ovr_GetEyePoses(fionaConf.session, fionaConf.frameIndex, ovrTrue, HmdToEyeOffset, fionaConf.eyeRenderPose, &fionaConf.sensorSampleTime);
	
	fionaConf.headPos = jvec3(fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Position.x, fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Position.y, fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Position.z);
	fionaConf.headRot = quat(fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Orientation.w, fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Orientation.x, fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Orientation.y, fionaConf.eyeRenderPose[eye == 2 ? 1 : 0].Orientation.z);

	//this also sets viewport...
	fionaConf.eyeRenderTexture[eye == 2 ? 1 : 0]->SetAndClearRenderSurface(fionaConf.eyeDepthBuffer[eye == 2 ? 1 : 0]);
	
#endif
#endif

	if(eye==0||eye==1)	
	{
		if(fionaConf.appType != FionaConfig::OCULUS)
		{
#ifdef ENABLE_VIVE
			glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.leftEyeDesc.m_nRenderFramebufferId);
			glViewport(0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight);
			glClearColor(0.f, 1.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
			glViewport(vp.lx*w,vp.ly*h,vp.lw*w,vp.lh*h);
#endif
		}
		else
		{			
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
			glViewport(0, 0, fionaConf.FBOWidth / 2, fionaConf.FBOHeight);
#endif
#ifdef ENABLE_DK1
			OVR::Util::Render::StereoEyeParams le = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Left);
			glViewport(le.VP.x, le.VP.y, le.VP.w, le.VP.h);
#endif
#endif
		}
	}
	else
	{
		if(fionaConf.appType != FionaConfig::OCULUS)
		{
#ifdef ENABLE_VIVE
			glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.rightEyeDesc.m_nRenderFramebufferId);
			glViewport(0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight);
			glClearColor(1.f, 0.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
			glViewport(vp.rx*w,vp.ry*h,vp.rw*w,vp.rh*h);
#endif
		}
		else
		{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
			glViewport((fionaConf.FBOWidth) / 2, 0, fionaConf.FBOWidth / 2, fionaConf.FBOHeight);
#endif
#ifdef ENABLE_DK1
			OVR::Util::Render::StereoEyeParams re = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Right);
			glViewport(re.VP.x, re.VP.y, re.VP.w, re.VP.h);
#endif
#endif
		}
	}
	
	// setup projection matrix
	glMatrixMode(GL_PROJECTION); 
	glPushMatrix(); 
	glLoadIdentity();
	
	if( fionaConf.desktopProjection )
	{
		gluPerspective(fionaConf.desktopFOV, (double)((double)w / (double)h), fionaConf.nearClip, fionaConf.farClip);
	}
	else
	{
		if (fionaConf.appType != FionaConfig::OCULUS)
		{
#ifdef ENABLE_VIVE
			if (eye == 2)
			{
				glm::mat4 mvp = fionaConf.projRight * fionaConf.mvRight * fionaConf.hmdPose;
				glMultMatrix(glm::value_ptr(mvp));
			}
			else
			{
				glm::mat4 mvp = fionaConf.projLeft * fionaConf.mvLeft * fionaConf.hmdPose;
				glMultMatrix(glm::value_ptr(mvp));
			}
#else
			glMultMatrix(viewportProjection(vp)*projectorCalibration(vp)*caveProjection(wall.sz,_ep));
			//glMultMatrix(viewportProjection(vp)*projectorCalibration(vp)*caveProjectionWall(wall, _ep));
#endif
		}
		else
		{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
			OVR::Matrix4f m;
			//todo - pull fov from renderpose if possible..
			if(eye <= 1)
			{
				m = ovrMatrix4f_Projection(fionaConf.oculusEyeRenderDesc[0].Fov, fionaConf.nearClip, fionaConf.farClip, ovrProjection_RightHanded);// | ovrProjection_FarLessThanNear);
			}
			else
			{
				m = ovrMatrix4f_Projection(fionaConf.oculusEyeRenderDesc[1].Fov, fionaConf.nearClip, fionaConf.farClip, ovrProjection_RightHanded);// | ovrProjection_FarLessThanNear);
			}
			
			mat4 fm(m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3], m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3], m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
			glMultMatrix(fm);
#endif
#ifdef ENABLE_DK1
			if(eye==0)
			{
				OVR::Util::Render::StereoEyeParams ce = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Center);
				float mat[16];
				memset(mat, 0.f, sizeof(float)*16);
				OVR::Matrix4f m = (ce.Projection /** ce.ViewAdjust*/);
				mat4 fm(m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3], m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3], m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
				glMultMatrix(fm);
			}
			else if(eye==1)
			{
				OVR::Util::Render::StereoEyeParams le = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Left);
				float mat[16];
				memset(mat, 0.f, sizeof(float)*16);
				OVR::Matrix4f m = (le.Projection/* * le.ViewAdjust*/);
				mat4 fm(m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3], m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3], m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
				glMultMatrix(fm);
			}
			else if(eye==2)
			{
				OVR::Util::Render::StereoEyeParams re = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Right);
				float mat[16];
				memset(mat, 0.f, sizeof(float)*16);
				OVR::Matrix4f m = (re.Projection/* * re.ViewAdjust*/);
				mat4 fm(m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3], m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3], m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
				glMultMatrix(fm);
			}
#endif
#ifdef ENABLE_CV1
			ovrMatrix4f m;
			
			if (eye <= 1)
			{
				m = ovrMatrix4f_Projection(fionaConf.hmdDesc.DefaultEyeFov[0], fionaConf.nearClip, fionaConf.farClip, ovrProjection_None);// | ovrProjection_FarLessThanNear);
			}
			else
			{
				m = ovrMatrix4f_Projection(fionaConf.hmdDesc.DefaultEyeFov[1], fionaConf.nearClip, fionaConf.farClip, ovrProjection_None);// | ovrProjection_FarLessThanNear);
			}

			mat4 fm(m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3], m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3], m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
			glMultMatrix(fm);
#endif
#endif
		}
	}


	// Since the modelview matrix should be set to make the origin matches to the head position,
	//  (NOTE: one can set it to the eye position..)
	// it puts the origin back to the cave origin.

	if(fionaConf.appType != FionaConfig::OCULUS)
	{
		glTranslate(_ep);
	}
	
	// In model view matrix
	glMatrixMode(GL_MODELVIEW); 
	glPushMatrix(); 
	glLoadIdentity();

	// place the origin to the head position
	if(fionaConf.appType == FionaConfig::OCULUS)
	{
		glRotate(fionaConf.headRot.inv());
	}
	else
	{
		glTranslate(-_ep);
	}

	// rotate the world to align the screen to the x-y plane.
	if(fionaConf.appType != FionaConfig::OCULUS)
	{
		if( fionaConf.monitorView ) 
		{
			if(fionaConf.appType == FionaConfig::HEADNODE || fionaConf.appType == FionaConfig::WINDOWED)
			{
				glTranslate(-fionaConf.monitorStepBack*ZAXIS);
			}
		}
	
		//if(!fionaConf.desktopProjection)
		glRotate(wall.preRot);
		
		if( fionaConf.monitorView || fionaConf.wandView )
		{
			if(fionaConf.appType == FionaConfig::HEADNODE || fionaConf.appType == FionaConfig::WINDOWED)
			{
				//glRotatef(-15.0f, 1.f, 0.f, 0.f);
				glRotate(fionaConf.monitorCamOri.inv());
				glTranslate(-fionaConf.monitorCamPos);
			}
		}
	}
		
	if(fionaConf.appType == FionaConfig::OCULUS)
	{
		//for oculus, translate after the tracker & world rotation.
		//glTranslate(-_ep);
		glTranslate(-fionaConf.headPos);
	}

#ifndef BIG_OPENGL_OPT
	//majority of the below function calls really don't get used as most of the time
	//our scene class handles this functionality...
	//the layered stereo technique makes use of them...
	// Camera transformation
	if(!fionaConf.dualView)
	{
		glRotate(fionaConf.camRot.inv());
	}
	else
	{
		if(_FionaUTIsSingleViewMachine())
		{
			glRotate(fionaConf.camRot.inv());
		}
		else if(_FionaUTIsDualViewMachine())
		{
			glRotate(fionaConf.secondCamRot.inv());
		}
	}

	if(!fionaConf.dualView)
	{
		glTranslate(-fionaConf.camPos);
	}
	else
	{
		if(_FionaUTIsSingleViewMachine())
		{
			glTranslate(-fionaConf.camPos);
		}
		else if(_FionaUTIsDualViewMachine())
		{
			glTranslate(-fionaConf.secondCamPos);
		}
	}
#endif
}

#ifdef ENABLE_VIVE
#include <Kit3D/glslUtils.h>
#endif

// Buffer should be set earlier.
void renderSingle(int eye, int wini, const FionaWall& wall, const FionaViewport& vp, const jvec3& _ep, int w, int h)
{
	_FionaUTCalcMatrices( eye, wini,  wall, vp, _ep, w, h);

	// Finally call display function to render scene.
	if(fionaWinConf[wini].displayFunc) 
		fionaWinConf[wini].displayFunc();
	else 
		printf("Well, a window does not have display function\n");
	
	//this could be redundant but we don't know for sure whether the person adjusted the matrix mode inside their display func..
	glMatrixMode(GL_MODELVIEW); 
	glPopMatrix();
	glMatrixMode(GL_PROJECTION); 
	glPopMatrix();
	
#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
	fionaConf.eyeRenderTexture[eye == 2 ? 1 : 0]->UnsetRenderSurface();
	// Commit changes to the textures so they get picked up frame
	fionaConf.eyeRenderTexture[eye == 2 ? 1 : 0]->Commit();
#endif
#endif
#ifdef ENABLE_VIVE
	
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (eye == 2)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fionaConf.rightEyeDesc.m_nRenderFramebufferId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fionaConf.rightEyeDesc.m_nResolveFramebufferId);

		glBlitFramebuffer(0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	else
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fionaConf.leftEyeDesc.m_nRenderFramebufferId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fionaConf.leftEyeDesc.m_nResolveFramebufferId);

		glBlitFramebuffer(0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

#endif
	// Increase the render cycle count, such that one can see
	//   which eye, which wall, and which sub is being rendered.
	fionaRenderCycleCount++;
}


// This function draw FBO onto the screen
// For the wall and projector (sub) render the texture onto the screen
void renderFBO(int eye, int tex, int w, int h, const FionaViewport& vp)
{
	// OpenGL setting for texture rect rendering
	//   Viewport as in Vp
	int vpx = 0;
	int vpy = 0;
	int vpwidth = 0;
	int vpheight = 0;
	if(eye==0||eye==1)	
	{
		vpx = vp.lx*w;
		vpy = vp.ly*h;
		vpwidth = vp.lw*w;
		vpheight = vp.lh*h;
		glViewport(vpx,vpy,vpwidth,vpheight);
	}
	else				
	{
		vpx = vp.rx*w;
		vpy = vp.ry*h;
		vpwidth = vp.rw*w;
		vpheight = vp.rh*h;
		glViewport(vpx,vpy,vpwidth,vpheight);
	}
	
	//printf("***%d: %d, %d, %d, %d\n", eye, vpx, vpy, vpwidth, vpheight);

	glMatrixMode(GL_MODELVIEW); 
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glColor4f(1,1,1,1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#ifdef NEW_DISTORTION
	OVR::StereoEye eyeVal = OVR::StereoEye_Center;
	if(eye == 1)
	{
		eyeVal = OVR::StereoEye_Left;
	}
	else if(eye == 2)
	{
		eyeVal = OVR::StereoEye_Right;
	}

	/*uniformLensCenter=glGetUniformLocation(correctionProgram, "EdgeFadeScale");
	uniformScreenCenter=glGetUniformLocation(correctionProgram, "EyeToSourceNDCScale");
	uniformScreenCenterOffset=glGetUniformLocation(correctionProgram, "EyeToSourceNDCOffset");
	uniformScale=glGetUniformLocation(correctionProgram, "EyeToSourceUVScale");
	uniformScaleIn=glGetUniformLocation(correctionProgram, "EyeToSourceUVOffset");
	uniformHMDWarpParam=glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam=glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0=glGetUniformLocation(correctionProgram, "Texture");
	uniformDistortionClear = glGetUniformLocation(correctionProgram, "DistortionClearColor");
	uniformTanEyeAngleScale = glGetUniformLocation(correctionProgram, "TanEyeAngleScale");
	uniformTanEyeAngleOffset = glGetUniformLocation(correctionProgram, "TanEyeAngleOffset");
	uniformView = glGetUniformLocation(correctionProgram, "View");
	uniformTexM = glGetUniformLocation(correctionProgram, "Texm");
	*/
	
	fionaConf.stereoConfig.SetDirty();
	const OVR::StereoEyeParams &sep = fionaConf.stereoConfig.GetEyeRenderParams(eyeVal).StereoEye;
	const OVR::DistortionRenderDesc &d = sep.Distortion;
	   
	/*renderInfo.EyeLeft.Distortion.K[0]      =  1.0f;
    renderInfo.EyeLeft.Distortion.K[1]      = -0.494165344f;
    renderInfo.EyeLeft.Distortion.K[2]      = 0.587046423f;
    renderInfo.EyeLeft.Distortion.K[3]      = -0.841887126f;
    renderInfo.EyeLeft.Distortion.MaxR      = 1.0f;

    renderInfo.EyeLeft.Distortion.ChromaticAberration[0] = -0.006f;
    renderInfo.EyeLeft.Distortion.ChromaticAberration[1] =  0.0f;
    renderInfo.EyeLeft.Distortion.ChromaticAberration[2] =  0.014f;
    renderInfo.EyeLeft.Distortion.ChromaticAberration[3] =  0.0f;*/

	OVR::Vector2f localTanEyeAngleOffset = -d.LensCenter.EntrywiseMultiply(d.TanEyeAngleScale);
	//printf("%d, %d, %d, %d\n", sep.DistortionViewport.x, sep.DistortionViewport.y, sep.DistortionViewport.w, sep.DistortionViewport.h);
	glUseProgram(correctionProgram);
	glUniform1f(uniformLensCenter, 1.f / 0.075f);
	glUniform2f(uniformScreenCenter, sep.EyeToSourceNDC.Scale.x, sep.EyeToSourceNDC.Scale.y );
	glUniform2f(uniformScreenCenterOffset, sep.EyeToSourceNDC.Offset.x, sep.EyeToSourceNDC.Offset.y);
	glUniform2f(uniformScale, sep.EyeToSourceUV.Scale.x, sep.EyeToSourceUV.Scale.y);
	glUniform2f(uniformScaleIn, sep.EyeToSourceUV.Offset.x, sep.EyeToSourceUV.Offset.y);
	//printf("$$$$%f, %f, %f, %f\n", sep.EyeToSourceUV.Scale.x, sep.EyeToSourceUV.Scale.y, sep.EyeToSourceUV.Offset.x, sep.EyeToSourceUV.Offset.y);
	glUniform2f(uniformTanEyeAngleScale, d.TanEyeAngleScale.x, d.TanEyeAngleScale.y);
	glUniform2f(uniformTanEyeAngleOffset, localTanEyeAngleOffset.x, localTanEyeAngleOffset.y);
	glUniform3f(uniformDistortionClear, 0.f, 0.f, 0.f);
	//glUniform4f(uniformHMDWarpParam, 1.f, -0.49416, 0.58704, -0.841887);//d.Lens.K[0], d.Lens.K[1], d.Lens.K[2], d.Lens.K[3]);
	glUniform4f(uniformChromAbParam, -0.006f + 1.f, 0.0f, 0.014f + 1.f, 0.0f);////d.Lens.ChromaticAberration[0] + 1.f, d.Lens.ChromaticAberration[1], d.Lens.ChromaticAberration[2] + 1.f, d.Lens.ChromaticAberration[3]);
	glUniform1i(uniformTexture0, 0);
	//float m[16];
	//mat4 m;
	//m.loadIdentity();
	//glGetFloatv(GL_MODELVIEW_MATRIX, m.p);
	//glUniform4fv(uniformView, 1, m);
	//float t[16];
	//glGetFloatv(GL_TEXTURE_MATRIX, t);
	//glUniform4fv(uniformTexM, 1, m.p);
#else
	//sticking with old shader for dk2 until they fix stuff.. good lord...
	/*OVR::StereoEye eyeVal = OVR::StereoEye_Center;
	if(eye == 1)
	{
		eyeVal = OVR::StereoEye_Left;
	}
	else if(eye == 2)
	{
		eyeVal = OVR::StereoEye_Right;
	}

	fionaConf.stereoConfig.SetDirty();
	const OVR::StereoEyeParams &sep = fionaConf.stereoConfig.GetEyeRenderParams(eyeVal).StereoEye;
	const OVR::DistortionRenderDesc &d = sep.Distortion;*/
	   
	float shaderw = float(vpwidth) / float(w);
	float shaderh = float(vpheight) / float(h);
	float shaderx = float(vpx) / float(w);
	float shadery = float(vpy) / float(h);
	float as = float(vpwidth) / float(vpheight);

	//float XCenterOffset = 0.0379941;//d.XCenterOffset;
	//if( eye == 2 )
	//	XCenterOffset = -0.0379941;//d.XCenterOffset;
	//d.LensCenter.x = -0.00986
	//d.LensCenter.x right = 0.00986
	float lensCenterx = eye == 1 ? 0.25 + 0.00986 : 0.75 - 0.00986;//shaderx + (shaderw + XCenterOffset * 0.5f)*0.5f;//d.LensCenter.x;
	float lensCentery = shadery + shaderh*0.5f;//d.LensCenter.y;

	//printf("Old: %f, %f; New: %f, %f\n", lensCenterx, lensCentery, d.LensCenter.x, d.LensCenter.y);

	float scaleFactor = 1.0f / 1.3527f;//1.71f;// / d.Scale;

	glUseProgram(correctionProgram);
	glUniform2f(uniformLensCenter, lensCenterx, lensCentery);
	glUniform2f(uniformScreenCenter, shaderx + shaderw*0.5f, shadery + shaderh*0.5f);
	glUniform2f(uniformScale, (shaderw/2.f) * scaleFactor, (shaderh/2.f) * scaleFactor * as);
	glUniform2f(uniformScaleIn, 2.f/shaderw, (2.f/shaderh)/as);
	glUniform4f(uniformHMDWarpParam, 1.f, 0.22f, 0.24f, 0.0f);//d.K[0], d.K[1], d.K[2], d.K[3]);
	glUniform4f(uniformChromAbParam, 1.f, 0.f, 1.f, 0.f);//d.ChromaticAberration[0], d.ChromaticAberration[1], d.ChromaticAberration[2], d.ChromaticAberration[3]);
	glUniform1i(uniformTexture0, 0);
#endif
#endif
#ifdef ENABLE_DK1
	const OVR::Util::Render::DistortionConfig d = fionaConf.stereoConfig.GetDistortionConfig();
	
	float scaleFactor = 1.0f / d.Scale;
	float shaderw = float(vpwidth) / float(w);
	float shaderh = float(vpheight) / float(h);
	float shaderx = float(vpx) / float(w);
	float shadery = float(vpy) / float(h);
	float as = float(vpwidth) / float(vpheight);

	float XCenterOffset = d.XCenterOffset;
	if( eye == 2 )
		XCenterOffset = -d.XCenterOffset;
	float lensCenterx = shaderx + (shaderw + XCenterOffset * 0.5f)*0.5f;
	

	float lensCentery = shadery + shaderh*0.5f;
	float screenCenterx = shaderx + shaderw*0.5f;
	float screenCentery = shadery + shaderh*0.5f;

	float scalex = (shaderw/2.f) * scaleFactor;
	float scaley = (shaderh/2.f) * scaleFactor * as;
	float scaleinx = 2.f/shaderw;
	float scaleiny = (2.f/shaderh)/as;

	glUseProgram(correctionProgram);
	glUniform2f(uniformLensCenter, lensCenterx, lensCentery);
	glUniform2f(uniformScreenCenter, screenCenterx, screenCentery);
	glUniform2f(uniformScale, scalex, scaley);
	glUniform2f(uniformScaleIn, scaleinx, scaleiny);
	glUniform4f(uniformHMDWarpParam, d.K[0], d.K[1], d.K[2], d.K[3]);
	glUniform4f(uniformChromAbParam, d.ChromaticAberration[0], d.ChromaticAberration[1], d.ChromaticAberration[2], d.ChromaticAberration[3]);
	glUniform1i(uniformTexture0, 0);
#endif
#else
	glUseProgram(0);
#endif

	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();
	glMultMatrix(projectorCalibration(vp));

	drawRect(-1,1,-1,1,vp.sx,vp.sw+vp.sx,vp.sy,vp.sh+vp.sy);

#ifdef ENABLE_OCULUS
	glUseProgram(0);
#endif

	glEnable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	//	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo);
	//	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	//	glBlitFramebuffer(0,0,FBOWidth,FBOHeight,
	//					  0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
}

//use layered framebuffer technique to render both stereo eyes in single pass..
void renderBothEyes(int wini)
{
	GLint oldFrameBuf;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFrameBuf);

	int w = fionaWinConf[wini].winw, h = fionaWinConf[wini].winh;

	if(fionaConf.fbo == 0)
	{
		glGenTextures(1, &fionaConf.img);
		glBindTexture(GL_TEXTURE_2D_ARRAY, fionaConf.img);
#ifndef LINUX_BUILD
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, w, h, 2);
#endif
		
		glGenTextures(1, &fionaConf.depth);
		glBindTexture(GL_TEXTURE_2D_ARRAY, fionaConf.depth);
#ifndef LINUX_BUILD
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32F, w, h, 2);
#endif
		glGenFramebuffers(1, &fionaConf.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fionaConf.img, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fionaConf.depth, 0);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
	}

	static const GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, draw_buffers);

	if(!fionaConf.dontClear)
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}

	for( int i=0; i<(int)fionaWinConf[wini].walls.size(); i++ )
	{
		FionaWall& wall = fionaWinConf[wini].walls[i];
		fionaCurWall = i;

		jvec3 head = fionaConf.headPos;
		jvec3 eyePosLeft = fionaConf.kevinOffset+fionaConf.lEyeOffset;
		jvec3 eyePosRight = fionaConf.kevinOffset+fionaConf.rEyeOffset;

		//wall preRot is calculated in FionaWall constructor
		quat preRot = wall.preRot;
		//same with wall cntr
		//head is now in coordinate space relative to wall center
		head = preRot.rot(head)-wall.cntr;

		eyePosLeft = preRot.rot(fionaConf.headRot.rot(eyePosLeft))+head;
		eyePosRight = preRot.rot(fionaConf.headRot.rot(eyePosRight))+head;

		if(fionaConf.wallFunc)
		{
			fionaConf.wallFunc();
		}

		for( int k=0; k<(int)wall.viewports.size(); k++ )
		{
			FionaViewport& vp = wall.viewports[k];
			fionaCurViewport = k;

			glViewport(vp.lx*w,vp.ly*h,vp.lw*w,vp.lh*h);

			/*tran proj = viewportProjection(vp)*projectorCalibration(vp);
			glMatrixMode(GL_PROJECTION); 
			glPushMatrix(); 
			glLoadIdentity();
			glMultMatrix(proj*caveProjection(wall.sz,eyePosLeft));
			glTranslate(eyePosLeft);
			float rp[16];
			glGetFloatv(GL_PROJECTION_MATRIX, rp);
			fionaConf.projLeft = glm::make_mat4(rp);
			glLoadIdentity();
			glMultMatrix(proj*caveProjection(wall.sz,eyePosRight));
			glTranslate(eyePosRight);
			glGetFloatv(GL_PROJECTION_MATRIX, rp);
			fionaConf.projRight = glm::make_mat4(rp);*/

			tran leftEyeMVP = viewportProjection(vp)*projectorCalibration(vp)*caveProjection(wall.sz,eyePosLeft);
			tran rightEyeMVP = viewportProjection(vp)*projectorCalibration(vp)*caveProjection(wall.sz,eyePosRight);

			glm::mat4x4 eyeTransRightP(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, eyePosRight.x, eyePosRight.y, eyePosRight.z, 1.f);
			glm::mat4x4 eyeTransLeftP(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, eyePosLeft.x, eyePosLeft.y, eyePosLeft.z, 1.f);
			glm::mat4 vpProj,projCal,caveProjLeft,caveProjRight;
			viewportProjectionGLM(vp, vpProj);
			projectionCalibrationGLM(vp, projCal);
			caveProjectionGLM(glm::vec3(wall.sz.x, wall.sz.y, wall.sz.z), glm::vec3(eyePosLeft.x, eyePosLeft.y, eyePosLeft.z), caveProjLeft);
			caveProjectionGLM(glm::vec3(wall.sz.x, wall.sz.y, wall.sz.z), glm::vec3(eyePosRight.x, eyePosRight.y, eyePosRight.z), caveProjRight);

			fionaConf.projLeft = glm::transpose(eyeTransLeftP * vpProj * projCal * caveProjLeft); 
			fionaConf.projRight = glm::transpose(eyeTransRightP * vpProj * projCal * caveProjRight); 

			/*const float *m4p = glm::value_ptr(glm::transpose(m4));
			printf("************************\n");
			//values are transposed...
			for(int q = 0; q < 16; ++q)
			{
				printf("%f*\n", rp[q]);
				printf("%f\n", m4p[q]);
			}*/

			// In model view matrix
			glMatrixMode(GL_MODELVIEW); 
			glPushMatrix(); 
			glLoadIdentity();

			// place the origin to the head position
			/*glTranslatef(-eyePosLeft.x, -eyePosLeft.y, -eyePosLeft.z);
			glRotate(wall.preRot);
			glRotate(fionaConf.camRot.inv());
			glTranslate(-fionaConf.camPos);	

			float m[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, m);
			fionaConf.mvLeft = glm::make_mat4(m);

			glLoadIdentity();
			glTranslatef(-eyePosRight.x, -eyePosRight.y, -eyePosRight.z);
			glRotate(wall.preRot);
			glRotate(fionaConf.camRot.inv());
			glTranslate(-fionaConf.camPos);

			glGetFloatv(GL_MODELVIEW_MATRIX, m);
			fionaConf.mvRight = glm::make_mat4(m);*/

			// rotate the world to align the screen to the x-y plane.
			jvec3 v=ln(wall.preRot)*2; 
			real l=len(v); 
			v=v.unit();
			// Camera transformation
			quat cr = fionaConf.camRot.inv();
			jvec3 crv = ln(cr)*2.f;
			real cl = len(crv);
			crv=crv.unit();


			// Finally call display function to render scene.
			glm::mat4x4 eyeTransRight(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -eyePosRight.x, -eyePosRight.y, -eyePosRight.z, 1.f);
			glm::mat4x4 eyeTransLeft(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -eyePosLeft.x, -eyePosLeft.y, -eyePosLeft.z, 1.f);
			
			//glm::mat4x4 cam(orientation[0], orientation[1], orientation[2], position);
			/*glm::mat4x4 camTrans(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -fionaConf.camPos.x, -fionaConf.camPos.y, -fionaConf.camPos.z, 1.f);
			glm::mat4x4 camRotMat;
			if(crv.len() > 0.f)
			{
				camRotMat = glm::axisAngleMatrix(glm::vec3(crv.x, crv.y, crv.z), cl*180.f/PI);
			}
			else
			{
				camRotMat = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
			}*/

			glm::mat4x4 wallMat;
			if(v.len() > 0.f)
			{
				wallMat = glm::axisAngleMatrix(glm::vec3(v.x, v.y, v.z), l*180.f/PI);
			}
			else
			{
				wallMat = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
			}

			//glm::mat4x4 glmMV = camTrans * camRotMat * wallMat;

			glm::mat4x4 glmMV = fionaConf.mvLeft * wallMat;
			
			//fionaConf.mvLeft = glmMV * eyeTransLeft;
			//fionaConf.mvRight = glmMV * eyeTransRight;

			if(fionaWinConf[wini].displayFunc) 
				fionaWinConf[wini].displayFunc();
			else 
				printf("Well, a window does not have display function\n");
	
			glMatrixMode(GL_MODELVIEW); 
			glPopMatrix();

			//glMatrixMode(GL_PROJECTION); 
			//glPopMatrix();
	
			// Increase the render cycle count, such that one can see
			//   which eye, which wall, and which sub is being rendered.
			fionaRenderCycleCount++;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, oldFrameBuf);
}

// Draw walls image for an eye
// Notice that in FBO mode, a FBO rendered once per wall and eye,
//   So you don't have to draw a same scene from the same view point twice.
void renderEye(int eye,int wini,const jvec3& ep)
{
	int w = fionaWinConf[wini].winw, h = fionaWinConf[wini].winh;
	// Clearing buffer for Eye..
	if( !fionaConf.splitStereo )
	{
		if(fionaConf.stereo)
		{
			if( eye==1 )	
			{
				glDrawBuffer(GL_BACK_LEFT);
			}
			else if( eye==2 )	
			{
				if(fionaConf.appType != FionaConfig::TEST_STEREO)
				{
					glDrawBuffer(GL_BACK_RIGHT);
				}
			}
		}
		/*else
		{
			if( eye==1 )	
			{
				glDrawBuffer(GL_BACK_LEFT);
			}
			else if(eye == 0)
			{
				glDrawBuffer(GL_BACK_LEFT);
			}
		}*/
		if(!fionaConf.dontClear)
		{
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT/*|GL_STENCIL_BUFFER_BIT*/);
		}
	}
	else
	{
		if( eye==1 )	
		{
			if(!fionaConf.dontClear)
			{
#ifndef ENABLE_CV1
#ifndef ENABLE_VIVE
				//glClearColor(1.f, 0.f, 0.f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT/*|GL_STENCIL_BUFFER_BIT*/);
#endif
#endif
			}
		}
	}

	for( int i=0; i<(int)fionaWinConf[wini].walls.size(); i++ )
	{
		FionaWall& wall = fionaWinConf[wini].walls[i];
		fionaCurWall = i;
		
		//we can calculate eye pos here..
		//this can be calculated earlier...
		//since it is only dependent on the wall data not viewport...
		//headPos is the tracker position, plus sensor offset
		//headRot is the straight quat value from vrpn
		jvec3 head = fionaConf.headPos;
		if(fionaConf.dualView && _FionaUTIsDualViewMachine())
		{
			head = fionaConf.secondHeadPos;
		}
		
		jvec3 eyePos = ep;
		//wall preRot is calculated in FionaWall constructor
		quat preRot = wall.preRot;
		//same with wall cntr
		//head is now in coordinate space relative to wall center
		head = preRot.rot(head)-wall.cntr;

		//and now the eye
		if(fionaConf.dualView && _FionaUTIsDualViewMachine())
		{
			eyePos = preRot.rot(fionaConf.secondHeadRot.rot(eyePos))+head;
		}
		else
		{
			eyePos = preRot.rot(fionaConf.headRot.rot(eyePos))+head;
		}

		fionaCurEyePos = eyePos;
		
		if(fionaConf.wallFunc)
		{
			fionaConf.wallFunc();
		}

		// Texture ID corresponding to the FBO
		GLuint tex=fionaConf.img;

		if( fionaConf.useFBO )
		{
			if(fionaConf.multiGPU)
			{
				printf("shouldn't be here...\n");
			}
			// Prepare a FBO
			int fw=fionaConf.FBOWidth;
			int fh=fionaConf.FBOHeight;
			if(fionaConf.fbo==0 && !_FionaUTIsUsingExtFBO() ) 
			{
#ifdef ENABLE_OCULUS
#ifndef ENABLE_DK2	//dk2 makes the fbo in FionaUTConfig after window creation...
				createFBO(fw,fh);
#endif
				createOculusCorrection();
#else
				createFBO(fw, fh);
#endif
			}
			else
			{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
				static int once = 0;
				if(once == 0)
				{
					once = 1;
					createOculusCorrection();
				}
#endif
#endif
			}

			// Notifying the scene module that it is FBO mode
			//   So, the size of the viewport is different from that of the window
			isInFBO=true;
			
			// Check if the scene module is to draw its own FBO
			//   If it does, call render function to draw scene on its own FBO,
			//   and get Texture ID for the FBO
			//
			// NOTE: Voreen is the typical case of this.
			//       For voreen, it is recommended to use FBO mode,
			//       So that the number of rendering pass would be reduced by half.
			if( _FionaUTIsUsingExtFBO() )
			{
				// Rendering engine draw scene onto its own FBO (or texture)
				// this was sometimes used for voreen.. not much else
				renderSingle(eye,wini,wall,VP_FBO,eyePos,w,h);
				tex = _FionaUTGetExtFBOTexture();
			}
			// Otherwise setup render scene to the FBO
			else
			{
//#ifndef ENABLE_DK2	ENABLE_DIRECT_DK2
				glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
//#endif
				if(fionaConf.splitStereo)
				{
//#ifndef ENABLE_DK2	ENABLE_DIRECT_DK2
					if(eye==1)
					{
						if(!fionaConf.dontClear)
						{
							glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
						}
					}
//#endif
				}
				else
				{
					if(!fionaConf.dontClear)
					{
						glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
					}
				}
				//not convinced that this works for non-split stereo, non external fbo fbo drawing...
				//don't think we actually have an example of that yet...
				renderSingle(eye,wini,wall,VP_FBO,eyePos,fw,fh);
//#ifndef ENABLE_DK2	ENABLE_DIRECT_DK2
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
//#endif
			}
			isInFBO=false;
		}

		if(fionaConf.multiGPU)
		{
			static GLuint fbo2 = 0;
			static GLuint img2 = 0;
			static GLuint depth2 = 0;

			if(fionaCurWall > 0)
			{
				__FionaUTMakeCurrent(fionaWinConf[wini].window, true);

				//initialize and bind a framebuffer object here..
				if(fionaConf.fbo == 0)
				{
					//fionaConf.FBOWidth = 1920;	//defaults
					//fionaConf.FBOHeight = 1920;
					createFBO(fionaConf.FBOWidth, fionaConf.FBOHeight);
					glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
					glClearColor(0.f, 0.f, 0.f, 0.f);
				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
				}

				//draw eye into fbo..

				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				static const float Q = 1016.f/1920.f;
				static const FionaViewport BOTTOM(0,0,1,Q,0,0,1,Q);
				static const FionaViewport TOP(0,1-Q,1,Q,0,1-Q,1,Q);
				for( int k=0; k<(int)wall.viewports.size(); k++ )
				{
					fionaCurViewport = k;
					if(k == 0)
					{
						renderSingle(eye,wini,wall,BOTTOM,eyePos,fionaConf.FBOWidth,fionaConf.FBOHeight);
					}
					else
					{	
						renderSingle(eye,wini,wall,TOP,eyePos,fionaConf.FBOWidth,fionaConf.FBOHeight);
					}
				}
							
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifndef LINUX_BUILD
				GLboolean success = wglCopyImageSubDataNV(__FionaUTGetContext(fionaWinConf[0].window, true), fionaConf.img, GL_TEXTURE_2D, 0, 0,0,0,
					__FionaUTGetContext(fionaWinConf[0].window, false), img2, GL_TEXTURE_2D, 0, 0,0,0,
					1920, 1920, 1);
				
				if(success == GL_FALSE)
				{
					printf("wglCopyImageSubDataNV failed");
				}
#endif
				tex = img2;

				__FionaUTMakeCurrent(fionaWinConf[wini].window, false);
				//this makes it so subsequent draw draws the fbo to screen..
				fionaConf.useFBO = true;
			}
			else
			{

				if(fbo2 == 0)
				{
					//makeing this 1920x1920 allows us to map it correctly when drawing the fbo later...
					glGenFramebuffers(1, &fbo2);
					glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
	
					glGenTextures(1, &img2);
					glBindTexture(GL_TEXTURE_2D, img2);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1920, 1920, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	
					glGenTextures(1, &depth2);
					glBindTexture(GL_TEXTURE_2D, depth2);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1920, 1920, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img2, 0);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth2, 0);
	
					GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
				}

				fionaConf.useFBO = false;
			}
		}

		//why are we setting the buffer again here - for FBO rendering case...
#ifndef BIG_OPENGL_OPT
		/*if( eye==0 || fionaConf.splitStereo )
		{
			glDrawBuffer(GL_BACK);
		}
		else */
		if(fionaConf.stereo)
		{
			if( eye==1 )	
			{
				glDrawBuffer(GL_BACK_LEFT);
			}
			else if( eye==2 )
			{
				if(fionaConf.appType != FionaConfig::TEST_STEREO)
				{
					glDrawBuffer(GL_BACK_RIGHT);
				}
			}
		}
#endif
		for( int k=0; k<(int)wall.viewports.size(); k++ )
		{
			FionaViewport& vp = wall.viewports[k];
			fionaCurViewport = k;
			if( fionaConf.useFBO  )	
			{
				if(fionaConf.drawFBO)
				{
#ifdef ENABLE_DK2	//todo remove when direct mode working..
#ifndef ENABLE_DIRECT_DK2
				renderFBO(eye,tex,w,h,vp);
#endif
#else
				renderFBO(eye, tex, w, h, vp);
#endif
				}
			}
			else					
			{
				renderSingle(eye,wini,wall,vp,eyePos,w,h);
			}
		}

		if(fionaConf.multiGPU)
		{
			fionaConf.useFBO = false;
		}
	}
}

void	_FionaUTDisplay		(WIN win, CTX cntx)
{
	int i = _FionaUTFindWindow(win); 
	
	if( i<0 ) 
		return; 
	
	fionaCurWindow = i;
	
	fionaActiveWindow = win;
	
	fionaActiveContext=cntx;
	
	if( fionaGlobalMode & GLUT_MULTISAMPLE)
	{
		glEnable(GL_MULTISAMPLE);
	}

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
//#ifdef ENABLE_DIRECT_DK2
	//glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//ovrFrameTiming frameTiming = ovrHmd_BeginFrame(fionaConf.oculusHmd, 0);
//#else
//	ovrHmd_BeginFrameTiming(fionaConf.oculusHmd, 0);
//#endif
	if (fionaConf.pTextureSet != 0)
	{
		fionaConf.pTextureSet->CurrentIndex = (fionaConf.pTextureSet->CurrentIndex + 1) % fionaConf.pTextureSet->TextureCount;

		ovrGLTexture* tex = (ovrGLTexture*)&fionaConf.pTextureSet->Textures[fionaConf.pTextureSet->CurrentIndex];
		glBindTexture(GL_TEXTURE_2D, tex->OGL.TexId);
	}
#endif
#endif
	
	jvec3 ep = fionaConf.kevinOffset;

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#ifdef ENABLE_DIRECT_DK2
	ovrPosef eyeRenderPose[2];
#endif
#endif
#endif

	if( fionaConf.stereo || fionaConf.splitStereo )
	{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#ifdef ENABLE_DIRECT_DK2
		ovrEyeType eye = fionaConf.oculusHmd->EyeRenderOrder[0];
		eyeRenderPose[eye] = ovrHmd_GetEyePose(fionaConf.oculusHmd, eye);
#endif
#endif
#endif
		if(fionaConf.layeredStereo)
		{
			fionaRenderCycleLeft = true;
#ifndef LINUX_BUILD
			renderBothEyes(i);
#endif
		}
		else
		{
			fionaRenderCycleLeft = true;
			renderEye(1,i,ep+fionaConf.lEyeOffset);
			fionaRenderCycleLeft = false;
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#ifdef ENABLE_DIRECT_DK2
			eye = fionaConf.oculusHmd->EyeRenderOrder[1];
			eyeRenderPose[eye] = ovrHmd_GetEyePose(fionaConf.oculusHmd, eye);
#endif
#endif
#endif 
			fionaRenderCycleRight= true;
			renderEye(2,i,ep+fionaConf.rEyeOffset);
			fionaRenderCycleRight= false;	
		}
	}
	else
	{
		fionaRenderCycleLeft = true;
		renderEye(0,i,ep);
		fionaRenderCycleLeft = false;
	}
	
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#ifdef ENABLE_DIRECT_DK2
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	ovrHmd_EndFrame(fionaConf.oculusHmd, eyeRenderPose, &(fionaConf.oculusEyeTexture[0].Texture));
#else
	//ovrHmd_EndFrameTiming(fionaConf.oculusHmd);
	
	//**ovrLayerHeader* layers = &fionaConf.layer.Header;
	//**ovrResult       result = ovrHmd_SubmitFrame(fionaConf.oculusHmd, 0, nullptr, &layers, 1);
	//**bool isVisible = (result == ovrSuccess);
#endif
#endif
#ifdef ENABLE_CV1
	
	ovrLayerEyeFov ld;
	ld.Header.Type = ovrLayerType_EyeFov;
	ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

	for (int eye = 0; eye < 2; ++eye)
	{
		ld.ColorTexture[eye] = fionaConf.eyeRenderTexture[eye]->TextureChain;
		ld.Viewport[eye].Pos.x = 0;
		ld.Viewport[eye].Pos.y = 0;
		ld.Viewport[eye].Size = fionaConf.eyeRenderTexture[eye]->GetSize();
		ld.Fov[eye] = fionaConf.hmdDesc.DefaultEyeFov[eye];
		ld.RenderPose[eye] = fionaConf.eyeRenderPose[eye];
		ld.SensorSampleTime = fionaConf.sensorSampleTime;
	}
	//printf("%lld\n", fionaConf.frameIndex);
	ovrLayerHeader* layers = &ld.Header;
	ovrResult result = ovr_SubmitFrame(fionaConf.session, fionaConf.frameIndex, nullptr, &layers, 1);
	// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
	if (!OVR_SUCCESS(result))
		printf("Rendering error\n");

	bool isVisible = (result == ovrSuccess);

	//ovrSessionStatus sessionStatus;
	ovr_GetSessionStatus(fionaConf.session, &fionaConf.sessionStatus);
	if (fionaConf.sessionStatus.ShouldQuit)
		fionaDone = true;
	if (fionaConf.sessionStatus.ShouldRecenter)
		ovr_RecenterTrackingOrigin(fionaConf.session);

	//if (fionaConf.sessionStatus.HmdMounted && fionaConf.sessionStatus.HmdPresent)
	{
		// Blit mirror texture to back buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fionaConf.mirrorFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLint w = fionaWinConf[0].winw;
		GLint h = fionaWinConf[0].winh;
		glBlitFramebuffer(0, fionaConf.FBOHeight, fionaConf.FBOWidth, 0,
			0, 0, w, h,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	fionaConf.frameIndex++;
#endif
#endif

#ifdef ENABLE_VIVE
	if (fionaConf.m_pHMD)
	{
		//these three functions should happen in renderSingle...
		//DrawControllers();
		//RenderStereoTargets();
		//RenderDistortion();

		//render distortion...
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		static GLuint m_unLensVAO = 0;
		static GLuint m_unLensProgramID = 0;
		static GLuint m_uiIndexSize = 0;
		static GLuint m_glIDVertBuffer = 0;
		static GLuint m_glIDIndexBuffer = 0;
		static const char * sDistortionVert =
			"#version 410 core\n"
			"layout(location = 0) in vec2 position;\n"
			"layout(location = 1) in vec2 v2UVredIn;\n"
			"layout(location = 2) in vec2 v2UVGreenIn;\n"
			"layout(location = 3) in vec2 v2UVblueIn;\n"
			"noperspective  out vec2 v2UVred;\n"
			"noperspective  out vec2 v2UVgreen;\n"
			"noperspective  out vec2 v2UVblue;\n"
			"void main()\n"
			"{\n"
			"	v2UVred = v2UVredIn;\n"
			"	v2UVgreen = v2UVGreenIn;\n"
			"	v2UVblue = v2UVblueIn;\n"
			"	gl_Position = vec4(position, 0.0, 1.0);\n"
			"}\n";

		static const char *sDistortionFrag =
			// fragment shader
			"#version 410 core\n"
			"uniform sampler2D mytexture;\n"

			"noperspective  in vec2 v2UVred;\n"
			"noperspective  in vec2 v2UVgreen;\n"
			"noperspective  in vec2 v2UVblue;\n"

			"out vec4 outputColor;\n"

			"void main()\n"
			"{\n"
			"	float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );\n"
			"	if( fBoundsCheck > 1.0 )\n"
			"	{ outputColor = vec4( 0, 0, 0, 1.0 ); }\n"
			"	else\n"
			"	{\n"
			"		float red = texture(mytexture, v2UVred).x;\n"
			"		float green = texture(mytexture, v2UVgreen).y;\n"
			"		float blue = texture(mytexture, v2UVblue).z;\n"
			"		outputColor = vec4( red, green, blue, 1.0  );\n"
			"   }\n"
			"}\n";

		if (m_unLensProgramID == 0)
		{
			m_unLensProgramID = loadProgram(std::string(sDistortionVert), std::string(sDistortionFrag), true);

			glUseProgram(0);

			struct VertexDataLens
			{
				glm::vec2 position;
				glm::vec2 texCoordRed;
				glm::vec2 texCoordGreen;
				glm::vec2 texCoordBlue;
			};

			GLushort m_iLensGridSegmentCountH = 43;
			GLushort m_iLensGridSegmentCountV = 43;

			float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
			float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

			float u, v = 0;

			std::vector<VertexDataLens> vVerts(0);
			VertexDataLens vert;

			//left eye distortion verts
			float Xoffset = -1;
			for (int y = 0; y<m_iLensGridSegmentCountV; y++)
			{
				for (int x = 0; x<m_iLensGridSegmentCountH; x++)
				{
					u = x*w; v = 1 - y*h;
					vert.position = glm::vec2(Xoffset + u, -1 + 2 * y*h);

					vr::DistortionCoordinates_t dc0;
					fionaConf.m_pHMD->ComputeDistortion(vr::Eye_Left, u, v, &dc0);

					vert.texCoordRed = glm::vec2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
					vert.texCoordGreen = glm::vec2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
					vert.texCoordBlue = glm::vec2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

					vVerts.push_back(vert);
				}
			}

			//right eye distortion verts
			Xoffset = 0;
			for (int y = 0; y<m_iLensGridSegmentCountV; y++)
			{
				for (int x = 0; x<m_iLensGridSegmentCountH; x++)
				{
					u = x*w; v = 1 - y*h;
					vert.position = glm::vec2(Xoffset + u, -1 + 2 * y*h);

					vr::DistortionCoordinates_t dc0;
					fionaConf.m_pHMD->ComputeDistortion(vr::Eye_Right, u, v, &dc0);

					vert.texCoordRed = glm::vec2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
					vert.texCoordGreen = glm::vec2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
					vert.texCoordBlue = glm::vec2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

					vVerts.push_back(vert);
				}
			}

			std::vector<GLushort> vIndices;
			GLushort a, b, c, d;

			GLushort offset = 0;
			for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
			{
				for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
				{
					a = m_iLensGridSegmentCountH*y + x + offset;
					b = m_iLensGridSegmentCountH*y + x + 1 + offset;
					c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
					d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
					vIndices.push_back(a);
					vIndices.push_back(b);
					vIndices.push_back(c);

					vIndices.push_back(a);
					vIndices.push_back(c);
					vIndices.push_back(d);
				}
			}

			offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
			for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
			{
				for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
				{
					a = m_iLensGridSegmentCountH*y + x + offset;
					b = m_iLensGridSegmentCountH*y + x + 1 + offset;
					c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
					d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
					vIndices.push_back(a);
					vIndices.push_back(b);
					vIndices.push_back(c);

					vIndices.push_back(a);
					vIndices.push_back(c);
					vIndices.push_back(d);
				}
			}
			m_uiIndexSize = vIndices.size();

			glGenVertexArrays(1, &m_unLensVAO);
			glBindVertexArray(m_unLensVAO);

			glGenBuffers(1, &m_glIDVertBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
			glBufferData(GL_ARRAY_BUFFER, vVerts.size()*sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

			glGenBuffers(1, &m_glIDIndexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size()*sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

			glBindVertexArray(0);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(3);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		glViewport(0, 0, fionaConf.FBOWidth, fionaConf.FBOHeight);

		GLuint loc = glGetUniformLocation(m_unLensProgramID, "mytexture");
		glUseProgram(m_unLensProgramID);
		
		//render left lens (first half of index array )
		//glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, fionaConf.leftEyeDesc.m_nResolveTextureId);
		//glUniform1i(loc, 0);

		//glBindTexture(GL_TEXTURE_2D, fionaConf.leftEyeDesc.m_nResolveTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		glBindVertexArray(m_unLensVAO);
		glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);

		//glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, fionaConf.rightEyeDesc.m_nResolveTextureId);
		//glUniform1i(loc, 0);

		//render right lens (second half of index array )
		//glBindTexture(GL_TEXTURE_2D, fionaConf.rightEyeDesc.m_nResolveTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		glBindVertexArray(m_unLensVAO);
		glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));
		glBindVertexArray(0); 

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);

		vr::Texture_t leftEyeTexture = { (void*)fionaConf.leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)fionaConf.rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

		vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

		//this gets hmd, controller matrices, etc..
		vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
		
		glm::mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

		int m_iValidPoseCount = 0;
		std::string m_strPoseClasses = "";
		char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];

		for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
		{
			if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
			{
				m_iValidPoseCount++;
				glm::mat4 matrixObj(
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][0], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][0], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][0], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][1], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][1], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][1], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][2], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][2], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][2], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][3], 1.0f
					);
				//may have to transpose this...
				m_rmat4DevicePose[nDevice] = matrixObj;//ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
				if (m_rDevClassChar[nDevice] == 0)
				{
					switch (fionaConf.m_pHMD->GetTrackedDeviceClass(nDevice))
					{
						case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
						case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
						case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
						case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
						default:                                       m_rDevClassChar[nDevice] = '?'; break;
					}
				}

				if (m_rDevClassChar[nDevice] == 'C')
				{
					if (m_rTrackedDevicePose[vr::TrackedDeviceClass_Controller].bDeviceIsConnected && m_rTrackedDevicePose[vr::TrackedDeviceClass_Controller].bPoseIsValid)
					{
						glm::mat4 wandPose = glm::inverse(m_rmat4DevicePose[nDevice]);
						/*printf("%f, %f, %f, %f\n", wandPose[0][0], wandPose[0][1], wandPose[0][2], wandPose[0][3]);
						printf("%f, %f, %f, %f\n", wandPose[1][0], wandPose[1][1], wandPose[1][2], wandPose[1][3]);
						printf("%f, %f, %f, %f\n", wandPose[2][0], wandPose[2][1], wandPose[2][2], wandPose[2][3]);
						printf("%f, %f, %f, %f\n", wandPose[3][0], wandPose[3][1], wandPose[3][2], wandPose[3][3]);
						printf("******************************\n");*/
						fionaConf.wandPos = jvec3(wandPose[3][0], wandPose[3][1], wandPose[3][2]);
					}
				}

				m_strPoseClasses += m_rDevClassChar[nDevice];
			}
		}

		if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			fionaConf.hmdPose = glm::inverse(m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd]);// .invert();
		}
	}
#endif

	fionaActiveWindow = FIONA_INVALID_WIN;
	fionaActiveContext= FIONA_INVALID_CTX;

	if(fionaConf.displayGraphicsMem)
	{
		int vidMemAvail = __FionaUTGetVideoMemorySizeBytes(false, GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX);
		printf("Vid Memory Available 1st card: %d MB\n\n", vidMemAvail / 1024);

		if(fionaConf.multiGPU && _FionaUTIsCAVEMachine())
		{
			__FionaUTMakeCurrent(fionaWinConf[0].window, true);
			vidMemAvail = __FionaUTGetVideoMemorySizeBytes(false, GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX);
			printf("Vid Memory Available 2nd card: %d MB\n\n", vidMemAvail / 1024);
			__FionaUTMakeCurrent(fionaWinConf[0].window, false);
		}
		//fionaConf.displayGraphicsMem=false;
	}
}

void _FionaUTCleanupRendering(void)
{
	if(fionaConf.useFBO || fionaConf.layeredStereo)
	{
		glDeleteFramebuffers(1, &fionaConf.fbo);
		glDeleteTextures(1, &fionaConf.depth);
		glDeleteTextures(1, &fionaConf.img);
#ifdef ENABLE_OCULUS
		glDeleteBuffers(1, &m_quadVertexBuffer);
		glDeleteBuffers(1, &m_quadLeftUVBuffer);
		glDeleteBuffers(1, &m_quadRightUVBuffer);
		glDeleteProgram(correctionProgram);
#endif		
	}
}

bool	_FionaUTIsInFBO			(void)		{ return isInFBO; }
int		_FionaUTGetFBOWidth		(void)		{ return fionaConf.FBOWidth; }
int		_FionaUTGetFBOHeight	(void)		{ return fionaConf.FBOHeight; }
int		_FionaUTIsUsingExtFBO	(void)		{ return isUsingExtFBO; }
void	_FionaUTUseExtFBO		(bool v)	{ isUsingExtFBO = v; }
void	_FionaUTSetExtFBOTexture(GLuint tex){ extFBOTexture=tex; }
GLuint	_FionaUTGetExtFBOTexture(void)		{ return extFBOTexture; }
