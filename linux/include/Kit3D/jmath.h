//
//  jmath.h
//  SkinTest
//
//  Created by Hyun Joon Shin on 9/15/11.
//  Copyright 2011 VCLab, Ajou University. All rights reserved.
//


#ifndef __JMATH_H__
#define __JMATH_H__

#include <math.h>
#include <sstream>
#include <iostream>


#ifndef MIN
#define MIN(X,Y) ((X)>(Y)?(Y):(X))
#define MAX(X,Y) ((X)<(Y)?(Y):(X))
#endif
#ifndef ABS
#define ABS(X) ((X)>=0?(X):-(X))
#endif

typedef float real;
const real PI=3.14159265358979f;
const real EPSILON=0.0000001f;

//************************************************************************
//
//    Three vector class
//    which is used to specify position and displacement in 3D
//
//************************************************************************

struct jvec3;
struct vec3i;
struct quat;

typedef jvec3& rfv3;
typedef vec3i& rfv3i;
typedef quat& rfqt;
typedef const jvec3& crv3;
typedef const vec3i& crv3i;
typedef const quat& crqt;

struct jvec3
{
	union {
		struct { real x, y, z; };
		real p[3];
	};
	inline jvec3(void)					{ zero(); }
	inline jvec3(real a,real b,real c)	{ set(a,b,c); }
	inline jvec3(const real a[])			{ set(a); }
	inline jvec3(const double a[])		{ set((real)a[0],(real)a[1],(real)a[2]); }
	inline jvec3(crv3 a)					{ x=a.x; y=a.y; z=a.z; }
	
	inline operator real*(void)			{ return p; }
	inline real& operator[](int i)		{ return p[i]; }
	
	inline void set(const real v[])		{ x=v[0]; y=v[1]; z=v[2]; }
	inline void set(real a,real b, real c) { x=a, y=b, z=c; }
	inline void set(crqt a,crqt b);
	inline void get(real v[]) const		{ v[0]=x,v[1]=y,v[2]=z; }
	inline void zero(void)				{ x=y=z=0; }
	
	inline jvec3 operator- (void)const	{ return jvec3(-x,-y,-z); }
	
	inline rfv3 operator+=(crv3 a)		{ x+=a.x, y+=a.y; z+=a.z; return *this;}
	inline rfv3 operator-=(crv3 a)		{ x-=a.x, y-=a.y; z-=a.z; return *this;}
	inline rfv3 operator*=(crv3 a)		{ return *this=jvec3(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); }
	inline rfv3 operator*=(real a)		{ x*=a; y*=a; z*=a; return *this; }
	inline rfv3 operator/=(real a)		{ x/=a; y/=a; z/=a; return *this; }
	
	inline jvec3	operator+ (crv3 a)const { return jvec3(x+a.x,y+a.y,z+a.z); }
	inline jvec3	operator- (crv3 a)const { return jvec3(x-a.x,y-a.y,z-a.z); }
	inline jvec3	operator* (crv3 a)const	{ return jvec3(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); }
	inline real	operator% (crv3 a)const { return x*a.x+y*a.y+z*a.z; }
	inline jvec3	operator* (real a)const { return jvec3(x*a,y*a,z*a); }
	inline jvec3	operator/ (real a)const { return jvec3(x/a,y/a,z/a); }
	friend inline jvec3	operator*(real a, crv3 v) { return v*a; }
	
	inline real sqlen(void)const		{ return x*x+y*y+z*z; }
	inline real len  (void)const		{ return sqrtf(sqlen()); }
	inline jvec3 unit (void)const
	{	real l=sqlen();
		if(l<EPSILON) return jvec3(0,0,0);
		if(l<1+EPSILON&&l>1-EPSILON) return *this;
		return *this/sqrtf(l);
	}
	inline rfv3 normalize(void)			{ return *this=unit(); }
	friend inline real len  (crv3 a)	{ return a.len(); }
	friend inline real sqlen(crv3 a)	{ return a.sqlen(); }
	friend inline jvec3 unit (crv3 a)	{ return a.unit(); }
	friend inline jvec3 normalize(crv3 a){ return a.unit(); }
	friend inline jvec3 delDOF(crv3 v, crv3 uax){ return v-(v%uax)*uax; }
	
	friend inline std::ostream& operator <<(std::ostream& s,crv3 a)
	{ s<<a.x<<", "<<a.y<<", "<<a.z<<" "; return s; }
	friend inline std::istream& operator >>(std::istream& s,rfv3 a)
	{ s>>a.x>>a.y>>a.z; return s; }
	inline std::string toString(void)const
	{ std::stringstream s; s<<*this; return s.str(); }
	friend inline std::string toString(crv3 a) { return a.toString(); }
	friend inline jvec3 toVec3(std::string str)
	{ jvec3 v; std::stringstream s(str); s>>v; return v; }
#ifdef __Vector3_H__
#define JMATH_OGRE_WRAPPER_
	inline jvec3(const Ogre::Vector3& v): x(v.x),y(v.y),z(v.z){}
	inline operator const Ogre::Vector3(void) const { return Ogre::Vector3(x,y,z); }
#endif
};

const jvec3 XAXIS(1,0,0);
const jvec3 YAXIS(0,1,0);
const jvec3 ZAXIS(0,0,1);
const jvec3 VEC3X(1,0,0);
const jvec3 VEC3Y(0,1,0);
const jvec3 VEC3Z(0,0,1);
const jvec3 V3ID(0,0,0);
const jvec3 V3_0(0,0,0);
const jvec3 V3_1(1,1,1);

struct vec3i
{
	union {
		struct { int x, y, z; };
		int p[3];
	};
	inline vec3i(void)					{ zero(); }
	inline vec3i(int a,int b,int c)	{ set(a,b,c); }
	inline vec3i(const int a[])			{ set(a); }
	inline vec3i(const double a[])		{ set((int)a[0],(int)a[1],(int)a[2]); }
	inline vec3i(crv3i a)					{ x=a.x; y=a.y; z=a.z; }
	
	inline operator int*(void)			{ return p; }
	inline int& operator[](int i)		{ return p[i]; }
	
	inline void set(const int v[])		{ x=v[0]; y=v[1]; z=v[2]; }
	inline void set(int a,int b, int c) { x=a, y=b, z=c; }
	inline void set(crqt a,crqt b);
	inline void get(int v[]) const		{ v[0]=x,v[1]=y,v[2]=z; }
	inline void zero(void)				{ x=y=z=0; }
	
	inline vec3i operator- (void)const	{ return vec3i(-x,-y,-z); }
	
	inline rfv3i operator+=(crv3i a)		{ x+=a.x, y+=a.y; z+=a.z; return *this;}
	inline rfv3i operator-=(crv3i a)		{ x-=a.x, y-=a.y; z-=a.z; return *this;}
	inline rfv3i operator*=(crv3i a)		{ return *this=vec3i(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); }
	inline rfv3i operator*=(int a)		{ x*=a; y*=a; z*=a; return *this; }
	inline rfv3i operator/=(int a)		{ x/=a; y/=a; z/=a; return *this; }
	
	inline vec3i	operator+ (crv3i a)const { return vec3i(x+a.x,y+a.y,z+a.z); }
	inline vec3i	operator- (crv3i a)const { return vec3i(x-a.x,y-a.y,z-a.z); }
	inline vec3i	operator* (crv3i a)const	{ return vec3i(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); }
	inline int	operator% (crv3i a)const { return x*a.x+y*a.y+z*a.z; }
	inline vec3i	operator* (int a)const { return vec3i(x*a,y*a,z*a); }
	inline vec3i	operator/ (int a)const { return vec3i(x/a,y/a,z/a); }
	friend inline vec3i	operator*(int a, crv3i v) { return v*a; }
	
	inline int sqlen(void)const		{ return x*x+y*y+z*z; }
	inline int len  (void)const		{ return (int)sqrtf(sqlen()); }
	inline vec3i unit (void)const
	{	int l=sqlen();
		if(l<EPSILON) return vec3i(0,0,0);
		if(l<1+EPSILON&&l>1-EPSILON) return *this;
		return *this/(int)sqrtf(l);
	}
	inline rfv3i normalize(void)		{ return *this=unit(); }
	friend inline int len  (crv3i a)	{ return a.len(); }
	friend inline int sqlen(crv3i a)	{ return a.sqlen(); }
	friend inline vec3i unit (crv3i a)	{ return a.unit(); }
	friend inline vec3i normalize(crv3i a){ return a.unit(); }
	friend inline vec3i delDOF(crv3i v, crv3i uax){ return v-(v%uax)*uax; }
	
	friend inline std::ostream& operator <<(std::ostream& s,crv3i a)
	{ s<<a.x<<", "<<a.y<<", "<<a.z<<" "; return s; }
	friend inline std::istream& operator >>(std::istream& s,rfv3i a)
	{ s>>a.x>>a.y>>a.z; return s; }
	inline std::string toString(void)const
	{ std::stringstream s; s<<*this; return s.str(); }
	friend inline std::string toString(crv3i a) { return a.toString(); }
	friend inline vec3i toVec3i(std::string str)
	{ vec3i v; std::stringstream s(str); s>>v; return v; }
#ifdef __Vector3_H__
#define JMATH_OGRE_WRAPPER_
	inline vec3i(const Ogre::Vector3& v): x(v.x),y(v.y),z(v.z){}
	inline operator const Ogre::Vector3(void) const { return Ogre::Vector3(x,y,z); }
#endif
};

//************************************************************************
//
//    Quaternion class
//    which is used to specify position and displacement in 3D
//
//************************************************************************

struct quat
{
	union {
		struct { real w, x, y, z; };
		struct { real r, v[3]; };
		real p[4];
	};
	inline quat(void)					{ clear(); }
	inline quat(real a,real b,real c,real d){ set(a,b,c,d); }
	inline quat(const real a[])			{ set(a); }
	inline quat(const double a[])		{ set((real)a[0],(real)a[1],(real)a[2],(real)a[3]); }
	inline quat(real a, crv3 b)			{ set(a,b); }
	inline quat(crqt a)					{ w=a.w; x=a.x; y=a.y; z=a.z; }
	inline quat(crv3 a, crv3 b)			{ set(a,b); }
	
	inline operator real*(void)			{ return p; }
	inline real& operator[](int i)		{ return p[i]; }
	inline jvec3 im(void) const			{ return jvec3(v); }
	inline void setIm(crv3 a)			{ a.get(v); }
	
	inline void set(const real v[])		{ w=v[0], x=v[1], y=v[2], z=v[3]; }
	inline void set(real a,real b, real c, real d) { w=a, x=b, y=c, z=d; }
	inline void set(real a,crv3 b)		{ w=a; x=b.x; y=b.y; z=b.z; }
	inline void get(real v[]) const		{ v[0]=w,v[1]=x,v[2]=y,v[3]=z; }
	inline void set(crv3 p,crv3 q)
	{	jvec3 a=p.unit(), b=q.unit(), c=a*b;
		float si=c.len(), co=a%b;
		// Need a special case: if p and q are opposite to each other,
		// c became 0 vector and cos is t/2 is zero, too; which yield 0 quat.
		// In this case, we need to decide an axis orthogonal to a randomly
		if( co<-(1-EPSILON) && ABS(si)<EPSILON )
		{
			jvec3 axis = YAXIS; if( ABS(p%axis)>(1-EPSILON) ) axis=ZAXIS;
			c=p*axis;
		}
		real t=atan2f(si,co);
		set(cosf(t/2),c.unit()*sinf(t/2));
	}
	inline void clear(void)				{ w=1,x=y=z=0; }
	
	inline quat operator- (void)const	{ return quat(-w,-x,-y,-z); }
	inline rfqt operator*=(crqt a)		{ return *this=quat(r*a.r-im()%a.im(),r*a.im()+a.r*im()+im()*a.im()); }
	inline rfqt operator*=(real a)		{ w*=a;x*=a;y*=a;z*=a; return *this; }
	inline rfqt operator/=(real a)		{ w/=a;x/=a;y/=a;z/=a; return *this; }
	inline quat	operator* (crqt a)const { quat k(*this); k*=a; return k; }
	inline real	operator% (crqt a)const { return w*a.w+x*a.x+y*a.y+z*a.z; }
	inline quat	operator* (real a)const { return quat(w*a,x*a,y*a,z*a); }
	inline quat	operator/ (real a)const { return quat(w/a,x/a,y/a,z/a); }
	friend inline quat	operator*(real a, crqt v) { return v*a; }
	
	// WARNING: the following operators are only for testing or research purpose
	//          Basically, the quaternions here are assumed unit.
	//			Renormalization (+scalar multiplication) is meaningful sometimes.
	inline rfqt operator+=(crqt a)		{ w+=a.w;x+=a.x,y+=a.y;z+=a.z; return *this; }
	inline rfqt operator-=(crqt a)		{ w-=a.w;x-=a.x,y-=a.y;z-=a.z; return *this; }
	inline quat	operator+ (crqt a)const { return quat(w+a.w,x+a.x,y+a.y,z+a.z);}
	inline quat	operator- (crqt a)const { return quat(w-a.w,x-a.x,y-a.y,z-a.z);}
	// END WARNING
	
	inline real sqlen(void)const		{ return w*w+x*x+y*y+z*z; }
	inline real len  (void)const		{ return sqrtf(sqlen()); }
	inline quat unit (void)const
	{	real l=sqlen();
		if(l<EPSILON) return quat(0,0,0,0);
		if(l<1+EPSILON&&l>1-EPSILON) return *this;
		return *this/sqrtf(l);
	}
	inline rfqt normalize(void)			{ return *this=unit(); }
	inline quat inv(void)const			{ return quat(w,-im()); }
	inline jvec3 rot(crv3 a)const		{ quat b(0,a); return(*this*b*inv()).im(); }
	inline jvec3 ln(void) const	
	{ real t=atan2f(im().len(),w); return im().unit()*t; }
	friend inline real sqlen(crqt a)	{ return a.sqlen(); }
	friend inline real len  (crqt a)	{ return a.len(); }
	friend inline quat unit (crqt a)	{ return a.unit(); }
	friend inline quat normalize(crqt a){ return a.unit(); }
	friend inline quat inv(crqt a)		{ return a.inv(); }
	friend inline jvec3 rot(crqt a, crv3 b){ return a.rot(b); }
	friend inline jvec3 ln (crqt a)		{ return a.ln(); }
	
	friend inline std::ostream& operator <<(std::ostream& s,crqt a)
	{ s<<a.w<<", "<<a.x<<", "<<a.y<<", "<<a.z<<" "; return s; }
	friend inline std::istream& operator >>(std::istream& s,rfqt a)
	{ s>>a.w>>a.x>>a.y>>a.z; return s; }
	inline std::string toString(void)const
	{ std::stringstream s; s<<*this; return s.str(); }
	friend inline std::string toString(crqt a) { return a.toString(); }
	friend inline quat toQuat(std::string str)
	{ quat v; std::stringstream s(str); s>>v; return v; }
	
#ifdef __Quaternion_H__
#define JMATH_OGRE_WRAPPER_
	inline quat(const Ogre::Quaternion& v): w(v.w), x(v.x),y(v.y),z(v.z){}
	inline operator const Ogre::Quaternion(void) const { return Ogre::Quaternion(w,x,y,z); }
#endif
};

inline quat exp(crv3 a)			{ real t=a.len();return quat(cosf(t),a.unit()*sinf(t));}
inline quat r2q(crv3 a)			{ return exp(a/2); }
inline quat r2q(real d, crv3 a)	{ return exp(d*a/2); }
inline jvec3 q2r(crqt a)			{ return ln(a)*2; }
inline real angle(crv3 p, crv3 q)
{
	jvec3 a=p.unit(), b=q.unit(), c=a*b;
	return atan2f(c.len(),a%b);
}

inline quat getDOF(crqt q, crv3 uax)
{
	/*	jvec3 v = (XAXIS*uax).unit();
	 jvec3 w = q.rot(v);
	 if( uax%v>0.8f || uax%v<-0.8f || uax%w>0.8f || uax%w<-0.8f )
	 {
	 printf("Check!!");
	 v=(ZAXIS*uax).unit(), w=q.rot(v);	
	 if( uax%v>0.8f || uax%v<-0.8f || uax%w>0.8f || uax%w<-0.8f )
	 v=(YAXIS*uax).unit(), w=q.rot(v);
	 }
	 
	 */
	jvec3 v = (ZAXIS*uax).unit();
	jvec3 w = delDOF(q.rot(v),uax);
	if( sqlen(v)<0.1f || sqlen(w)<0.1f )
	{
		v = (YAXIS*uax).unit();
		w = delDOF(q.rot(v),uax);
	}
	if( sqlen(v)<0.1f || sqlen(w)<0.1f )
	{
		v = (XAXIS*uax).unit();
		w = delDOF(q.rot(v),uax);
	}
	return quat( v, w );
}

inline jvec3 diff(crqt a,crqt b)			{ return ln(a*b.inv()); }
inline jvec3 DIFF(crqt a,crqt b)			{ return a%b<0?diff(a,-b):diff(a,b); }
inline quat delDOF(crqt q,crv3 uax)		{ return getDOF(q, uax).inv()*q; }
inline quat slerp(crqt a,crqt b,real t) { return exp(diff(a,b)*t)*b; }
inline quat SLERP(crqt a,crqt b,real t) { return slerp(a,a%b<0?-b:b,t); }

const quat QID(1,0,0,0);


//************************************************************************
//
//    Two vector class
//    which is used to specify position and displacement in 2D
//
//************************************************************************

struct vec2;
typedef const vec2& crv2;
typedef vec2& rfv2;
struct vec2
{
	union {
		struct { real x, y; };
		real p[2];
	};
	inline vec2(void)					{ zero(); }
	inline vec2(real a,real b)			{ set(a,b); }
	inline vec2(const real a[])			{ set(a); }
	inline vec2(const double a[])		{ set((real)a[0],(real)a[1]); }
	inline vec2(crv2 a)					{ x=a.x; y=a.y; }
	
	inline operator real*(void)			{ return p; }
	inline real& operator[](int i)		{ return p[i]; }
	
	inline void set(const real v[])		{ x=v[0]; y=v[1]; }
	inline void set(real a,real b)		{ x=a, y=b; }
	inline void get(real v[]) const		{ v[0]=x, v[1]=y; }
	inline void zero(void)				{ x=y=0; }
	
	inline vec2 operator- (void)const	{ return vec2(-x,-y); }
	
	inline rfv2 operator+=(crv2 a)		{ x+=a.x, y+=a.y; return *this; }
	inline rfv2 operator-=(crv2 a)		{ x-=a.x, y-=a.y; return *this; }
	inline rfv2 operator*=(real a)		{ x*=a; y*=a; return *this; }
	inline rfv2 operator/=(real a)		{ x/=a; y/=a; return *this; }
	
	inline vec2	operator+ (crv2 a)const { return vec2(x+a.x,y+a.y); }
	inline vec2	operator- (crv2 a)const { return vec2(x-a.x,y-a.y); }
	inline real	operator% (crv2 a)const { return x*a.x+y*a.y; }
	inline vec2	operator* (real a)const { return vec2(x*a,y*a); }
	inline vec2	operator/ (real a)const { return vec2(x/a,y/a); }
	friend inline vec2	operator*(real a, crv2 v) { return v*a; }
	
	inline real sqlen(void)const		{ return x*x+y*y; }
	inline real len  (void)const		{ return sqrtf(sqlen()); }
	inline vec2 unit (void)const
	{	real l=sqlen();
		if(l<EPSILON) return vec2(0,0);
		if(l<1+EPSILON&&l>1-EPSILON) return *this;
		return *this/sqrtf(l);
	}
	inline rfv2 normalize(void)			{ return *this=unit(); }
	friend inline real len  (crv2 a)	{ return a.len(); }
	friend inline real sqlen(crv2 a)	{ return a.sqlen(); }
	friend inline vec2 unit (crv2 a)	{ return a.unit(); }
	friend inline vec2 normalize(crv2 a){ return a.unit(); }
	friend inline vec2 delDOF(crv2 v, crv2 uax){ return v-(v%uax)*uax; }
	
	friend inline std::ostream& operator <<(std::ostream& s,crv2 a)
	{ s<<a.x<<", "<<a.y<<" "; return s; }
	friend inline std::istream& operator >>(std::istream& s,rfv2 a)
	{ s>>a.x>>a.y; return s; }
	inline std::string toString(void)const
	{ std::stringstream s; s<<*this; return s.str(); }
	friend inline std::string toString(crv2 a) { return a.toString(); }
	friend inline vec2 toVec2(std::string str)
	{ vec2 v; std::stringstream s(str); s>>v; return v; }
#ifdef __Vector2_H__
#define JMATH_OGRE_WRAPPER_
	inline vec2(const Ogre::Vector2& v): x(v.x),y(v.y){}
	inline operator const Ogre::Vector2(void) const { return Ogre::Vector2(x,y); }
#endif
};

const vec2 V2ID(0,0);
const vec2 V2_0(0,0);
const vec2 V2_1(1,1);



//************************************************************************
//
//    2x2 Matrix class
//
//    Note: It is column major.
//          Constructor with array has different order
//          than constructor with four values.
//
//************************************************************************

struct mat2;
typedef const mat2& crm2;
typedef mat2& rfm2;
struct mat2
{
	union
	{
		struct{ real a,c,b,d; };
		struct{ real a00,a10,a01,a11; };
		real p[4];
	};
	inline mat2(void):a(1),b(0),c(0),d(1){}
	inline mat2(real _a,real _b,real _c,real _d):a(_a),b(_b),c(_c),d(_d){}
	inline mat2(const real x[]):a(x[0]),b(x[2]),c(x[1]),d(x[3]){}
	inline mat2(const double x[]):a((real)x[0]),b((real)x[2]),c((real)x[1]),d((real)x[3]){}
	inline mat2(real ang) { fromAngle(ang); }
	
	inline void loadIdentity(void) { a00=a11=1;a01=a10=0; }
	inline operator real* (void){ return p; }
	
	inline mat2 operator- (void) { return mat2(-a,-b,-c,-d); }
	
	inline rfm2 operator+=(crm2 k){ a+=k.a; b+=k.b; c+=k.c; d+=k.d; return *this;}
	inline rfm2 operator-=(crm2 k){ a-=k.a; b-=k.b; c-=k.c; d-=k.d; return *this;}
	inline rfm2 operator*=(real f){ a*=f,b*=f,c*=f,d*=f; return *this;}
	inline rfm2 operator/=(real f){ a/=f,b/=f,c/=f,d/=f; return *this;}
	inline rfm2 operator*=(crm2 k)
	{
		real k00 = a*k.a+b*k.c,
		k01 = a*k.b+b*k.d,
		k10 = c*k.a+d*k.c,
		k11 = c*k.b+d*k.d;
		a=k00, b=k01, c=k10, d=k11;
		return *this;
	}
	
	inline mat2 operator+ (crm2 k)const	{ return mat2(a+k.a,b+k.b,c+k.c,d+k.d);}
	inline mat2 operator- (crm2 k)const	{ return mat2(a-k.a,b-k.b,c-k.c,d-k.d);}
	inline mat2 operator* (crm2 k)const	{ return mat2(a*k.a+b*k.c,a*k.b+b*k.d,c*k.a+d*k.c,c*k.b+d*k.d);}
	inline mat2 operator* (real f)const	{ return mat2(a*f,b*f,c*f,d*f);}
	inline mat2 operator/ (real f)const	{ return mat2(a/f,b/f,c/f,d/f);}
	inline rfm2 transpose (void  )		{ return *this=mat2(a,c,b,d);}
	inline mat2 t         (void  )const	{ return mat2(a,c,b,d);}
	inline friend mat2 t  (crm2 m)		{ return m.t();}
	inline vec2 operator* (crv2 v)const	{ return vec2(a*v.x+b*v.y,c*v.x+d*v.y);}
	inline rfm2 invert   (void)
	{
		real INVDET = 1.0f/(a*d-b*c);
		real _a= d, _b=-c, _c=-b, _d= a;
		
		a=_a*INVDET; b=_b*INVDET;
		c=_c*INVDET; d=_d*INVDET;
		return *this;
	}
	inline mat2        inv(void   )const{ mat2 r=*this; r.invert(); return r;}
	inline friend mat2 inv(crm2 m)		{ mat2 r=m;     r.invert(); return r;}
	inline friend mat2 operator *(real v, crm2 m) { return m*v; }
	inline friend vec2 operator *(crv2 v, crm2 m) { return vec2(v.x*m.a+v.y*m.c,v.x*m.b+v.y*m.d);}
	
	friend inline std::istream& operator>>( std::istream& is, rfm2 t )
	{
		is>>t.a>>t.b;
		is>>t.c>>t.d;
		return is;
	}
	friend inline std::ostream& operator<<( std::ostream& os, crm2 t )
	{
		os<<t.a<<"\t"<<t.b<<std::endl;
		os<<t.c<<"\t"<<t.d<<std::endl;
		return os;
	}
	inline void fromAngle(real _a) { a=cosf(_a),b=-sinf(_a),c=sinf(_a),d=cosf(_a);}
	inline real toAngle(void) const { return atan2f(-b,a);}
};
inline mat2 sqsym(crv2 v)
{
	return mat2(v.x*v.x, v.x*v.y, 
				v.y*v.x, v.y*v.y);
}

inline real fromMat(crm2 m)	{ return m.toAngle(); }
inline mat2 toMat(real a)		{ return mat2(a); }
const  mat2 M2ID(1,0,0,1);
const  mat2 M2_0(0,0,0,0);
const  mat2 M2_1(1,1,1,1);

//****************************************************************************
//
//   OpenGL Functions
//     using jvec3 and quat
//
//****************************************************************************

#ifdef __gl_h_
inline void glVertex   (crv2 p)			{ glVertex2fv(p.p); }
inline void glTexCoord2(crv2 p)			{ glTexCoord2fv(p.p); }
inline void glLine     (crv2 a, crv2 b)	{ glBegin(GL_LINES); glVertex(a); glVertex(b); glEnd(); }
inline void glLineR	   (crv2 a, crv2 b)	{ glLine(a,a+b); }
inline void glRotate   (real a)			{ glRotatef(a*PI/180.0f,0,0,1);}

inline void glVertex   (crv3 p)			{ glVertex3fv(p.p); }
inline void glTexCoord2(crv3 p)			{ glTexCoord2fv(p.p); }
inline void glTexCoord3(crv3 p)			{ glTexCoord3fv(p.p); }
inline void glNormal   (crv3 p)			{ glNormal3fv(p.p); }
inline void glTranslate(crv3 p)			{ glTranslatef(p.x,p.y,p.z); }
inline void glLine     (crv3 a, crv3 b)	{ glBegin(GL_LINES); glVertex(a); glVertex(b); glEnd(); }
inline void glLineR    (crv3 a, crv3 b)	{ glLine(a,a+b); }
inline void glRotate   (crqt q)
{
	jvec3 v=ln(q)*2; real l=len(v); v=v.unit();
	glRotatef(l*180/PI, v.x, v.y, v.z);
}
#endif // __gl_h_


#endif

