/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef ASOBJECT_H
#define ASOBJECT_H 1

#include "compat.h"
#include "swftypes.h"
#include "smartrefs.h"
#include "threading.h"
#include "memory_support.h"
#include <map>
#include <boost/intrusive/list.hpp>

#define ASFUNCTION(name) \
	static ASObject* name(ASObject* , ASObject* const* args, const unsigned int argslen)

/* declare setter/getter and associated member variable */
#define ASPROPERTY_GETTER(type,name) \
	type name; \
	ASFUNCTION( _getter_##name)

#define ASPROPERTY_SETTER(type,name) \
	type name; \
	ASFUNCTION( _setter_##name)

#define ASPROPERTY_GETTER_SETTER(type, name) \
	type name; \
	ASFUNCTION( _getter_##name); \
	ASFUNCTION( _setter_##name)

/* declare setter/getter for already existing member variable */
#define ASFUNCTION_GETTER(name) \
	ASFUNCTION( _getter_##name)

#define ASFUNCTION_SETTER(name) \
	ASFUNCTION( _setter_##name)

#define ASFUNCTION_GETTER_SETTER(name) \
	ASFUNCTION( _getter_##name); \
	ASFUNCTION( _setter_##name)

/* general purpose body for an AS function */
#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

/* full body for a getter declared by ASPROPERTY_GETTER or ASFUNCTION_GETTER */
#define ASFUNCTIONBODY_GETTER(c,name) \
	ASObject* c::_getter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS("Arguments provided in getter"); \
		return ArgumentConversion<decltype(th->name)>::toAbstract(th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER */
#define ASFUNCTIONBODY_SETTER(c,name) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		return NULL; \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER.
 * After the property has been updated, the callback member function is called with the old value
 * as parameter */
#define ASFUNCTIONBODY_SETTER_CB(c,name,callback) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		th->callback(oldValue); \
		return NULL; \
	}

/* full body for a getter declared by ASPROPERTY_GETTER_SETTER or ASFUNCTION_GETTER_SETTER */
#define ASFUNCTIONBODY_GETTER_SETTER(c,name) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_CB(c,name,callback) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER_CB(c,name,callback)

/* registers getter/setter with Class_base. To be used in ::sinit()-functions */
#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_getter_##name),GETTER_METHOD,true)

#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

#define CLASS_DYNAMIC_NOT_FINAL 0
#define CLASS_FINAL 1
#define CLASS_SEALED 2

// TODO: Every class should have a constructor
#define CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes) \
	c->setSuper(Class<superClass>::getRef()); \
	c->setConstructor(NULL); \
	c->isFinal = ((attributes) & CLASS_FINAL) != 0;	\
	c->isSealed = ((attributes) & CLASS_SEALED) != 0

#define CLASS_SETUP(c, superClass, constructor, attributes) \
	CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes); \
	c->setConstructor(Class<IFunction>::getFunction(constructor));

#define CLASS_SETUP_CONSTRUCTOR_LENGTH(c, superClass, constructor, ctorlength, attributes) \
	CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes); \
	c->setConstructor(Class<IFunction>::getFunction((constructor), (ctorlength)));

namespace lightspark
{

class ASObject;
class IFunction;
template<class T> class Class;
class Class_base;
class ByteArray;
class Loader;
class Type;
class ABCContext;

enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, INSTANCE_TRAIT=5, CONSTANT_TRAIT=9 /* constants are also declared traits */ };
enum TRAIT_STATE { NO_STATE=0, HAS_GETTER_SETTER=1, TYPE_RESOLVED=2 };

struct variable
{
	ASObject* var;
	union
	{
		multiname* traitTypemname;
		const Type* type;
		void* typeUnion;
	};
	IFunction* setter;
	IFunction* getter;
	TRAIT_KIND kind;
	TRAIT_STATE traitState;
	bool isenumerable:1;
	variable(TRAIT_KIND _k)
		: var(NULL),typeUnion(NULL),setter(NULL),getter(NULL),kind(_k),traitState(NO_STATE),isenumerable(true) {}
	variable(TRAIT_KIND _k, ASObject* _v, multiname* _t, const Type* type);
	void setVar(ASObject* v);
	/*
	 * To be used only if the value is guaranteed to be of the right type
	 */
	void setVarNoCoerce(ASObject* v);
};

struct varName
{
	uint32_t nameId;
	nsNameAndKind ns;
	varName(uint32_t name, const nsNameAndKind& _ns):nameId(name),ns(_ns){}
	bool operator<(const varName& r) const
	{
		//Sort by name first
		if(nameId==r.nameId)
		{
			//Then by namespace
			return ns<r.ns;
		}
		else
			return nameId<r.nameId;
	}
};

class variables_map
{
public:
	//Names are represented by strings in the string and namespace pools
	typedef std::map<varName,variable,std::less<varName>,reporter_allocator<std::pair<const varName, variable>>>
		mapType;
	mapType Variables;
	typedef std::map<varName,variable>::iterator var_iterator;
	typedef std::map<varName,variable>::const_iterator const_var_iterator;
	std::vector<var_iterator, reporter_allocator<var_iterator>> slots_vars;
	variables_map(MemoryAccount* m);
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	variable* findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds);
	variable* findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	/**
	 * Const version of findObjVar, useful when looking for getters
	 */
	const variable* findObjVar(const multiname& mname, uint32_t traitKinds, NS_KIND &nskind) const;
	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(const multiname& mname, ASObject* obj, multiname *typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj);
	void killObjVar(const multiname& mname);
	ASObject* getSlot(unsigned int n)
	{
		assert_and_throw(n > 0 && n<=slots_vars.size());
		return slots_vars[n-1]->second.var;
	}
	/*
	 * This method does throw if the slot id is not valid
	 */
	void validateSlotId(unsigned int n) const;
	void setSlot(unsigned int n,ASObject* o);
	/*
	 * This version of the call is guarantee to require no type conversion
	 * this is verified at optimization time
	 */
	void setSlotNoCoerce(unsigned int n,ASObject* o);
	void initSlot(unsigned int n, uint32_t nameId, const nsNameAndKind& ns);
	void appendSlot(uint32_t nameId, const nsNameAndKind& ns)
	{
		initSlot(slots_vars.size()+1, nameId, ns);
	}
	int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(unsigned int i) const;
	variable* getValueAt(unsigned int i);
	int getNextEnumerable(unsigned int i) const;
	~variables_map();
	void check() const;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap) const;
	void dumpVariables();
	void destroyContents();
};

enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };
//for toPrimitive
enum TP_HINT { NO_HINT, NUMBER_HINT, STRING_HINT };

class ASObject: public memory_reporter, public boost::intrusive::list_base_hook<>, public RefCountable
{
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend void lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs);
friend class IFunction; //Needed for clone
private:
	variables_map Variables;
	Class_base* classdef;
	const variable* findGettable(const multiname& name, NS_KIND &nskind) const DLL_LOCAL;
	variable* findSettable(const multiname& name, bool* has_getter=NULL) DLL_LOCAL;
	multiname* proxyMultiName;
protected:
	ASObject(MemoryAccount* m);
	ASObject(const ASObject& o);
	virtual ~ASObject();
	SWFOBJECT_TYPE type;
	bool traitsInitialized:1;
	bool constructIndicator:1;
	void serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t> traitsMap) const;
	void setClass(Class_base* c);
	static variable* findSettableImpl(variables_map& map, const multiname& name, bool* has_getter);
	static const variable* findGettableImpl(const variables_map& map, const multiname& name, NS_KIND &nskind);
public:
	ASObject(Class_base* c);
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized:1;
#endif
	bool implEnable:1;
	Class_base* getClass() const { return classdef; }
	ASFUNCTION(_constructor);
	// constructor for subclasses that can't be instantiated.
	// Throws ArgumentError.
	ASFUNCTION(_constructorNotInstantiatable);
	ASFUNCTION(_toString);
	ASFUNCTION(_toLocaleString);
	ASFUNCTION(hasOwnProperty);
	ASFUNCTION(valueOf);
	ASFUNCTION(isPrototypeOf);
	ASFUNCTION(propertyIsEnumerable);
	ASFUNCTION(setPropertyIsEnumerable);
	void check() const;
	static void s_incRef(ASObject* o)
	{
		o->incRef();
	}
	static void s_decRef(ASObject* o)
	{
		if(o)
			o->decRef();
	}
	static void s_decRef_safe(ASObject* o,ASObject* o2)
	{
		if(o && o!=o2)
			o->decRef();
	}
	/*
	   The finalize function should be implemented in all derived class that stores pointers.
	   It should decRef all referenced objects. It's guaranteed that the only operations
	   that will happen on the object after finalization are decRef and delete.
	   Each class must call BaseClass::finalize in their finalize function. 
	   The finalize method must be callable multiple time with the same effects (no double frees).
	   Each class must also call his own ::finalize in the destructor!*/
	virtual void finalize();

	enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, XML_STRICT=0x02};

	virtual _NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return getVariableByMultiname(name,opt,classdef);
	}
	/*
	 * Helper method using the get the raw variable struct instead of calling the getter.
	 * It is used by getVariableByMultiname and by early binding code
	 */
	const variable* findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, NS_KIND &nskind);
	/*
	 * Gets a variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a getter, it is called and its return value returned.
	 */
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls);
	virtual int32_t getVariableByMultiname_i(const multiname& name);
	/* Simple getter interface for the common case */
	_NR<ASObject> getVariableByMultiname(const tiny_string& name, std::list<tiny_string> namespaces);
	/*
	 * Execute a AS method on this object. Returns the value
	 * returned by the function. One reference of each args[i] is
	 * consumed. The method must exist, otherwise a TypeError is
	 * thrown.
	 */
	_NR<ASObject> executeASMethod(const tiny_string& methodName, std::list<tiny_string> namespaces, ASObject* const* args, uint32_t num_args);
	virtual void setVariableByMultiname_i(const multiname& name, int32_t value);
	enum CONST_ALLOWED_FLAG { CONST_ALLOWED=0, CONST_NOT_ALLOWED };
	virtual void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
	{
		setVariableByMultiname(name,o,allowConst,classdef);
	}
	/*
	 * Sets  variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a setter, it is called with the given 'o'.
	 * If no property is found, an instance variable is created.
	 * Setting CONSTANT_TRAIT is only allowed if allowConst is true
	 */
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst, Class_base* cls);
	/*
	 * Called by ABCVm::buildTraits to create DECLARED_TRAIT or CONSTANT_TRAIT and set their type
	 */
	void initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname,
			ABCContext* context, TRAIT_KIND traitKind,bool bOverwrite);
	/*
	 * Called by ABCVm::initProperty (implementation of ABC instruction), it is allowed to set CONSTANT_TRAIT
	 */
	void initializeVariableByMultiname(const multiname& name, ASObject* o);
	virtual bool deleteVariableByMultiname(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed);
	void setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	ASObject* getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	void setSlot(unsigned int n,ASObject* o)
	{
		Variables.setSlot(n,o);
	}
	void setSlotNoCoerce(unsigned int n,ASObject* o)
	{
		Variables.setSlotNoCoerce(n,o);
	}
	void initSlot(unsigned int n, const multiname& name);
	void appendSlot(const multiname& name);
	unsigned int numVariables() const;
	tiny_string getNameAt(int i) const
	{
		return Variables.getNameAt(i);
	}
	_R<ASObject> getValueAt(int i);
	SWFOBJECT_TYPE getObjectType() const
	{
		return type;
	}
	/* Implements ECMA's 9.8 ToString operation, but returns the concrete value */
	tiny_string toString();
	tiny_string toLocaleString();
	virtual int32_t toInt();
	virtual uint32_t toUInt();
	uint16_t toUInt16();
	/* Implements ECMA's 9.3 ToNumber operation, but returns the concrete value */
	number_t toNumber();
	/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
	_R<ASObject> toPrimitive(TP_HINT hint = NO_HINT);
	bool isPrimitive() const;

	bool isInitialized() const {return traitsInitialized;}
	virtual bool isConstructed() const;
	
	/* helper functions for calling the "valueOf" and
	 * "toString" AS-functions which may be members of this
	 *  object */
	bool has_valueOf();
	_R<ASObject> call_valueOf();
	bool has_toString();
	_R<ASObject> call_toString();
	bool has_toJSON();
	tiny_string call_toJSON();

	/* Helper function for calling getClass()->getQualifiedClassName() */
	virtual tiny_string getClassName() const;

	ASFUNCTION(generator);

	/* helpers for the dynamic property 'prototype' */
	bool hasprop_prototype();
	ASObject* getprop_prototype();
	void setprop_prototype(_NR<ASObject>& prototype);

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual bool isEqualStrict(ASObject* r);
	virtual TRISTATE isLess(ASObject* r);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);

	//Enumeration handling
	virtual uint32_t nextNameIndex(uint32_t cur_index);
	virtual _R<ASObject> nextName(uint32_t index);
	virtual _R<ASObject> nextValue(uint32_t index);

	//Called when the object construction is completed. Used by MovieClip implementation
	virtual void constructionComplete();

	/**
	  Serialization interface

	  The various maps are used to implement reference type of the AMF3 spec
	*/
	virtual void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);

	virtual ASObject *describeType() const;

	virtual tiny_string toJSON(std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces,const tiny_string& filter);
	/* returns true if the current object is of type T */
	template<class T> bool is() const { return dynamic_cast<const T*>(this); }
	/* returns this object casted to the given type.
	 * You have to make sure that it actually is the type (see is<T>() above)
	 */
	template<class T> const T* as() const { return static_cast<const T*>(this); }
	template<class T> T* as() { return static_cast<T*>(this); }

	/* Returns a debug string identifying this object */
	virtual std::string toDebugString();
	
	/* stores proxy namespace settings for internal usage */
	void setProxyProperty(const multiname& name); 
	/* applies proxy namespace settings to name for internal usage */
	void applyProxyProperty(multiname &name); 
	
	void dumpVariables() { Variables.dumpVariables(); }
	
	void setConstructIndicator() { constructIndicator = true; }
};

class Number;
class UInteger;
class Integer;
class Boolean;
class Template_base;
class ASString;
class Function;
class Array;
class Null;
class Undefined;
class Type;
template<> inline bool ASObject::is<Number>() const { return type==T_NUMBER; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<Function>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<Null>() const { return type==T_NULL; }
template<> inline bool ASObject::is<Array>() const { return type==T_ARRAY; }
template<> inline bool ASObject::is<Class_base>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<Template_base>() const { return type==T_TEMPLATE; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
}
#endif /* ASOBJECT_H */
