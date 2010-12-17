/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SWFTYPES_H
#define SWFTYPES_H

#include "compat.h"
#include <llvm/System/DataTypes.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>

#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "exceptions.h"
#ifndef WIN32
 // TODO: Proper CMake check
 #include <arpa/inet.h>
#endif
#include <stdatomic.h>
#include <endian.h>

#ifdef BIG_ENDIAN
#include <algorithm>
#endif

namespace lightspark
{

#define ASFUNCTION(name) \
	static ASObject* name(ASObject* , ASObject* const* args, const unsigned int argslen)
#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

#define CLASSBUILDABLE(className) \
	friend class Class<className>; 

enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	T_DEFINABLE=7, T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14};

enum STACK_TYPE{STACK_NONE=0,STACK_OBJECT,STACK_INT,STACK_UINT,STACK_NUMBER,STACK_BOOLEAN};

enum TRISTATE { TFALSE=0, TTRUE, TUNDEFINED };

typedef double number_t;

class ASObject;
class Bitmap;

class tiny_string
{
friend std::ostream& operator<<(std::ostream& s, const tiny_string& r);
private:
	enum TYPE { READONLY=0, STATIC, DYNAMIC };
	#define TS_SIZE 64
	char _buf_static[TS_SIZE];
	char* buf;
	TYPE type;
	//TODO: use static buffer again if reassigning to short string
	void makePrivateCopy(const char* s)
	{
		resetToStatic();
		if(strlen(s)>(TS_SIZE-1))
			createBuffer();
		assert_and_throw(strlen(s)<=4096);
		strcpy(buf,s);
	}
	void createBuffer()
	{
		type=DYNAMIC;
		buf=new char[4096];
	}
	void resetToStatic()
	{
		if(type==DYNAMIC)
			delete[] buf;
		buf=_buf_static;
		type=STATIC;
	}
public:
	tiny_string():buf(_buf_static),type(STATIC){buf[0]=0;}
	tiny_string(const char* s,bool copy=false):buf(_buf_static),type(READONLY)
	{
		if(copy)
			makePrivateCopy(s);
		else
			buf=(char*)s; //This is an unsafe conversion, we have to take care of the RO data
	}
	tiny_string(const tiny_string& r):buf(_buf_static),type(STATIC)
	{
		if(strlen(r.buf)>(TS_SIZE-1))
			createBuffer();
		assert_and_throw(strlen(r.buf)<=4096);
		strcpy(buf,r.buf);
	}
	tiny_string(const std::string& r):buf(_buf_static),type(STATIC)
	{
		if(r.size()>(TS_SIZE-1))
		{
			createBuffer();
			assert_and_throw(r.size()<=4096);
			//Comment this assertion and uncomment the following lines to just crop the strings
			//if(r.size()>4096)
			//{
			//	LOG(LOG_NO_INFO, _("tiny_string::tiny_string(): std::string is too big for tiny_string, cropping: ") << r.size() <<_(">")<<4096);
			//	strcpy(buf,r.substr(0,4096).c_str());
			//	return;
			//}
		}
		strcpy(buf,r.c_str());
	}
	~tiny_string()
	{
		resetToStatic();
	}
	explicit tiny_string(int i):buf(_buf_static),type(STATIC)
	{
		sprintf(buf,"%i",i);
	}
	explicit tiny_string(number_t d):buf(_buf_static),type(STATIC)
	{
		sprintf(buf,"%g",d);
	}
	tiny_string& operator=(const tiny_string& s)
	{
		resetToStatic();
		if(s.len()>(TS_SIZE-1))
			createBuffer();
		//Lenght is already checked by the other tiny_string
		strcpy(buf,s.buf);
		return *this;
	}
	tiny_string& operator=(const std::string& s)
	{
		resetToStatic();
		if(s.size()>(TS_SIZE-1))
		{
			createBuffer();
			assert_and_throw(s.size()<=4096);
			//Comment this assertion and uncomment the following lines to just crop the strings
			//if(s.size()>4096)
			//{
			//	LOG(LOG_NO_INFO, _("tiny_string::operator=(): std::string is too big for tiny_string, cropping: ") << s.size() <<_(">")<<4096);
			//	strcpy(buf,s.substr(0,4096).c_str());
			//  return *this;
			//}
		}
		//Lenght is already checked by the assertion
		strcpy(buf,s.c_str());
		return *this;
	}
	tiny_string& operator=(const char* s)
	{
		resetToStatic();
		type=READONLY;
		buf=(char*)s; //This is an unsafe conversion, we have to take care of the RO data
		return *this;
	}
	tiny_string& operator+=(const char* s)
	{
		assert_and_throw((strlen(buf)+strlen(s)+1)<=4096);
		if(type==READONLY)
		{
			char* tmp=buf;
			makePrivateCopy(tmp);
		}
		if(type==STATIC && (strlen(buf)+strlen(s)+1)>TS_SIZE)
		{
			createBuffer();
			strcpy(buf,_buf_static);
		}
		strcat(buf,s);
		return *this;
	}
	tiny_string& operator+=(const tiny_string& r)
	{
		assert_and_throw((strlen(buf)+strlen(r.buf)+1)<=4096);
		if(type==READONLY)
		{
			char* tmp=buf;
			makePrivateCopy(tmp);
		}
		if(type==STATIC && (strlen(buf)+strlen(r.buf)+1)>TS_SIZE)
		{
			createBuffer();
			strcpy(buf,_buf_static);
		}
		strcat(buf,r.buf);
		return *this;
	}
	const tiny_string operator+(const tiny_string& r)
	{
		tiny_string ret(buf);
		ret+=r;
		return ret;
	}
	bool operator<(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)<0;
	}
	bool operator==(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)==0;
	}
	bool operator!=(const tiny_string& r) const
	{
		return strcmp(buf,r.buf)!=0;
	}
	bool operator==(const char* r) const
	{
		return strcmp(buf,r)==0;
	}
	bool operator!=(const char* r) const
	{
		return strcmp(buf,r)!=0;
	}
	const char* raw_buf() const
	{
		return buf;
	}
	char operator[](int i) const
	{
		return *(buf+i);
	}
	int len() const
	{
		return strlen(buf);
	}
	tiny_string substr(int start, int end) const
	{
		tiny_string ret;
		assert_and_throw((end-start+1)<TS_SIZE);
		strncpy(ret.buf,buf+start,end-start);
		ret.buf[end-start]=0;
		return ret;
	}
};

class QName
{
public:
	tiny_string ns;
	tiny_string name;
	QName(const tiny_string& _name, const tiny_string& _ns):ns(_ns),name(_name){}
	bool operator<(const QName& r) const
	{
		if(ns==r.ns)
			return name<r.name;
		else
			return ns<r.ns;
	}
};

class UI8 
{
friend std::istream& operator>>(std::istream& s, UI8& v);
private:
	uint8_t val;
public:
	UI8():val(0){}
	UI8(uint8_t v):val(v){}
	operator uint8_t() const { return val; }
};

class UI16_SWF
{
friend std::istream& operator>>(std::istream& s, UI16_SWF& v);
protected:
	uint16_t val;
public:
	UI16_SWF():val(0){}
	UI16_SWF(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class UI16_FLV
{
friend std::istream& operator>>(std::istream& s, UI16_FLV& v);
protected:
	uint16_t val;
public:
	UI16_FLV():val(0){}
	UI16_FLV(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class SI16_SWF
{
friend std::istream& operator>>(std::istream& s, SI16_SWF& v);
protected:
	int16_t val;
public:
	SI16_SWF():val(0){}
	SI16_SWF(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

class SI16_FLV
{
friend std::istream& operator>>(std::istream& s, SI16_FLV& v);
protected:
	int16_t val;
public:
	SI16_FLV():val(0){}
	SI16_FLV(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

class UI24_SWF
{
friend std::istream& operator>>(std::istream& s, UI24_SWF& v);
protected:
	uint32_t val;
public:
	UI24_SWF():val(0){}
	operator uint32_t() const { return val; }
};

class UI24_FLV
{
friend std::istream& operator>>(std::istream& s, UI24_FLV& v);
protected:
	uint32_t val;
public:
	UI24_FLV():val(0){}
	operator uint32_t() const { return val; }
};

class SI24_SWF
{
friend std::istream& operator>>(std::istream& s, SI24_SWF& v);
protected:
	int32_t val;
public:
	SI24_SWF():val(0){}
	operator int32_t() const { return val; }
};

class SI24_FLV
{
friend std::istream& operator>>(std::istream& s, SI24_FLV& v);
protected:
	int32_t val;
public:
	SI24_FLV():val(0){}
	operator int32_t() const { return val; }
};

class UI32_SWF
{
friend std::istream& operator>>(std::istream& s, UI32_SWF& v);
protected:
	uint32_t val;
public:
	UI32_SWF():val(0){}
	UI32_SWF(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class UI32_FLV
{
friend std::istream& operator>>(std::istream& s, UI32_FLV& v);
protected:
	uint32_t val;
public:
	UI32_FLV():val(0){}
	UI32_FLV(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class STRING
{
friend std::ostream& operator<<(std::ostream& s, const STRING& r);
friend std::istream& operator>>(std::istream& stream, STRING& v);
friend class ASString;
private:
	std::string String;
public:
	STRING():String(){};
	STRING(const char* s):String(s)
	{
	}
	bool operator==(const STRING& s)
	{
		if(String.size()!=s.String.size())
			return false;
		for(uint32_t i=0;i<String.size();i++)
		{
			if(String[i]!=s.String[i])
				return false;
		}
		return true;
	}
	/*STRING operator+(const STRING& s)
	{
		STRING ret(*this);
		for(unsigned int i=0;i<s.String.size();i++)
			ret.String.push_back(s.String[i]);
		return ret;
	}*/
	bool isNull() const
	{
		return !String.size();
	}
	operator const std::string&() const
	{
		return String;
	}
	operator const char*() const
	{
		return String.c_str();
	}
	int size()
	{
		return String.size();
	}
};

//Numbers taken from AVM2 specs
enum NS_KIND { NAMESPACE=0x08, PACKAGE_NAMESPACE=0x16, PACKAGE_INTERNAL_NAMESPACE=0x17, PROTECTED_NAMESPACE=0x18, 
			EXPLICIT_NAMESPACE=0x19, STATIC_PROTECTED_NAMESPACE=0x1A, PRIVATE_NAMESPACE=0x05 };

struct nsNameAndKind
{
	tiny_string name;
	NS_KIND kind;
	nsNameAndKind(const tiny_string& _name, NS_KIND _kind):name(_name),kind(_kind){}
	nsNameAndKind(const char* _name, NS_KIND _kind):name(_name),kind(_kind){}
	bool operator<(const nsNameAndKind& r) const
	{
		return name < r.name;
	}
	bool operator==(const nsNameAndKind& r) const
  	{
		return /*kind==r.kind &&*/ name==r.name;
  	}
};

struct multiname
{
	enum NAME_TYPE {NAME_STRING,NAME_INT,NAME_NUMBER,NAME_OBJECT};
	NAME_TYPE name_type;
	tiny_string name_s;
	union
	{
		int32_t name_i;
		number_t name_d;
		ASObject* name_o;
	};
	std::vector<nsNameAndKind> ns;
	tiny_string qualifiedString() const;
};

class FLOAT 
{
friend std::istream& operator>>(std::istream& s, FLOAT& v);
private:
	float val;
public:
	FLOAT():val(0){}
	FLOAT(float v):val(v){}
	operator float(){ return val; }
};

class DOUBLE 
{
friend std::istream& operator>>(std::istream& s, DOUBLE& v);
private:
	double val;
public:
	DOUBLE():val(0){}
	DOUBLE(double v):val(v){}
	operator double(){ return val; }
};

//TODO: Really implement or suppress
typedef UI32_SWF FIXED;

//TODO: Really implement or suppress
typedef UI16_SWF FIXED8;

class RECORDHEADER
{
friend std::istream& operator>>(std::istream& s, RECORDHEADER& v);
private:
	UI16_SWF CodeAndLen;
	UI32_SWF Length;
public:
	unsigned int getLength() const
	{
		if((CodeAndLen&0x3f)==0x3f)
			return Length;
		else
			return CodeAndLen&0x3f;
	}
	unsigned int getTagType() const
	{
		return CodeAndLen>>6;
	}
};

class RGB
{
public:
	RGB(){};
	RGB(int r,int g, int b):Red(r),Green(g),Blue(b){};
	UI8 Red;
	UI8 Green;
	UI8 Blue;
};

class RGBA
{
public:
	RGBA():Red(0),Green(0),Blue(0),Alpha(255){}
	RGBA(int r, int g, int b, int a):Red(r),Green(g),Blue(b),Alpha(a){}
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	UI8 Alpha;
	RGBA& operator=(const RGB& r)
	{
		Red=r.Red;
		Green=r.Green;
		Blue=r.Blue;
		Alpha=255;
		return *this;
	}
	float rf() const
	{
		float ret=Red;
		ret/=255;
		return ret;
	}
	float gf() const
	{
		float ret=Green;
		ret/=255;
		return ret;
	}
	float bf() const
	{
		float ret=Blue;
		ret/=255;
		return ret;
	}
	float af() const
	{
		float ret=Alpha;
		ret/=255;
		return ret;
	}
};

typedef UI8 LANGCODE;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI8& v)
{
	s.read((char*)&v.val,1);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=LittleEndianToHost16(v.val);
	return s;
}

inline std::istream & operator>>(std::istream &s, SI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=BigEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=LittleEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=BigEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_SWF& v)
{
	s.read((char*)&v.val,4);
	v.val=LittleEndianToHost32(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_FLV& v)
{
	s.read((char*)&v.val,4);
	v.val=BigEndianToHost32(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, FLOAT& v)
{
	union float_reader
	{
		uint32_t dump;
		float value;
	};
	float_reader dummy;
	s.read((char*)&dummy.dump,4);
	dummy.dump=LittleEndianToHost32(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, DOUBLE& v)
{
	union double_reader
	{
		uint64_t dump;
		double value;
	};
	double_reader dummy;
	// "Wacky format" is 45670123. Thanks to Gnash for reversing :-)
	s.read(((char*)&dummy.dump)+4,4);
	s.read(((char*)&dummy.dump),4);
	dummy.dump=LittleEndianToHost64(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, RECORDHEADER& v)
{
	s >> v.CodeAndLen;
	if((v.CodeAndLen&0x3f)==0x3f)
		s >> v.Length;
	return s;
}

class BitStream
{
public:
	std::istream& f;
	unsigned char buffer;
	unsigned char pos;
public:
	BitStream(std::istream& in):f(in),pos(0){};
	unsigned int readBits(unsigned int num)
	{
		unsigned int ret=0;
		while(num)
		{
			if(!pos)
			{
				pos=8;
				f.read((char*)&buffer,1);
			}
			ret<<=1;
			ret|=(buffer>>(pos-1))&1;
			pos--;
			num--;
		}
		return ret;
	}
};

class FB
{
	int32_t buf;
	int size;
public:
	FB() { buf=0; }
	FB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,_("Fixed point bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator float() const
	{
		if(buf>=0)
		{
			int32_t b=buf;
			return b/65536.0f;
		}
		else
		{
			int32_t b=-buf;
			return -(b/65536.0f);
		}
		//return (buf>>16)+(buf&0xffff)/65536.0f;
	}
};

class UB
{
	uint32_t buf;
	int size;
public:
	UB() { buf=0; }
	UB(int s,BitStream& stream):size(s)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(imin(s,8));
			s-=imin(s,8);
			i++;
		}*/
		if(s>32)
			LOG(LOG_ERROR,_("Unsigned bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
	}
	operator int() const
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
	int size;
public:
	SB() { buf=0; }
	SB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,_("Signed bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator int() const
	{
		return buf;
	}
};

class RECT
{
	friend std::ostream& operator<<(std::ostream& s, const RECT& r);
	friend std::istream& operator>>(std::istream& stream, RECT& v);
public:
	int Xmin;
	int Xmax;
	int Ymin;
	int Ymax;
public:
	RECT();
	RECT(int xmin, int xmax, int ymin, int ymax);
};

class MATRIX
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
public:
	number_t ScaleX;
	number_t ScaleY;
	number_t RotateSkew0;
	number_t RotateSkew1;
	int TranslateX;
	int TranslateY;
public:
	MATRIX():ScaleX(1),ScaleY(1),RotateSkew0(0),RotateSkew1(0),TranslateX(0),TranslateY(0){}
	void get4DMatrix(float matrix[16]) const;
	void multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	MATRIX multiplyMatrix(const MATRIX& r) const;
	const bool operator!=(const MATRIX& r) const;
	MATRIX getInverted() const;
};

class GRADRECORD
{
	friend std::istream& operator>>(std::istream& s, GRADRECORD& v);
public:
	GRADRECORD(int v):version(v){}
	int version;
	UI8 Ratio;
	RGBA Color;
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
};

class GRADIENT
{
	friend std::istream& operator>>(std::istream& s, GRADIENT& v);
public:
	GRADIENT(int v):version(v){}
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
};

class FOCALGRADIENT
{
	friend std::istream& operator>>(std::istream& s, FOCALGRADIENT& v);
public:
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
	float FocalPoint;
};

class FILLSTYLEARRAY;
class MORPHFILLSTYLE;

enum FILL_STYLE_TYPE { SOLID_FILL=0x00, LINEAR_GRADIENT=0x10, RADIAL_GRADIENT=0x12, FOCAL_RADIAL_GRADIENT=0x13, REPEATING_BITMAP=0x40,
			CLIPPED_BITMAP=0x41, NON_SMOOTHED_REPEATING_BITMAP=0x42, NON_SMOOTHED_CLIPPED_BITMAP=0x43};

class FILLSTYLE
{
public:
	FILLSTYLE(int v):version(v),Gradient(v){}
	int version;
	FILL_STYLE_TYPE FillStyleType;
	RGBA Color;
	MATRIX Matrix;
	GRADIENT Gradient;
	FOCALGRADIENT FocalGradient;
	Bitmap* bitmap;
	virtual ~FILLSTYLE(){}
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	MORPHFILLSTYLE():FILLSTYLE(1){}
	RGBA StartColor;
	RGBA EndColor;
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	UI8 NumGradients;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
	~MORPHFILLSTYLE(){}
};

class LINESTYLE
{
public:
	LINESTYLE(int v):version(v){}
	int version;
	UI16_SWF Width;
	RGBA Color;
};

class LINESTYLE2
{
public:
	LINESTYLE2(int v):version(v),FillType(v){}
	int version;
	UI16_SWF Width;
	UB StartCapStyle;
	UB JointStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16_SWF MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
};

class MORPHLINESTYLE
{
public:
	UI16_SWF StartWidth;
	UI16_SWF EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class LINESTYLEARRAY
{
public:
	LINESTYLEARRAY(int v):version(v){}
	int version;
	void appendStyles(const LINESTYLEARRAY& r);
	UI8 LineStyleCount;
	std::list<LINESTYLE> LineStyles;
	std::list<LINESTYLE2> LineStyles2;
};

class MORPHLINESTYLEARRAY
{
public:
	UI8 LineStyleCount;
	MORPHLINESTYLE* LineStyles;
};

class FILLSTYLEARRAY
{
public:
	FILLSTYLEARRAY(int v):version(v){}
	int version;
	void appendStyles(const FILLSTYLEARRAY& r);
	UI8 FillStyleCount;
	std::list<FILLSTYLE> FillStyles;
};

class MORPHFILLSTYLEARRAY
{
public:
	UI8 FillStyleCount;
	MORPHFILLSTYLE* FillStyles;
};

class SHAPE;
class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	SHAPE* parent;
	bool TypeFlag;
	bool StateNewStyles;
	bool StateLineStyle;
	bool StateFillStyle1;
	bool StateFillStyle0;
	bool StateMoveTo;

	uint32_t MoveBits;
	int32_t MoveDeltaX;
	int32_t MoveDeltaY;

	unsigned int FillStyle1;
	unsigned int FillStyle0;
	unsigned int LineStyle;

	//Edge record
	bool StraightFlag;
	uint32_t NumBits;
	bool GeneralLineFlag;
	bool VertLineFlag;
	int32_t DeltaX;
	int32_t DeltaY;

	int32_t ControlDeltaX;
	int32_t ControlDeltaY;
	int32_t AnchorDeltaX;
	int32_t AnchorDeltaY;

	SHAPERECORD(SHAPE* p,BitStream& bs);
};

class TEXTRECORD;

class GLYPHENTRY
{
public:
	UB GlyphIndex;
	SB GlyphAdvance;
	TEXTRECORD* parent;
	GLYPHENTRY(TEXTRECORD* p,BitStream& bs);
};

class DefineTextTag;

class TEXTRECORD
{
public:
	UB TextRecordType;
	UB StyleFlagsReserved;
	UB StyleFlagsHasFont;
	UB StyleFlagsHasColor;
	UB StyleFlagsHasYOffset;
	UB StyleFlagsHasXOffset;
	UI16_SWF FontID;
	RGBA TextColor;
	SI16_SWF XOffset;
	SI16_SWF YOffset;
	UI16_SWF TextHeight;
	UI8 GlyphCount;
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	TEXTRECORD(DefineTextTag* p):parent(p){}
};

class SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPE& v);
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPE():fillOffset(0),lineOffset(0){}
	virtual ~SHAPE(){};
	UB NumFillBits;
	UB NumLineBits;
	unsigned int fillOffset;
	unsigned int lineOffset;
	std::vector<SHAPERECORD> ShapeRecords;
};

class SHAPEWITHSTYLE : public SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPEWITHSTYLE(int v):version(v),FillStyles(v),LineStyles(v){}
	const int version;
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
};

class CXFORMWITHALPHA
{
	friend std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
private:
	UB HasAddTerms;
	UB HasMultTerms;
	UB NBits;
	SB RedMultTerm;
	SB GreenMultTerm;
	SB BlueMultTerm;
	SB AlphaMultTerm;
	SB RedAddTerm;
	SB GreenAddTerm;
	SB BlueAddTerm;
	SB AlphaAddTerm;
};

class CXFORM
{
};

class DROPSHADOWFILTER
{
public:
    RGBA DropShadowColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class BLURFILTER
{
public:
	FIXED BlurX;
	FIXED BlurY;
	UB Passes;
};

class GLOWFILTER
{
public:
    RGBA GlowColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class BEVELFILTER
{
public:
    RGBA ShadowColor;
    RGBA HighlightColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
    UB Passes;
};

class GRADIENTGLOWFILTER
{
public:
    UI8 NumColors;
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class CONVOLUTIONFILTER
{
public:
    UI8 MatrixX;
    UI8 MatrixY;
    FLOAT Divisor;
    FLOAT Bias;
    std::vector<FLOAT> Matrix;
    RGBA DefaultColor;
    bool Clamp;
    bool PreserveAlpha;
};

class COLORMATRIXFILTER
{
public:
    FLOAT Matrix[20];
};

class GRADIENTBEVELFILTER
{
public:
    UI8 NumColors;
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
    UB Passes;
};

class FILTER
{
public:
	UI8 FilterID;
	DROPSHADOWFILTER DropShadowFilter;
	BLURFILTER BlurFilter;
	GLOWFILTER GlowFilter;
	BEVELFILTER BevelFilter;
	GRADIENTGLOWFILTER GradientGlowFilter;
	CONVOLUTIONFILTER ConvolutionFilter;
	COLORMATRIXFILTER ColorMatrixFilter;
	GRADIENTBEVELFILTER GradientBevelFilter;
};

class FILTERLIST
{
public:
	UI8 NumberOfFilters;
	std::vector<FILTER> Filters;
};

class BUTTONRECORD
{
public:
	BUTTONRECORD(int v):buttonVersion(v){}
	int buttonVersion;
	UB ButtonReserved;
	UB ButtonHasBlendMode;
	UB ButtonHasFilterList;
	UB ButtonStateHitTest;
	UB ButtonStateDown;
	UB ButtonStateOver;
	UB ButtonStateUp;
	UI16_SWF CharacterID;
	UI16_SWF PlaceDepth;
	MATRIX PlaceMatrix;
	CXFORMWITHALPHA	ColorTransform;
	FILTERLIST FilterList;
	UI8 BlendMode;

	bool isNull() const
	{
		return !(ButtonReserved | ButtonHasBlendMode | ButtonHasFilterList | ButtonStateHitTest | ButtonStateDown | ButtonStateOver | ButtonStateUp);
	}
};

class CLIPEVENTFLAGS
{
public:
	uint32_t toParse;
	bool isNull();
};

class CLIPACTIONRECORD
{
public:
	CLIPEVENTFLAGS EventFlags;
	UI32_SWF ActionRecordSize;
	bool isLast();
};

class CLIPACTIONS
{
public:
	UI16_SWF Reserved;
	CLIPEVENTFLAGS AllEventFlags;
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
};

class RunState
{
public:
	unsigned int FP;
	unsigned int next_FP;
	unsigned int max_FP;
	bool stop_FP;
	bool explicit_FP;
	RunState();
	void prepareNextFP();
};

ASObject* abstract_i(intptr_t i);
ASObject* abstract_b(bool i);
ASObject* abstract_d(number_t i);

void stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns);

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::ostream& operator<<(std::ostream& s, const multiname& r);
std::ostream& operator<<(std::ostream& s, const tiny_string& r) DLL_PUBLIC;
std::ostream& operator<<(std::ostream& s, const QName& r);

std::istream& operator>>(std::istream& s, RECT& v);
std::istream& operator>>(std::istream& s, CLIPEVENTFLAGS& v);
std::istream& operator>>(std::istream& s, CLIPACTIONRECORD& v);
std::istream& operator>>(std::istream& s, CLIPACTIONS& v);
std::istream& operator>>(std::istream& s, RGB& v);
std::istream& operator>>(std::istream& s, RGBA& v);
std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, LINESTYLE2& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPERECORD& v);
std::istream& operator>>(std::istream& stream, TEXTRECORD& v);
std::istream& operator>>(std::istream& stream, MATRIX& v);
std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
std::istream& operator>>(std::istream& stream, GLYPHENTRY& v);
std::istream& operator>>(std::istream& stream, STRING& v);
std::istream& operator>>(std::istream& stream, BUTTONRECORD& v);
std::istream& operator>>(std::istream& stream, RECORDHEADER& v);
std::istream& operator>>(std::istream& stream, FILTERLIST& v);
std::istream& operator>>(std::istream& stream, FILTER& v);
std::istream& operator>>(std::istream& stream, DROPSHADOWFILTER& v);
std::istream& operator>>(std::istream& stream, BLURFILTER& v);
std::istream& operator>>(std::istream& stream, GLOWFILTER& v);
std::istream& operator>>(std::istream& stream, BEVELFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTGLOWFILTER& v);
std::istream& operator>>(std::istream& stream, CONVOLUTIONFILTER& v);
std::istream& operator>>(std::istream& stream, COLORMATRIXFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTBEVELFILTER& v);


};
#endif
