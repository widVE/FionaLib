//
//  glUtils.h
//  NewViewer
//
//  Created by Hyun Joon Shin on 10/10/11.
//  Copyright 2011 VCLab, Ajou University. All rights reserved.
//


#ifndef __J_GL_UTILS__
#define __J_GL_UTILS__

#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined WIN32
#include <GL/gl.h>
#endif
#include "jmath.h"
#include "jtrans.h"


//typedef unsigned int ICOL; // color in unsigned integer formar ARGB888
#define NOCOLOR 0xFEF8F6F6
#define GETARRAY(X) float X[4]={r()/255.0f,g()/255.0f,b()/255.0f,a()/255.0f};

#ifndef uint
typedef unsigned int uint;
#endif

struct ICOL
{
	unsigned int c;
	inline ICOL(uint C=NOCOLOR): c(C){}
	inline ICOL(uint R, uint G, uint B, uint A){c=(A<<24)|(R<<16)|(G<<8)|B;}
	inline operator uint () const { return c; }
	inline uint a()const { return (c>>24)&0xFF; }
	inline uint r()const { return (c>>16)&0xFF; }
	inline uint g()const { return (c>>8)&0xFF; }
	inline uint b()const { return c&0xFF; }
	
	inline void glLightAmbient(GLenum l=GL_LIGHT0) const
	{ GETARRAY(f); glLightfv(l, GL_AMBIENT, f); }
	inline void glLightDiffuse(GLenum l=GL_LIGHT0) const
	{ GETARRAY(f); glLightfv(l, GL_DIFFUSE, f); }	
	inline void glLightSpecular(GLenum l=GL_LIGHT0) const
	{ GETARRAY(f); glLightfv(l, GL_SPECULAR, f); }
	inline void glMatDiffuse  (GLenum x=GL_FRONT_AND_BACK) const
	{ GETARRAY(f); glMaterialfv(x, GL_DIFFUSE, f); }
	inline void glMatAmbient  (GLenum x=GL_FRONT_AND_BACK) const
	{ GETARRAY(f); glMaterialfv(x, GL_AMBIENT, f); }
	inline void glMatSpecular (GLenum x=GL_FRONT_AND_BACK) const
	{ GETARRAY(f); glMaterialfv(x, GL_SPECULAR, f); }
	inline void glClearColor  (void) const
	{ GETARRAY(f); ::glClearColor(f[0],f[1],f[2],f[3]); }
	inline void glColor       (void) const
	{ glColor4b(r(), g(), b(), a()); }
	inline ICOL operator*(float x)const
	{return ICOL(r()*x,g()*x,b()*x,a()*x);}
};
const ICOL WHITECOL(0xFFFFFFFF);
const ICOL BLACKCOL(0xFF000000);
const ICOL TRANSCOL(0x00000000);

inline void glClearColor   (ICOL c)                        {c.glClearColor();}
inline void glLightAmbient (ICOL c,int l=GL_LIGHT0)        {c.glLightAmbient(l);}
inline void glLightDiffuse (ICOL c,int l=GL_LIGHT0)        {c.glLightDiffuse(l);}
inline void glLightSpecular(ICOL c,int l=GL_LIGHT0)        {c.glLightSpecular(l);}
inline void glMatAmbient   (ICOL c,int l=GL_FRONT_AND_BACK){c.glMatAmbient(l);}
inline void glMatDiffuse   (ICOL c,int l=GL_FRONT_AND_BACK){c.glMatDiffuse(l);}
inline void glMatSpecular  (ICOL c,int l=GL_FRONT_AND_BACK){c.glMatSpecular(l);}
inline void glColor        (ICOL c)                        {c.glColor();}
inline void glMat  (ICOL d, ICOL s=NOCOLOR, ICOL a=NOCOLOR, float spc=100)
{
	d.glMatDiffuse();
	(s==NOCOLOR?ICOL(WHITECOL):s).glMatSpecular();
	(a==NOCOLOR?d*0.3f:a).glMatAmbient();
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, spc);
}
inline void glMat  (float r, float g, float b, float a)
{
	glMat(ICOL(r*255, g*255, b*255, a*255));
}
inline void glColorC(ICOL c){ glMat(ICOL(0,0,0,c.a()),TRANSCOL,c); }
inline ICOL PCOLOR( float h, uint a=255 )
{
	uint r, g, b;
	if( h>1 ) h = 1;
	if( h<0 ) h = 0;
	int i = (int)(h*8);
	float m1 = (h*8.0f-i)*0.5f, m2 = m1+0.5f, n1 = 1-m1, n2 = n1-0.5f;
	switch (i) {
		case 0: r=m2*255+.5f, g=0,          b=0;          break;
		case 1: r=255,        g=m1*255+.5f, b=0;          break;
		case 2: r=255,        g=m2*255+.5f, b=0;          break;
		case 3: r=n1*255+.5f, g=255,        b=m1*255+.5f; break;
		case 4: r=n2*255+.5f, g=255,        b=m2*255+.5f; break;
		case 5: r=0,          g=n1*255+.5f, b=255;        break;
		case 6: r=0,          g=n2*255+.5f, b=255;        break;
		case 7: r=0,          g=0,          b=n1*255+.5f; break;
		case 8: r=0,          g=0,          b=128;        break;
	}
	return (a<<24)|(r<<16)|(g<<8)|(b);
}






#include <string>
#include <fstream>
#include <vector>

extern GLuint loadTextureFromFile(const std::string & pFilePath);


struct TexLib
{
	std::vector<GLuint>			glTexId;
	std::string					modelPath;
	std::vector<std::string>	texFile;

	void setMainFilePath(const std::string& _modelPath )
	{
		modelPath = _modelPath;
		modelPath.resize(modelPath.rfind('/')+1);
	}
	int find( const std::string& path )
	{
		for( int i=0; i<(int)texFile.size(); i++ )
			if( path.compare(texFile[i])==0 ) return i;
		return -1;
	}
	int add(const std::string& texFn )
	{
		std::ifstream ifs;
		std::string path = texFn;
		ifs.open(path.c_str());
		if( ifs.is_open() )
		{
			ifs.close();
			int idx = find(path);
			if( idx<0 )
			{
				texFile.push_back(path);
				glTexId.push_back(0);
				idx = (int)glTexId.size()-1;
			}
			return idx;
		}
		path = modelPath;
		path+=texFn;
		ifs.open(path.c_str());
		if( ifs.is_open() )
		{
			ifs.close();
			int idx = find(path);
			if( idx<0 )
			{
				texFile.push_back(path);
				glTexId.push_back(0);
				idx = find(path);
			}
			return idx;
		}
		std::cerr<<"Texture cannot be found: "<<path<<std::endl;
		return -1;
	}

	bool isInitialized(int i) const { return glTexId[i]!=0; }
	GLuint init(int i) { return getGLTex(i); }
	GLuint getGLTex(int i)
	{
		if( glTexId[i]!=0 ) return glTexId[i];
		return glTexId[i] = loadTextureFromFile(texFile[i]);
	}
};




struct MAT
{
	ICOL	d, s, a;
	real	e;
	TexLib*	texLib;
	int		tex;
	
	MAT(void):d(WHITECOL),s(0),a(d*.3f),e(1000),tex(-1),texLib(NULL){}
	MAT(ICOL D,ICOL S,ICOL A,real n):d(D),s(S),a(A),e(n),tex(-1),texLib(NULL){};

	void setTex(TexLib* lib, int texId, const std::string& texName="" )
	{	texLib = lib;	tex = texId; }
	void glSet(void) const
	{
		if( texLib!=NULL && tex>=0 )
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texLib->getGLTex(tex));
		}
		else glDisable(GL_TEXTURE_2D);
		glMat(d,s,a,e);
	}
	static void setDefault(void)
	{
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glMat(0xFFFFFFFF,0,0xFF404040,0);
	}
};

inline void glLight    (jvec3 p, int l=GL_LIGHT0, ICOL a=0xFF000000,
						bool enable=true, ICOL d=0xFFFFFFFF, ICOL s=0xFFFFFFFF)
{
	if(enable)
	{
		glEnable(GL_LIGHTING);
		glEnable(l);
	}
	glLightPos(vec4(p,0),l);
	glLightAmbient(a,l);
	glLightDiffuse(d,l);
	glLightSpecular(d,l);
}

inline void glLight    (vec4 p, int l=GL_LIGHT0, ICOL a=0xFF000000,
						bool enable=true, ICOL d=0xFFFFFFFF, ICOL s=0xFFFFFFFF)
{
	if(enable)
	{
		glEnable(GL_LIGHTING);
		glEnable(l);
	}
	glLightPos(p,l);
	glLightAmbient(a,l);
	glLightDiffuse(d,l);
	glLightSpecular(d,l);
}

extern void glGrid     (float hw=20, int step=80);
extern void glFloor    (float hw=20, int step=80, ICOL bCol=0xCCB2B2B2, ICOL dCol=0xCC666666);
extern void glCrossHair(const jvec3& p,float s=1);
extern void glXMarker  (const jvec3& p,float s=1, int i=0);
extern void glCone     (const jvec3& p,float r,const jvec3& dir,int sec=36);
extern void glCylinder (const jvec3& p,float r,const jvec3& dir,int sec=36);
extern void glArrow    (const jvec3& p,float r,const jvec3& dir,int sec=36);
extern void glSphere   (const jvec3& p,float r,const jvec3& dir=jvec3(),int sec=36);



inline void glGrid(float hw, int step)
{
	const int bigstep = 10;
	for(int i = -step; i <= step; i++) {
		if( (i%bigstep) == 0)	glLineWidth(2.0f);
		else				glLineWidth(1.5f);
		float xx = hw*i/(float)step;
		glBegin(GL_LINES);
		glVertex3f(xx,0,-hw); glVertex3f(xx,0,hw);
		glVertex3f(-hw,0,xx); glVertex3f(hw,0,xx);
		glEnd();
    }
}

inline void glFloor(float hw, int step, ICOL bCol, ICOL dCol)
{
	float delta = hw/(float)step;
	glBegin(GL_QUADS);
	glNormal(YAXIS);
	for(int z=-step; z<step; z++)
	{
		float zz1 = delta*z, zz2=zz1+delta;
		for(int x=-step; x<=step; x++)
		{
			if( (x+z)%2 )	glMat(bCol);
			else			glMat(dCol);
			float xx1 = delta*x, xx2=xx1+delta;
			glVertex3f(xx1,0,zz1);
			glVertex3f(xx1,0,zz2);
			glVertex3f(xx2,0,zz2);
			glVertex3f(xx2,0,zz1);
		}
    }
	glEnd();
}

inline void glCrossHair(const jvec3& p, float scale)
{
	glPushMatrix(); glTranslate(p); glScalef(scale,scale,scale);
	float coords[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
	glBegin(GL_LINES);
	for( int i=0; i<6; i++ ) glVertex3fv(coords[i]);
	glEnd();
	glPopMatrix();
}


inline void glXMarker( const jvec3& p, float s, int ignoreDepth )
{
	if( ignoreDepth ) glDepthMask( GL_FALSE );
	glPushMatrix(); glTranslate(p+jvec3(0,0.01f,0)); glScalef(s/2,s/2,s/2);
	glBegin( GL_TRIANGLES );
	float coords[12][3]={{0,0,0},{-.5f,0,.866f,},{.5f,0,.866f,},
		{0,0,0},{ .5f,0,-.866f,},{-.5f,0,-.866f,},
		{0,0,0},{-.866f,0,-.5f,},{-.866f,0, .5f,},
		{0,0,0},{ .866f,0, .5f,},{ .866f,0,-.5f,}};
	for( int i=0; i<12; i++ ) glVertex3fv(coords[i]);
	glEnd();
	if( ignoreDepth ) glDepthMask( GL_TRUE );
	glPopMatrix();
}

inline void __glCircle(int yup, int sec)
{
	glBegin(GL_TRIANGLE_FAN);
	if( yup )
	{
		glNormal(YAXIS);	
		glVertex3f(0,0,0);
		for( int i=0; i<=sec; i++ )
		{
			float theta = -i*PI/sec*2;
			glVertex3f(cosf(theta),0,sinf(theta));
		}
		glEnd();
	}
	else
	{
		glNormal(-YAXIS);
		glVertex3f(0,0,0);
		for( int i=0; i<=sec; i++ )
		{
			float theta = i*PI/sec*2;
			glVertex3f(cosf(theta),0,sinf(theta));
		}
		glEnd();
	}
}

inline void __glConeWall(int sec)
{
	float sq2 = 1.414213f/2;
	glBegin(GL_TRIANGLES);
	for( int i=0; i<sec; i++ )
	{
		float th1 = -i*PI/sec*2;
		float th2 = -(i+1)*PI/sec*2;
		glNormal3f(sq2*cosf((th1+th2)/2),sq2,sq2*sinf((th1+th2)/2));
		glVertex3f(0,1,0);
		glNormal3f(sq2*cosf((th1)),sq2,sq2*sinf((th1)));
		glVertex3f(cosf(th1),0,sinf(th1));
		glNormal3f(sq2*cosf((th2)),sq2,sq2*sinf((th2)));
		glVertex3f(cosf(th2),0,sinf(th2));
	}
	glEnd();
}

inline void __glCylinderWall(int sec)
{
	float halfRoot2 = 1.414213f/2;
	glBegin(GL_TRIANGLE_STRIP);
	for( int i=0; i<=sec; i++ )
	{
		float th1 = i*PI/sec*2;
		glNormal3f(halfRoot2*cosf((th1)),halfRoot2,halfRoot2*sinf((th1)));
		glVertex3f(cosf(th1),0,sinf(th1));
		glVertex3f(cosf(th1),1,sinf(th1));
	}
	glEnd();
}

inline void glCone( const jvec3& p1, float r, const jvec3& dir, int sec )
{
	jvec3 up(dir);
	float h = len(up);
	glPushMatrix();
	glTranslate(p1);
	glRotate(quat(YAXIS, unit(up)));
	glScalef(r,h,r);	
	{
		__glCircle(0,sec);
		__glConeWall(sec);
	}
	glPopMatrix();
}

inline void glCylinder( const jvec3& p1, float r, const jvec3& dir, int sec )
{
	jvec3 up(dir);
	float h = len(up);
	glPushMatrix();
	glTranslate(p1);
	glRotate(quat(YAXIS, unit(up)));
	glScalef(r,h,r);	
	{
		__glCircle(0,sec);
		__glCylinderWall(sec);
		glTranslatef(0,1,0);
		__glCircle(1,sec);
	}
	glPopMatrix();
}

inline void glArrow( const jvec3& p1, float r, const jvec3& dir, int sec )
{
	jvec3 up(dir);
	float h = len(up);
	float arrowHeadHeight = r*1.5f;
	float cylinderRadius = r*.6f;
	float cylinderHeight = h-r*1.5f;
	if( cylinderHeight<0 )
	{
		arrowHeadHeight = h*.7f;
		cylinderHeight=h*.3f;
	}
	
	glPushMatrix();
	glTranslate(p1);
	glRotate(quat(YAXIS, normalize(up)));
	glPushMatrix();
	glScalef(cylinderRadius,cylinderHeight,cylinderRadius);
	__glCircle(0,sec);
	__glCylinderWall(sec);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0,cylinderHeight,0);
	glScalef(r,arrowHeadHeight,r);
	__glCircle(0,sec);
	__glConeWall(sec);
	glPopMatrix();
	glPopMatrix();
}


inline void glSphere(const jvec3& p, float r, const jvec3& dir, int step )
{
	float yr = dir.sqlen()<0.000001f?r:dir.len();
	glPushMatrix();
	glTranslate(p);
	if( dir.sqlen()>0.000001f ) glRotate(quat(YAXIS,dir.unit()));
	glScalef(r,yr,r);
	for( int i=0; i<step/2; i++ )
	{
		glBegin(GL_TRIANGLE_STRIP);
		float ph1 = i*2.0f/step*PI;
		float ph2 = ph1+2.0f/step*PI;
		for( int j=0; j<=step; j++ )
		{
			float th = j*2*PI/step;
			jvec3 v1(cosf(th)*sinf(ph1),cosf(ph1),sinf(th)*sinf(ph1));
			jvec3 v2(cosf(th)*sinf(ph2),cosf(ph2),sinf(th)*sinf(ph2));
			glNormal(v2); glVertex(v2);
			glNormal(v1); glVertex(v1);
		}
		glEnd();
	}
	glPopMatrix();
}

inline GLuint __createTexture(void)
{
	GLuint texId;
	glGenTextures(1, &texId);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	return texId;
}

#endif

