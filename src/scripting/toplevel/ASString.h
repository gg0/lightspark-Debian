/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef ASSTRING_H_
#define ASSTRING_H_

#include "class.h"

namespace Glib { class ustring; }

namespace lightspark
{
/*
 * The AS String class.
 * The 'data' is immutable -> it cannot be changed after creation of the object
 */
class ASString: public ASObject
{
CLASSBUILDABLE(ASString);
private:
	tiny_string toString_priv() const;
	ASString();
	ASString(const std::string& s);
	ASString(const tiny_string& s);
	ASString(const Glib::ustring& s);
	ASString(const char* s);
	ASString(const char* s, uint32_t len);
public:
	tiny_string data;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(charAt);
	ASFUNCTION(charCodeAt);
	ASFUNCTION(concat);
	ASFUNCTION(fromCharCode);
	ASFUNCTION(indexOf);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(match);
	ASFUNCTION(replace);
	ASFUNCTION(search);
	ASFUNCTION(slice);
	ASFUNCTION(split);
	ASFUNCTION(substr);
	ASFUNCTION(substring);
	ASFUNCTION(toLowerCase);
	ASFUNCTION(toUpperCase);
	ASFUNCTION(_toString);
	ASFUNCTION(_getLength);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	number_t toNumber() const;
	int32_t toInt();
	uint32_t toUInt();
	ASFUNCTION(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
	std::string toDebugString() { return std::string("\"") + std::string(data) + "\""; }
};

template<>
inline ASObject* Class<ASString>::coerce(ASObject* o) const
{ //Special handling for Null and Undefined follows avm2overview's description of 'coerce_s' opcode
	if(o->is<Null>())
		return o;
	if(o->is<Undefined>())
	{
		o->decRef();
		return new Null;
	}
	tiny_string n = o->toString();
	o->decRef();
	return Class<ASString>::getInstanceS(n);
}

}
#endif /* ASSTRING_H_ */
