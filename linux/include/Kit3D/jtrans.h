//
//  jTrans.h
//  CemeraFilter
//
//  Created by Hyun Joon Shin on 11/2/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//


#ifndef __JTrans_H__
#define __JTrans_H__

#include "jmath.h"


//************************************************************************
//
//    Four vector class
//    which is used to specify homogenious vector
//    or color with alpha value
//
//    Note: Quaternion looks very similar but
//          has the different mathematical meaning.
//          For example, default conversion to jvec3 should be different.
//          Therefore, to distingush them,
//          "h" is used for the last component instead of usual "w".
//
//************************************************************************

struct vec4;
typedef vec4& rfv4;
typedef const vec4& crv4;

struct vec4
{
	union {
		struct { real x, y, z, h; };
		real p[4];
	};
	inline vec4(void)					{ zero(); }
	inline vec4(real a,real b,real c,real d)	{ set(a,b,c,d); }
	inline vec4(const real a[])			{ set(a); }
	inline vec4(const double a[])		{ set((real)a[0],(real)a[1],(real)a[2],(real)a[3]); }
	inline vec4(crv4 a)					{ x=a.x; y=a.y; z=a.z; h=a.h; }
	inline vec4(crv3 a, real b=0)		{ x=a.x; y=a.y; z=a.z; h=b; }
	
	inline operator real*(void)			{ return p; }
	inline real& operator[](int i)		{ return p[i]; }
	inline operator jvec3(void) const	{ return jvec3(x,y,z); }
	
	inline void set(const real v[])		{ x=v[0]; y=v[1]; z=v[2]; h=v[3]; }
	inline void set(real a,real b,real c,real d) { x=a, y=b, z=c; h=d;}
	inline void get(real v[]) const		{ v[0]=x,v[1]=y,v[2]=z,v[3]=h; }
	inline void zero(void)				{ x=y=z=h=0; }
	
	inline vec4 operator- (void)const	{ return vec4(-x,-y,-z,-h); }
	
	inline rfv4 operator+=(crv4 a)		{ x+=a.x, y+=a.y, z+=a.z, h+=a.h; return *this; }
	inline rfv4 operator-=(crv4 a)		{ x-=a.x, y-=a.y, z-=a.z, h-=a.h; return *this; }
	inline rfv4 operator*=(real a)		{ x*=a,y*=a,z*=a,h*=a; return *this; }
	inline rfv4 operator/=(real a)		{ x/=a,y/=a,z/=a,h*=a; return *this; }
	
	inline vec4	operator+ (crv4 a)const { return vec4(x+a.x,y+a.y,z+a.z,h+a.h);}
	inline vec4	operator- (crv4 a)const { return vec4(x-a.x,y-a.y,z-a.z,h-a.h);}
	inline real	operator% (crv4 a)const { return x*a.x+y*a.y+z*a.z+h*a.h; }
	inline vec4	operator* (real a)const { return vec4(x*a,y*a,z*a,h*a); }
	inline vec4	operator/ (real a)const { return vec4(x/a,y/a,z/a,h/a); }
	friend inline vec4	operator*(real a, crv4 v) { return v*a; }
	
	inline real sqlen(void)const		{ return x*x+y*y+z*z+h*h; }
	inline real len  (void)const		{ return sqrtf(sqlen()); }
	inline vec4 unit (void)const
	{	real l=sqlen();
		if(l<EPSILON) return vec4(0,0,0,0);
		if(l<1+EPSILON&&l>1-EPSILON) return *this;
		return *this/sqrtf(l);
	}
	inline rfv4 normalize(void)			{ return *this=unit(); }
	friend inline real len  (crv4 a)	{ return a.len(); }
	friend inline real sqlen(crv4 a)	{ return a.sqlen(); }
	friend inline vec4 unit (crv4 a)	{ return a.unit(); }
	friend inline vec4 normalize(crv4 a){ return a.unit(); }
	friend inline vec4 delDOF(crv4 v, crv4 uax){ return v-(v%uax)*uax; }
	
	friend inline std::ostream& operator <<(std::ostream& s,crv4 a)
	{ s<<a.x<<", "<<a.y<<", "<<a.z<<" "<<a.h<<" "; return s; }
	friend inline std::istream& operator >>(std::istream& s,rfv4 a)
	{ s>>a.x>>a.y>>a.z>>a.h; return s; }
	inline std::string toString(void)const
	{ std::stringstream s; s<<*this; return s.str(); }
	friend inline std::string toString(crv4 a) { return a.toString(); }
	friend inline vec4 toVec4(std::string str)
	{ vec4 v; std::stringstream s(str); s>>v; return v; }
	
#ifdef __Vector4_H__
#define JMATH_OGRE_WRAPPER_
	inline vec4(const Ogre::Vector4& v): x(v.x),y(v.y),z(v.z),h(v.w){}
	inline operator const Ogre::Vector4(void) const { return Ogre::Vector4(x,y,z,h); }
#endif
};


//************************************************************************
//
//    3x3 matrix
//    Corresponding to 3D linear transformations.
//    It covers orthogonal matries and conversion to and from quat
//    Note: order of array representation is colume major.
//          subscript based represenration is
//          [  a00   a01   a02  ]
//          [  a10   a11   a12  ]
//          [  a20   a21   a22  ]
//
//************************************************************************

struct mat3;
typedef mat3& rfm3;
typedef const mat3& crm3;

struct mat3
{
	union{
		real p[9];
		struct{ real a00, a10, a20, a01, a11, a21, a02, a12, a22; };
	};
	inline mat3(const real v[])  { set(v[0],v[3],v[6],v[1],v[4],v[7],v[2],v[5],v[9]); }
	inline mat3(const double v[]){ set((real)v[0],(real)v[3],(real)v[6],(real)v[1],
									   (real)v[4],(real)v[7],(real)v[2],(real)v[5],(real)v[9]); }
	inline mat3(real b00, real b01, real b02, real b10, real b11, real b12,
				real b20, real b21, real b22) { set(b00,b01,b02,b10,b11,b12,b20,b21,b22); }
	inline mat3(void) { loadIdentity(); }
	inline mat3(crqt q){ fromQuat(q); }
	
	inline void loadIdentity(void) { a00=a11=a22=1;a01=a02=a10=a12=a20=a21=0; }
	inline operator real* (void){ return p; }
	inline operator quat (void) const { return toQuat(); }
	
	
	inline void set(real r0, real r1, real r2, real r3, real r4, real r5,
					real r6, real r7, real r8)
	{ a00=r0;a01=r1;a02=r2;a10=r3;a11=r4;a12=r5;a20=r6;a21=r7;a22=r8; }	
	inline jvec3 row(int i) const { return jvec3(p[0+i],p[3+i],p[6+i]); }
	inline jvec3 col(int i) const { return jvec3(&(p[i*3])); }
	
	inline mat3 operator -(void) const { return mat3(-a00,-a01,-a02,-a10,-a11,-a12,-a20,-a21,-a22); }
	
	inline rfm3 operator+=(crm3 m)
	{
		a00+=m.a00; a01+=m.a01; a02+=m.a02;
		a10+=m.a10; a11+=m.a11; a12+=m.a12;
		a20+=m.a20; a21+=m.a21; a22+=m.a22;
		return *this;
	}
	inline rfm3 operator-=(crm3 m)
	{
		a00-=m.a00; a01-=m.a01; a02-=m.a02;
		a10-=m.a10; a11-=m.a11; a12-=m.a12;
		a20-=m.a20; a21-=m.a21; a22-=m.a22;
		return *this;
	}
	inline rfm3 operator*=(real v)
	{
		a00*=v; a01*=v; a02*=v;
		a10*=v; a11*=v; a12*=v;
		a20*=v; a21*=v; a22*=v;
		return *this;
	}
	inline rfm3 operator/=(real v)
	{
		a00/=v; a01/=v; a02/=v;
		a10/=v; a11/=v; a12/=v;
		a20/=v; a21/=v; a22/=v;
		return *this;
	}
	inline rfm3 operator*=(crm3 m)
	{
		real q00=a00,q01=a01,q02=a02;
		real q10=a10,q11=a11,q12=a12;
		real q20=a20,q21=a21,q22=a22;
		a00 = q00*m.a00 + q01*m.a10 + q02*m.a20;
		a01 = q00*m.a01 + q01*m.a11 + q02*m.a21;
		a02 = q00*m.a02 + q01*m.a12 + q02*m.a22;
		a10 = q10*m.a00 + q11*m.a10 + q12*m.a20;
		a11 = q10*m.a01 + q11*m.a11 + q12*m.a21;
		a12 = q10*m.a02 + q11*m.a12 + q12*m.a22;
		a20 = q20*m.a00 + q21*m.a10 + q22*m.a20;
		a21 = q20*m.a01 + q21*m.a11 + q22*m.a21;
		a22 = q20*m.a02 + q21*m.a12 + q22*m.a22;
		return *this;
	}
	
	inline mat3 operator +(crm3 m)const { mat3 r=*this; r+=m; return r;}
	inline mat3 operator -(crm3 m)const { mat3 r=*this; r-=m; return r;}
	inline mat3 operator *(real v)const	{ mat3 r=*this; r*=v; return r;}
	inline mat3 operator *(crm3 m)const	{ mat3 r=*this; r*=m; return r;}
	inline mat3 operator /(real v)const	{ mat3 r=*this; r/=v; return r;}
	inline rfm3 transpose (void  )      { set(a00,a10,a20,a01,a11,a21,a02,a12,a22); return *this;}
	inline mat3	t         (void  )const { return mat3(a00,a10,a20,a01,a11,a21,a02,a12,a22); }
	inline friend mat3 t  (crm3 m)		{ return m.t(); }
	inline jvec3 operator *(crv3 v)const
	{
		return jvec3(a00*v.x + a01*v.y + a02*v.z, 
					a10*v.x + a11*v.y + a12*v.z, 
					a20*v.x + a21*v.y + a22*v.z);
	}
	
	inline rfm3 invert(void)
	{
		real INVDET = ABS(a00*(a22*a11-a21*a12)-a10*(a22*a01-a21*a02)
						  +a20*(a12*a01-a11*a02));
		real b00 =  a22*a11-a21*a12;
		real b01 = -a22*a01+a21*a02;
		real b02 =  a12*a01-a11*a02;
		real b10 = -a22*a10+a20*a12;
		real b11 =  a22*a00-a20*a02;
		real b12 = -a12*a00+a10*a02;
		real b20 =  a21*a10-a20*a11;
		real b21 = -a21*a00+a20*a01;
		real b22 =  a11*a00-a10*a01;
		
		a00=b00/INVDET; a01=b01/INVDET; a02=b02/INVDET;
		a10=b10/INVDET; a11=b11/INVDET; a12=b12/INVDET;
		a20=b20/INVDET; a21=b21/INVDET; a22=b22/INVDET;
		return *this;
	}
	
	inline mat3		   inv(void  )const	{ mat3 r=*this; r.invert(); return r; }
	inline friend mat3 inv(crm3 m)		{ mat3 r=m;     r.invert(); return r; }
	inline friend mat3 operator *(real v, crm3 m) { return m*v; }
	inline friend jvec3 operator *(crv3 v, crm3 m)
	{
		return jvec3(m.a00*v.x + m.a10*v.y + m.a20*v.z,
					m.a01*v.x + m.a11*v.y + m.a21*v.z,
					m.a02*v.x + m.a12*v.y + m.a22*v.z);
	}
	
	inline quat toQuat(void) const 
	{
		quat q;
		real tr=p[0] + p[4] + p[8], s;
		if ( tr > 0.0f )
		{
			s = sqrtf( tr + 1.0f );
			q[0] = ( s * 0.5f );
			s = 0.5f / s;
			q[1] =(p[5]-p[7]) * s;
			q[2] =(p[6]-p[2]) * s;
			q[3] =(p[1]-p[3]) * s;
		}
		else
		{
			int    i, j, k;
			static int next[3] = { 1, 2, 0 };
			i = 0;
			if ( p[4] > p[0] ) i = 1;
			if ( p[8] > p[i*3+i] ) i = 2;
			
			j = next[i];
			k = next[j];
			
			s = sqrtf( p[i*3+i]-p[j*3+j]-p[k*3+k] + 1.0f );
			q[i+1] = s * 0.5f;
			s = 0.5f / s;
			q[0]   = ( p[j*3+k] - p[k*3+j] ) * s;
			q[j+1] = ( p[i*3+j] + p[j*3+i] ) * s;
			q[k+1] = ( p[i*3+k] + p[k*3+i] ) * s;
		}
		return q;
	}
	inline void fromQuat(const quat q)
	{
		real s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
		s  = 2.0f/len(q);
		xs = q.x * s;  ys = q.y * s;  zs = q.z * s;
		wx = q.w * xs; wy = q.w * ys; wz = q.w * zs;
		xx = q.x * xs; xy = q.x * ys; xz = q.x * zs;
		yy = q.y * ys; yz = q.y * zs; zz = q.z * zs;
		set(1 - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1 - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1 - (xx + yy));
	}
	
	
	friend inline std::istream& operator>>( std::istream& is, rfm3 t )
	{
		is>>t.a00>>t.a01>>t.a02;
		is>>t.a10>>t.a11>>t.a12;
		is>>t.a20>>t.a21>>t.a22;
		return is;
	}
	friend inline std::ostream& operator<<( std::ostream& os, crm3 t )
	{
		os<<t.a00<<"\t"<<t.a01<<"\t"<<t.a02<<std::endl;
		os<<t.a10<<"\t"<<t.a11<<"\t"<<t.a12<<std::endl;
		os<<t.a20<<"\t"<<t.a21<<"\t"<<t.a22<<std::endl;
		return os;
	}
};

inline mat3 sqsym(crv3 v)
{
	return mat3(v.x*v.x, v.x*v.y, v.x*v.z,
				v.y*v.x, v.y*v.y, v.y*v.z,
				v.z*v.x, v.z*v.y, v.z*v.z);
}
inline quat fromMat(crm3 m)			{ return m.toQuat(); }
inline mat3 toMat  (crqt q)			{ return mat3(q); }

const  mat3 M3ID(1,0,0,0,1,0,0,0,1);
const  mat3 M3_0(0,0,0,0,0,0,0,0,0);
const  mat3 M3_1(1,1,1,1,1,1,1,1,1);




//************************************************************************
//
//    Homogenious transform matrix
//    Corresponding to 3D homogenious transform
//    One can use this structure to store 4x4 matrix but
//        many operation are not supported.
//    Note: order of array representation is colume major.
//          subscript based represenration is (in particular affine transform)
//          [  a00   a01   a02   tx  ]
//          [  a10   a11   a12   ty  ]
//          [  a20   a21   a22   tz  ]
//          [   0     0     0     1  ]
//
//    Array rep:
//
//          [  p[ 0]   p[ 4]   p[ 8]   p[12]  ]
//          [  p[ 1]   p[ 5]   p[ 9]   p[13]  ]
//          [  p[ 2]   p[ 6]   p[10]   p[14]  ]
//          [  p[ 3]   p[ 7]   p[11]   p[15]  ]
//
//************************************************************************

struct tran;
typedef tran mat4;
typedef tran& rftr;
typedef const tran& crtr;
typedef mat4& rfm4;
typedef const mat4& crm4;

struct tran
{
public:
	union{
		real p[16];
		struct{
			real a00, a10, a20, a30,
			a01, a11, a21, a31,
			a02, a12, a22, a32,
			a03, a13, a23, a33;
		};
	};
	inline tran(void) { loadIdentity(); }
	inline tran(const real v[]){ set(v); }
	inline tran(const double v[]){ set(v); }
	inline tran(real b00, real b01, real b02, real b03,
				real b10, real b11, real b12, real b13,
				real b20, real b21, real b22, real b23,
				real b30, real b31, real b32, real b33)
	{ set(b00,b01,b02,b03,b10,b11,b12,b13,b20,b21,b22,b23,b30,b31,b32,b33); }
	inline tran(crm3 m, crv3 v){ set(m,v); }
	inline tran(crqt q, crv3 v){ set(q,v); }
	
	inline void loadIdentity(void)
	{
		a00=1;a01=0;a02=0;a03=0;
		a10=0;a11=1;a12=0;a13=0;
		a20=0;a21=0;a22=1;a23=0;
		a30=0;a31=0;a32=0;a33=1;
	}
	inline operator real*(void){ return p; }
	inline operator mat3 (void){ return getMat3(); }
	
	inline void set(real b00, real b01, real b02, real b03,
					real b10, real b11, real b12, real b13,
					real b20, real b21, real b22, real b23,
					real b30, real b31, real b32, real b33)
	{
		a00=b00; a01=b01; a02=b02; a03=b03;
		a10=b10; a11=b11; a12=b12; a13=b13;
		a20=b20; a21=b21; a22=b22; a23=b23;
		a30=b30; a31=b31; a32=b32; a33=b33;
	}
	
	inline void set(crm3 m, crv3 v)
	{
		a00=m.a00; a01=m.a01; a02=m.a02; a03=v.x;
		a10=m.a10; a11=m.a11; a12=m.a12; a13=v.y;
		a20=m.a20; a21=m.a21; a22=m.a22; a23=v.z;
		a30=0;     a31=0;     a32=0;     a33=1;
	}
	inline void set(const real v[])
	{
		p[ 0]=v[ 0]; p[ 1]=v[ 1]; p[ 2]=v[ 2]; p[ 3]=v[ 3];
		p[ 4]=v[ 4]; p[ 5]=v[ 5]; p[ 6]=v[ 6]; p[ 7]=v[ 7];
		p[ 8]=v[ 8]; p[ 9]=v[ 9]; p[10]=v[10]; p[11]=v[11];
		p[12]=v[12]; p[13]=v[13]; p[14]=v[14]; p[15]=v[15];
	}
	inline void set(const double v[])
	{
		p[ 0]=(real)v[ 0];p[ 1]=(real)v[ 1];p[ 2]=(real)v[ 2];p[ 3]=(real)v[ 3];
		p[ 4]=(real)v[ 4];p[ 5]=(real)v[ 5];p[ 6]=(real)v[ 6];p[ 7]=(real)v[ 7];
		p[ 8]=(real)v[ 8];p[ 9]=(real)v[ 9];p[10]=(real)v[10];p[11]=(real)v[11];
		p[12]=(real)v[12];p[13]=(real)v[13];p[14]=(real)v[14];p[15]=(real)v[15];
	}
	
	inline void set(crqt q, crv3 v) { mat3 m(q); set(m,v); }
	
	inline vec4 row(int i){	return vec4(p[0+i],p[4+i],p[8+i],p[12+i]); }
	inline vec4 col(int i){ return vec4(&(p[i*4])); }
	inline mat3 getMat3(void)const { return mat3(a00,a01,a02,a10,a11,a12,a20,a21,a22); }
	
	inline jvec3 getTrns(void)const { return jvec3(a03,a13,a23); }
	inline quat getQuat(void)const { return fromMat(getMat3()); }
	inline tran operator -(void) const
	{
		return tran(-a00,-a01,-a02,-a03,
					-a10,-a11,-a12,-a13,
					-a20,-a21,-a22,-a23,
					-a30,-a31,-a32,-a33);
	}
	inline rftr operator+=(crtr m)
	{
		a00+=m.a00; a01+=m.a01; a02+=m.a02; a03+=m.a03; 
		a10+=m.a10; a11+=m.a11; a12+=m.a12; a13+=m.a13; 
		a20+=m.a20; a21+=m.a21; a22+=m.a22; a23+=m.a23; 
		a30+=m.a30; a31+=m.a31; a32+=m.a32; a33+=m.a33; 
		return *this;
	}
	inline rftr operator-=(crtr m)
	{
		a00-=m.a00; a01-=m.a01; a02-=m.a02; a03-=m.a03; 
		a10-=m.a10; a11-=m.a11; a12-=m.a12; a13-=m.a13; 
		a20-=m.a20; a21-=m.a21; a22-=m.a22; a23-=m.a23; 
		a30-=m.a30; a31-=m.a31; a32-=m.a32; a33-=m.a33; 
		return *this;
	}
	inline rftr operator*=(real v)
	{
		a00*=v; a01*=v; a02*=v; a03*=v;
		a10*=v; a11*=v; a12*=v; a13*=v;
		a20*=v; a21*=v; a22*=v; a23*=v;
		a30*=v; a31*=v; a32*=v; a33*=v;
		return *this;
	}
	inline rftr operator/=(real v)
	{
		a00/=v; a01/=v; a02/=v; a03/=v;
		a10/=v; a11/=v; a12/=v; a13/=v;
		a20/=v; a21/=v; a22/=v; a23/=v;
		a30/=v; a31/=v; a32/=v; a33/=v;
		return *this;
	}
	inline rftr operator*=(crtr b)
	{
		*this=tran
		(a00*b.a00 + a01*b.a10 + a02*b.a20 + a03*b.a30,
		 a00*b.a01 + a01*b.a11 + a02*b.a21 + a03*b.a31,
		 a00*b.a02 + a01*b.a12 + a02*b.a22 + a03*b.a32,
		 a00*b.a03 + a01*b.a13 + a02*b.a23 + a03*b.a33,
		 
		 a10*b.a00 + a11*b.a10 + a12*b.a20 + a13*b.a30,
		 a10*b.a01 + a11*b.a11 + a12*b.a21 + a13*b.a31,
		 a10*b.a02 + a11*b.a12 + a12*b.a22 + a13*b.a32,
		 a10*b.a03 + a11*b.a13 + a12*b.a23 + a13*b.a33,
		 
		 a20*b.a00 + a21*b.a10 + a22*b.a20 + a23*b.a30,
		 a20*b.a01 + a21*b.a11 + a22*b.a21 + a23*b.a31,
		 a20*b.a02 + a21*b.a12 + a22*b.a22 + a23*b.a32,
		 a20*b.a03 + a21*b.a13 + a22*b.a23 + a23*b.a33,
		 
		 a30*b.a00 + a31*b.a10 + a32*b.a20 + a33*b.a30,
		 a30*b.a01 + a31*b.a11 + a32*b.a21 + a33*b.a31,
		 a30*b.a02 + a31*b.a12 + a32*b.a22 + a33*b.a32,
		 a30*b.a03 + a31*b.a13 + a32*b.a23 + a33*b.a33
		 );
		return *this;
	}
	inline tran operator +(crtr m)const { tran r=*this; r+=m; return r;}
	inline tran operator -(crtr m)const { tran r=*this; r-=m; return r;}
	inline tran operator *(real v)const	{ tran r=*this; r*=v; return r;}
	inline tran operator /(real v)const	{ tran r=*this; r/=v; return r;}
	inline tran operator *(crtr m)const	{ tran r=*this; r*=m; return r;}
	inline rftr transpose (void  )
	{
		real q00=a00, q01=a01, q02=a02, q03=a03;
		real q10=a10, q11=a11, q12=a12, q13=a13;
		real q20=a20, q21=a21, q22=a22, q23=a23;
		real q30=a30, q31=a31, q32=a32, q33=a33;
		a00=q00; a01=q10; a02=q20; a03=q30;
		a10=q01; a11=q11; a12=q21; a13=q31;
		a20=q02; a21=q12; a22=q22; a23=q32;
		a30=q03; a31=q13; a32=q23; a33=q33;
		return *this;
	}
	inline tran t         (void  )const { tran r=*this; r.transpose(); return r; }
	inline friend mat4 t  (crm4 m)		{ return m.t(); }
	
	inline vec4 operator* (crv4 v) const
	{
		return vec4(v.x*a00 + v.y*a01 + v.z*a02 + v.h*a03,
					v.x*a10 + v.y*a11 + v.z*a12 + v.h*a13,
					v.x*a20 + v.y*a21 + v.z*a22 + v.h*a23,
					v.x*a30 + v.y*a31 + v.z*a32 + v.h*a33);
	}
	inline jvec3 operator* (crv3 v) const
	{
		vec4 v4(v.x*a00 + v.y*a01 + v.z*a02 + a03,
				v.x*a10 + v.y*a11 + v.z*a12 + a13,
				v.x*a20 + v.y*a21 + v.z*a22 + a23,
				v.x*a30 + v.y*a31 + v.z*a32 + a33);
		return jvec3(v4.x,v4.y,v4.z)/v4.h;
	}
	inline friend tran operator *(real v, crtr m) { return m*v; }
	inline friend vec4 operator *(crv4 v, crm4 m)
	{
		return vec4(m.a00*v.x + m.a10*v.y + m.a20*v.z + m.a30*v.h,
					m.a01*v.x + m.a11*v.y + m.a21*v.z + m.a31*v.h,
					m.a02*v.x + m.a12*v.y + m.a22*v.z + m.a32*v.h,
					m.a03*v.x + m.a13*v.y + m.a23*v.z + m.a33*v.h);
	}
	
	
	friend inline std::istream& operator>>( std::istream& is, rftr t )
	{
		is>>t.a00>>t.a01>>t.a02>>t.a03;
		is>>t.a10>>t.a11>>t.a12>>t.a13;
		is>>t.a20>>t.a21>>t.a22>>t.a23;
		is>>t.a30>>t.a31>>t.a32>>t.a33;
		return is;
	}
	friend inline std::ostream& operator<<( std::ostream& os, crtr t )
	{
		os<<t.a00<<"\t"<<t.a01<<"\t"<<t.a02<<"\t"<<t.a03<<std::endl;
		os<<t.a10<<"\t"<<t.a11<<"\t"<<t.a12<<"\t"<<t.a13<<std::endl;
		os<<t.a20<<"\t"<<t.a21<<"\t"<<t.a22<<"\t"<<t.a23<<std::endl;
		os<<t.a30<<"\t"<<t.a31<<"\t"<<t.a32<<"\t"<<t.a33<<std::endl;
		return os;
	}
};

const  tran TRID(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
const  tran M4ID(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
const  tran M4_0(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
const  tran M4_1(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);



#include <vector>

template<class T,class M> struct lineT
{
	T p, v;
	inline lineT(const T& p1, const T& p2): p(p1), v((p2-p1).unit()){}
	inline real sqdist(const T& x) const { return sqlen((x-p)-((x-p)%v)*v); }
	inline real dist(const T& x) const { return sqrtf(sqdist(x)); }
	inline T    proj(const T& x) const { return p+((x-p)%v)*v; }
	
	// My beatiful powerful all-in-one constructor for linear robust regression
	//	line
};

// Hyperplain
template<class T> struct plainT
{
	T p, n;
	inline plainT(const T& p1, const T& v): p(p1), n(v.unit()){}
	inline real dist(const T& x) const { return (x-p)%n; }
	inline real sqdist(const T& x) const { real d=dist(x); return d*d; }
	inline T    proj(const T& x) const { return x-((x-p)%n)*n; }
};

//*****************************************************************************
//
//  Line functions
//
//*****************************************************************************

template<class T,class M> inline T closest(const std::vector<lineT<T,M> >& lines, int s=0, int l=-1)
{
	T r; r.zero();
	if( l<0 ) l=(int)lines.size();
	M A; A.loadIdentity(); A*=lines.size();
	for( int i=s; i<s+l; i++)
	{
		A-=sqsym(lines[i].v);
		r+=lines[i].p - (lines[i].v%lines[i].p)*lines[i].v;
	}
	return A.inv()*r;
}
template<class T,class M> inline real sqdist(const std::vector<lineT<T,M> >& lines, const T& p)
{
	real sum=0;
	for(int i=0; i<lines.size(); i++) sum+=lines[i].sqdist(p);
	return sum/lines.size();
}
template<class T,class M> inline vec2 inters(const lineT<T,M>& l1, const lineT<T,M>& l2)
{
	std::vector<lineT<T,M> > lines;
	lines.push_back(l1); lines.push_back(l2);
	return closest(lines);
}
template<class T,class M> inline vec2 inters(const T& p1, const T& e1, const T& p2, const T& e2)
{
	return inters(lineT<T,M>(p1,e1), lineT<T,M>(p2,e2));
}


//*****************************************************************************
//
//  Hyperplain functions
//
//*****************************************************************************

typedef lineT<vec2,mat2> line2;
typedef lineT<jvec3,mat3> line3;
typedef line3 line;
typedef plainT<vec2> plain2;
typedef plainT<jvec3> plain3;
typedef plain3 plain;

inline jvec3 inters(const line& l, const plain& p)
{
	return l.v/(l.v%p.n)*((p.p-l.p)%p.n)+l.p;
}
inline jvec3 inters(const line& l, crv3 p, crv3 n){ return inters(l,plain(p,n)); }


#ifdef __gl_h_
#define DEFL GL_LIGHT0
inline void glLightPos(crv4 p, int light=DEFL)	{ glLightfv(light, GL_POSITION, p.p); }
inline void glLightPos(crv3 p, int light=DEFL)	{ glLightPos(vec4(p,1),light); }
inline void glMultMatrix(crtr t)				{ glMultMatrixf(t.p); }
inline void glLoadMatrix(crtr t)				{ glLoadMatrixf(t.p); }
inline void glLine(const line& l)				{ glLine(l.p-l.v*1000,l.p+l.v*1000); }
inline void glLine(const line2& l)				{ glLine(l.p-l.v*1000,l.p+l.v*1000); }
#endif


#endif

