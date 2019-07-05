//
//  glslUtils.h
//  NewViewer
//
//  Created by Hyun Joon Shin on 10/11/11.
//  Copyright 2011 VCLab, Ajou University. All rights reserved.
//


#ifndef GLSL_UTILS_H__
#define GLSL_UTILS_H__

#ifdef WIN32
#include <GL/gl.h>
#elif defined LINUX_BUILD
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <string>


extern GLuint loadVertShaderFile(const std::string& fn,bool log=false);
extern GLuint loadFragShaderFile(const std::string& fn,bool log=false);

extern GLuint loadVertShader    (const std::string& str,bool log=false);
extern GLuint loadFragShader    (const std::string& str,bool log=false);

extern GLuint loadProgramFiles  (const std::string& v, const std::string& f,bool log=false);
extern GLuint loadProgram       (const std::string& v, const std::string& f,bool log=false);
extern GLuint makeProgram       (GLuint vShader, GLuint fShader,bool log=false);


enum DIFFUSE_MODEL
{
	LAMBERTIAN,
	PLASTICY,
	OREN_NAYAR,
	MINNAERT,
};
enum SPECULAR_MODEL
{
	PHONG,
	BLINN,
	WARD,
	COOK_TORRANCE,
};

extern std::string coinFShader(DIFFUSE_MODEL dm, SPECULAR_MODEL sm, int textured);
				   

/*
extern const char* commonVShader;
extern const char* phongFShader;
extern const char* phongFShaderTex;
extern const char* plasticFShader;
extern const char* plasticFShaderTex;
extern const char* mattFShader;
extern const char* mattFShaderTex;
*/

#include <fstream>

inline std::string __loadTextFile(const std::string& fn)
{
	std::string ret;
	char lineBuffer[2048];
	std::ifstream ifs(fn.c_str());
	if( !ifs.is_open() ) return ret;
	while( !ifs.eof() )
	{
		ifs.getline(lineBuffer,2047);
		if( ifs.eof() ) break;
		ret+=std::string(lineBuffer)+'\n';
	}
	ifs.close();
	return ret;
}

inline void __printShaderLog(GLuint obj)
{
	char infoLog[32767];
	int infologLength = 0; int maxLength=32767;
	glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	if( infologLength > 0 ) printf("%s\n",infoLog);
}

inline void __printProgramLog(GLuint obj)
{
	char infoLog[32767];
	int infologLength = 0; int maxLength=32767;
	glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
	if( infologLength > 0 ) printf("%s\n",infoLog);
}


inline GLuint loadVertShader    (const std::string& str,bool log)
{
	int len = (int)str.length();
	const char* ptr = str.c_str();
	printf("Create VShader\n");
	GLuint shader = glCreateShader(GL_VERTEX_SHADER);
	printf("Set Source for VShader\n");
	glShaderSource(shader, 1, &ptr, &len);
	printf("Compile VShader\n");
	glCompileShader(shader);
	printf("Done\n");
	if( log ) __printShaderLog(shader);
	return shader;
}

inline GLuint loadFragShader    (const std::string& str,bool log)
{
	int len = (int)str.length();
	const char* ptr = str.c_str();
	GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader, 1, &ptr, &len);
	glCompileShader(shader);
	if( log ) __printShaderLog(shader);
	return shader;
}

inline GLuint loadVertShaderFile(const std::string& fn,bool log)
{
	std::string str = __loadTextFile(fn);
	return loadVertShader(str.c_str(),log);
}

inline GLuint loadFragShaderFile(const std::string& fn,bool log)
{
	std::string str = __loadTextFile(fn);
	return loadFragShader(str.c_str(),log);
}


inline GLuint loadProgramFiles(const std::string& v, const std::string& f,bool log)
{
	GLuint vshader = loadVertShaderFile(v,log);
	GLuint fshader = loadFragShaderFile(f,log);
	return makeProgram(vshader,fshader,log);
}
inline GLuint loadProgram(const std::string& v, const std::string& f,bool log)
{
	GLuint vshader = loadVertShader(v,log);
	GLuint fshader = loadFragShader(f,log);
	return makeProgram(vshader,fshader,log);
}

inline GLuint makeProgram(GLuint vshader, GLuint fshader,bool log)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);
	if(log) __printProgramLog(program);
	return program;
}


inline std::string coinFShader(DIFFUSE_MODEL dm, SPECULAR_MODEL sm, int textured)
{
	std::string ret;
	ret+="varying jvec3 N, E, L;\n"
	"uniform sampler2D tex2;\n"
	"void main() {\n"
	"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
	"	float df, sf;\n"
	"	jvec3 n=normalize(N);\n"
	"	jvec3 e=normalize(E);\n"
	"	jvec3 l=normalize(L);\n";
	if( textured )
	{
		ret+="	tx=texture2D(tex2, gl_TexCoord[0].st);\n";
	}
	switch( dm )
	{
		case LAMBERTIAN:
			ret+=
			"	df = clamp(dot(n,l),0.0,1.0);\n";
			break;
		case PLASTICY:
			ret+=
			"	float SIGMA=0.2, en, ln, aa, bb, alpha, beta, ad, a, b;\n"
			"	en=dot(e,n);\n"
			"	ln=dot(l,n);\n"
			"	aa=acos(en);\n"
			"	bb=acos(ln);\n"
			"	ad=max(0.0, dot( normalize(e-n*en), normalize(l-n*ln)));\n"
			"	alpha=max(aa,bb);\n"
			"	beta =min(1.0,min(bb,aa));\n"
			"	a=1.0-(0.50*SIGMA)/(SIGMA+0.33);\n"
			"	b=    (0.45*SIGMA)/(SIGMA+0.09);\n"
			"	df=max(0.0,ln*(a+b*ad*sin(alpha)*tan(beta)));\n";
			break;
		case OREN_NAYAR:
			ret+=
			"	float sigma = 0.3;\n"
			"	float s2 = sigma*sigma;\n"
			"	float A = 1.0-s2/(2.0*(s2+0.33));\n"
			"	float B = (0.45*s2)/(s2+0.09);\n"
			"	float cosThetaI = dot(l,n);\n"
			"	float cosThetaO = dot(e,n);\n"
			"	float thetaI = acos(cosThetaI), thetaO = acos(cosThetaO);\n"
			"	jvec3 tangent = normalize(cross(jvec3(0,1,0),n));\n"
			"	float cosPhiI = dot(l, tangent), cosPhiO = dot(e, tangent);\n"
			"	float sinAlpha, tanBeta;\n"
			"	if(thetaI<thetaO)\n"
			"	{\n"
			"		sinAlpha = sin(thetaO);\n"
			"		tanBeta = tan(thetaI);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		sinAlpha = sin(thetaI);\n"
			"		tanBeta = tan(thetaO);\n"
			"	}\n"
			"	float temp = max(0.0, cos(acos(cosPhiI)-acos(cosPhiO)));\n"
			"	df = clamp(cosThetaI,0.0,1.0)*clamp(A+(B*temp*sinAlpha*tanBeta),0.0,1.0);\n";
			break;
		case MINNAERT:
			ret+=
			"	float EXP=0.25;"
			"	df=pow(max(0.0,dot(n,l))*dot(n,e),EXP*3.0+.5);";
			break;
	}
	switch( sm )
	{
		case PHONG:
			ret+=
			"	jvec3 r=normalize(2.0*n*dot(n,l)-l);\n"
			"	sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess );\n";
			break;
		case BLINN:
			ret+=
			"	jvec3 h = normalize(e+l);\n"
			"	sf=pow(max(0.0,dot(n,h)), gl_FrontMaterial.shininess );\n";
			break;
		case WARD:
			ret+=
			"	float p1 = log(gl_FrontMaterial.shininess)/5.0;\n"
			"	float s2=(1.0-p1)*(1.0-p1)+0.00000000001;\n"
			"	jvec3 h = normalize(e+l);\n"
			"	sf=dot(l,n)*exp(-pow(tan(acos(dot(n,h))),2.0)/s2)/(6.283184*s2*sqrt(df*dot(n,e)));";
			break;
		case COOK_TORRANCE:
			ret+=
			"	float p1 = log(gl_FrontMaterial.shininess)/4.7;"
			"	float p2 = 0.2;"
			"	float m=1.0-p1, scale=p2*0.4;"
			"	jvec3 h = normalize(e+l);\n"
			"	float c=dot(e,h);\n"
			"	float nh=dot(n,h);\n"
			"	float nl=dot(n,l);\n"
			"	float nv=dot(n,e);\n"
			"	float G=min(min(.5,nh*nv/c),nh*nl/c)*2.0;\n"
			"	float F=pow(1.0+nv,4.0);\n"
			"	float D=scale*exp((nh*nh-1.0)/m);\n"
			"	sf=G*F*D/nv;\n";
			break;
	}
	ret+=
	"	color =gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
	"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
	"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
	"	gl_FragColor = color;\n"
	"	gl_FragColor.a = diffuseColor.a;\n"
	"}\n";
	
	return ret;
}

inline std::string commonVShader(void)
{
	return std::string(
	"varying jvec3 N, L, E;\n"
	"void main()\n"
	"{\n"
	"	vec4 m = gl_ModelViewMatrix*gl_Vertex;\n"
	"	gl_FrontColor = gl_Color;\n"
	"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"	gl_Position = ftransform();\n"
	"	L = jvec3(gl_LightSource[0].position-m);\n"
	"	N = gl_NormalMatrix * gl_Normal;\n"
	"	E = jvec3(0,0,0)-jvec3(gl_ModelViewMatrix*gl_Vertex);\n"
	"}\n");
}

//const char* commonVShader =


//const char* phongFShader =
inline std::string phongFShader(void)
{
	return std::string(
	"varying jvec3 N, E, L;\n"
	"void main() {\n"
	"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
	"	jvec3 n=normalize(N);\n"
	"	jvec3 e=normalize(E);\n"
	"	jvec3 l=normalize(L);\n"
	"	jvec3 r=normalize(2.0*n*dot(n,l)-l);\n"
	"	float df = clamp(dot(n,l),0.0,1.0);\n"
	"	float sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess );\n"
	"	color =gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
	"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
	"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
	"	gl_FragColor = color;\n"
	"	gl_FragColor.a = diffuseColor.a;\n"
	"}\n");
}

//const char* phongFShaderTex =
inline std::string phontFShaderTex(void)
{
	return std::string(
	"varying jvec3 N, E, L;\n"
	"uniform sampler2D tex2;\n"
	"void main() {\n"
	"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
	"	tx=texture2D(tex2, gl_TexCoord[0].st);\n"
	"	diffuseColor*=tx; // Assuming Modulation Mode\n"
	"	jvec3 n=normalize(N);\n"
	"	jvec3 e=normalize(E);\n"
	"	jvec3 l=normalize(L);\n"
	"	jvec3 r=normalize(2.0*n*dot(n,l)-l);\n"
	"	float df = clamp(dot(n,l),0.0,1.0);\n"
	"	float sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess );\n"
	"	color =gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
	"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
	"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
	"	gl_FragColor = color;\n"
	"	gl_FragColor.a = diffuseColor.a;\n"
	"}\n");
}
//const char* plasticFShader =
inline std::string plasticFShader(void)
{
	return std::string(
"varying jvec3 N, E, L;\n"
"void main() {\n"
"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
"	jvec3 n=normalize(N);\n"
"	jvec3 e=normalize(E);\n"
"	jvec3 l=normalize(L);\n"
"	jvec3 r=normalize(2.0*n*dot(l,n)-l);\n"
"	float SIGMA=0.2, en, ln, aa, bb, alpha, beta, ad, a, b, df, sf, at;\n"
"	en=dot(e,n);\n"
"	ln=dot(l,n);\n"
"	aa=acos(en);\n"
"	bb=acos(ln);\n"
"	ad=max(0.0, dot( normalize(e-n*en), normalize(l-n*ln)));\n"
"	alpha=max(aa,bb);\n"
"	beta =min(1.0,min(bb,aa));\n"
"	a=1.0-(0.50*SIGMA)/(SIGMA+0.33);\n"
"	b=    (0.45*SIGMA)/(SIGMA+0.09);\n"
"	df=max(0.0,ln*(a+b*ad*sin(alpha)*tan(beta)));\n"
"	sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess);\n"
"	color+=gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
"	gl_FragColor = color;\n"
"	gl_FragColor.a=diffuseColor.a;\n"
"}");
}

//const char* plasticFShaderTex =
inline std::string plasticFShaderTex(void)
{
	return std::string(
"varying jvec3 N, E, L;\n"
"void main() {\n"
"	jvec3 n, r, e, l, ct;\n"
"	float SIGMA=0.2, en, ln, aa, bb, alpha, beta, ad, a, b, df, sf, at;\n"
"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
"	tx=texture2D(tex2, gl_TexCoord[0].st);\n"
"	n=normalize(N);\n"
"	e=normalize(E);\n"
"	l=normalize(L);\n"
"	r=normalize(2.0*n*dot(l,n)-l);\n"
"	en=dot(e,n);\n"
"	ln=dot(l,n);\n"
"	aa=acos(en);\n"
"	bb=acos(ln);\n"
"	ad=max(0.0, dot( normalize(e-n*en), normalize(l-n*ln)));\n"
"	alpha=max(aa,bb);\n"
"	beta =min(1.0,min(bb,aa));\n"
"	a=1.0-(0.50*SIGMA)/(SIGMA+0.33);\n"
"	b=    (0.45*SIGMA)/(SIGMA+0.09);\n"
"	df=max(0.0,ln*(a+b*ad*sin(alpha)*tan(beta)));\n"
"	sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess);\n"
"	color+=gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
"	gl_FragColor = color;\n"
"	gl_FragColor.a=diffuseColor.a;\n"
"}");
}

//const char* mattFShader =
inline std::string mattFShader(void)
{
	return std::string(
"varying jvec3 N, E, L;\n"
"void main() {\n"
"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
"	jvec3 n=normalize(N);\n"
"	jvec3 e=normalize(E);\n"
"	jvec3 l=normalize(L);\n"
"	jvec3 r=normalize(2.0*n*dot(n,l)-l);\n"
"	float sigma = 0.3;\n"
"	float s2 = sigma*sigma;\n"
"	float A = 1.0-s2/(2.0*(s2+0.33));\n"
"	float B = (0.45*s2)/(s2+0.09);\n"
"	float cosThetaI = dot(l,n);\n"
"	float cosThetaO = dot(e,n);\n"
"	float thetaI = acos(cosThetaI), thetaO = acos(cosThetaO);\n"
"	jvec3 tangent = normalize(cross(jvec3(0,1,0),n));\n"
"	float cosPhiI = dot(l, tangent), cosPhiO = dot(e, tangent);\n"
"	float sinAlpha, tanBeta;\n"
"	if(thetaI<thetaO)\n"
"	{\n"
"		sinAlpha = sin(thetaO);\n"
"		tanBeta = tan(thetaI);\n"
"	}\n"
"	else\n"
"	{\n"
"		sinAlpha = sin(thetaI);\n"
"		tanBeta = tan(thetaO);\n"
"	}\n"
"	float temp = max(0.0, cos(acos(cosPhiI)-acos(cosPhiO)));\n"
"	float df = clamp(cosThetaI,0.0,1.0)*clamp(A+(B*temp*sinAlpha*tanBeta),0.0,1.0);\n"
"	float sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess );\n"
"	color =gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
"	gl_FragColor = color;\n"
"	gl_FragColor.a = diffuseColor.a;\n"
"}\n");
}

//const char* mattFShaderTex =
inline std::string mattFShaderTex(void)
{
	return std::string(
"varying jvec3 N, E, L;\n"
"uniform sampler2D tex2;\n"
"void main() {\n"
"	vec4 color=vec4(0,0,0,0), tx, diffuseColor=gl_FrontMaterial.diffuse;\n"
"	tx=texture2D(tex2, gl_TexCoord[0].st);\n"
"	diffuseColor*=tx; // Assuming Modulation Mode\n"
"	jvec3 n=normalize(N);\n"
"	jvec3 e=normalize(E);\n"
"	jvec3 l=normalize(L);\n"
"	jvec3 r=normalize(2.0*n*dot(n,l)-l);\n"
"	float sigma = 0.9;\n"
"	float s2 = sigma*sigma;\n"
"	float A = 1.0-s2/(2.0*(s2+0.33));\n"
"	float B = (0.45*s2)/(s2+0.09);\n"
"	float cosThetaI = dot(l,n);\n"
"	float cosThetaO = dot(e,n);\n"
"	float thetaI = acos(cosThetaI), thetaO = acos(cosThetaO);\n"
"	jvec3 tangent = normalize(cross(jvec3(0,1,0),n));\n"
"	float cosPhiI = dot(l, tangent), cosPhiO = dot(e, tangent);\n"
"	float sinAlpha, tanBeta;\n"
"	if(thetaI<thetaO)\n"
"	{\n"
"		sinAlpha = sin(thetaO);\n"
"		tanBeta = tan(thetaI);\n"
"	}\n"
"	else\n"
"	{\n"
"		sinAlpha = sin(thetaI);\n"
"		tanBeta = tan(thetaO);\n"
"	}\n"
"	float temp = max(0.0, cos(acos(cosPhiI)-acos(cosPhiO)));\n"
"	float df = clamp(cosThetaI,0.0,1.0)*clamp(A+(B*temp*sinAlpha*tanBeta),0.0,1.0);\n"
"	float sf=pow(max(0.0,dot(r,e)), gl_FrontMaterial.shininess );\n"
"	color =gl_FrontMaterial.ambient *gl_LightSource[0].ambient;\n"
"	color+=diffuseColor             *gl_LightSource[0].diffuse *df;\n"
"	color+=gl_FrontMaterial.specular*gl_LightSource[0].specular*sf;\n"
"	gl_FragColor = color;\n"
"	gl_FragColor.a = diffuseColor.a;\n"
"}\n");
}

#endif

