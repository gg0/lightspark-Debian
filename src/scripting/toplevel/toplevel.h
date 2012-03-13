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

#ifndef TOPLEVEL_H
#define TOPLEVEL_H
#include "compat.h"
#include <vector>
#include <set>
#include "asobject.h"
#include "exceptions.h"
#include "threading.h"
#include "scripting/abcutils.h"
#include "Boolean.h"
#include "Error.h"
#include "XML.h"

namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class ABCContext;
class Template_base;
class ASString;
class method_info;
struct call_context;
struct traits_info;
class Any;
class Void;
/* This abstract class represents a type, i.e. something that a value can be coerced to.
 * Currently Class_base and Template_base implement this interface.
 * If you let another class implement this interface, change ASObject->is<Type>(), too!
 * You never take ownership of a type, so there is no need to incRef/decRef them.
 * Types are guaranteed to survive until SystemState::destroy().
 */
class Type
{
protected:
	/* this is private because one never deletes a Type */
	~Type() {}
public:
	static Any* const anyType;
	static Void* const voidType;
	/*
	 * This returns the Type for the given multiname.
	 * It searches for the and object of the type in global object.
	 * If no object of that name is found or it is not a Type,
	 * then an exception is thrown.
	 * The caller does not own the object returned.
	 */
	static const Type* getTypeFromMultiname(const multiname* mn);
	/*
	 * Checks if the type is already in sys->classes
	 */
	static bool isTypeResolvable(const multiname* mn);
	/*
	 * Converts the given object to an object of this type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 * If the argument cannot be converted, it throws a TypeError
	 */
	virtual ASObject* coerce(ASObject* o) const=0;

	/* Return "any" for anyType, "void" for voidType and class_name.name for Class_base */
	virtual tiny_string getName() const=0;
};
template<> inline Type* ASObject::as<Type>() { return dynamic_cast<Type*>(this); }
template<> inline const Type* ASObject::as<Type>() const { return dynamic_cast<const Type*>(this); }

class Any: public Type
{
public:
	ASObject* coerce(ASObject* o) const { return o; }
	virtual ~Any() {};
	tiny_string getName() const { return "any"; }
};

class Void: public Type
{
public:
	ASObject* coerce(ASObject* o) const;
	virtual ~Void() {};
	tiny_string getName() const { return "void"; }
};

class Class_base: public ASObject, public Type
{
friend class ABCVm;
friend class ABCContext;
template<class T> friend class Template;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	bool use_protected;
	nsNameAndKind protected_ns;
	void recursiveBuild(ASObject* target);
	IFunction* constructor;
	void describeTraits(xmlpp::Element* root, std::vector<traits_info>& traits) const;
	//Naive garbage collection until reference cycles are detected
	Mutex referencedObjectsMutex;
	std::set<ASObject*> referencedObjects;
	void finalizeObjects() const;
protected:
	void copyBorrowedTraitsFromSuper();
	ASFUNCTION(_toString);
public:
	bool isFinal:1;
	bool isSealed:1;
	void addPrototypeGetter();
	ASPROPERTY_GETTER(_NR<ASObject>,prototype);
	_NR<Class_base> super;
	//We need to know what is the context we are referring to
	ABCContext* context;
	QName class_name;
	int class_index;
	void handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	Class_base(const QName& name);
	~Class_base();
	void finalize();
	virtual ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)=0;
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces() const;
	void linkInterface(Class_base* c) const;
	/*
	 * Returns true when 'this' is a subclass of 'cls',
	 * i.e. this == cls or cls equals some super of this.
	 */
	bool isSubClass(const Class_base* cls) const;
	tiny_string getQualifiedClassName() const;
	tiny_string getName() const { return class_name.name; }
	tiny_string toString();
	virtual ASObject* generator(ASObject* const* args, const unsigned int argslen);
	ASObject *describeType() const;
	void describeInstance(xmlpp::Element* root) const;
	virtual const Template_base* getTemplate() const { return NULL; }
	//DEPRECATED: naive garbage collector
	void abandonObject(ASObject* ob) DLL_PUBLIC;
	void acquireObject(ASObject* ob) DLL_PUBLIC;
	/*
	 * Converts the given object to an object of this Class_base's type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 */
	virtual ASObject* coerce(ASObject* o) const;

	void setSuper(_R<Class_base> super_)
	{
		assert(!super);
		super = super_;
		copyBorrowedTraitsFromSuper();
	}
};

class Template_base : public ASObject
{
private:
	QName template_name;
public:
	Template_base(QName name);
	virtual Class_base* applyType(const std::vector<Type*>& t)=0;
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base(QName("Class","")){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		throw RunTimeException("Class_object::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_object::buildInstanceTraits");
	}
public:
	static Class_object* getClass();
	static _R<Class_object> getRef();
};

/* Adaptor from fuction to class, it does not seems to be a good idea to
 * derive IFunction from Class_base, because it's too heavyweight
 * This is the class of an object created by (new f()) where f is a function.
 * Its prototype points to f->prototype which is defined in IFunction.
 *
 * Class_function uses prototype based inheritance only.
 */
class Class_function: public Class_base
{
private:
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		throw RunTimeException("Class_function::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_function::buildInstanceTraits");
	}
public:
	//Name is 'Object' because trace(new f()) gives "[object Object]"
	Class_function() : Class_base(QName("Object","")) {}
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return NullRef;
	}
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		throw UnsupportedException("Class_function::getVariableByMultiname_i");
		return 0;
	}
	void setVariableByMultiname_i(const multiname& name, int32_t value)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname_i");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname");
	}
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic)
	{
		return false;
	}
};

class IFunction: public ASObject
{
CLASSBUILDABLE(IFunction);
protected:
	IFunction();
	virtual IFunction* clone()=0;
	_NR<ASObject> closure_this;
public:
	/* If this is a method, inClass is the class this is defined in.
	 * If this is a function, inClass == NULL
	 */
	Class_base* inClass;
	/* returns wether this is this a method of a function */
	bool isMethod() const { return inClass != NULL; }
	bool isBound() const { return closure_this; }
	void finalize();
	ASFUNCTION(apply);
	ASFUNCTION(_call);
	ASFUNCTION(_toString);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,prototype);
	ASPROPERTY_GETTER_SETTER(uint32_t,length);
	/*
	 * Calls this function with the given object and args.
	 * One reference of obj and each args[i] is consumed.
	 * Return the ASObject the function returned.
	 * This never returns NULL.
	 */
	virtual ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args)=0;
	IFunction* bind(_NR<ASObject> c, int level)
	{
		if(!isBound())
		{
			IFunction* ret=NULL;
			if(!c)
			{
				//If binding with null we are generated from newFunction, don't copy
				ret=this;
			}
			else
			{
				//Generate a copy
				ret=clone();
				ret->classdef=NULL; //Drop the classdef and set it ex novo
				ret->setClass(getClass());
			}
			ret->closure_this=c;
			//std::cout << "Binding " << ret << std::endl;
			return ret;
		}
		else
		{
			incRef();
			return this;
		}
	}
	virtual method_info* getMethodInfo() const=0;
	virtual ASObject *describeType() const;
};

class Function : public IFunction
{
friend class Class<IFunction>;
public:
	typedef ASObject* (*as_function)(ASObject*, ASObject* const *, const unsigned int);
private:
	as_function val;
	Function(){}
	Function(as_function v):val(v){}
	Function* clone()
	{
		return new Function(*this);
	}
	method_info* getMethodInfo() const { return NULL; }
public:
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args);
	bool isEqual(ASObject* r)
	{
		Function* f=dynamic_cast<Function*>(r);
		if(f==NULL)
			return false;
		return val==f->val;
	}
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend class Class<IFunction>;
public:
	typedef ASObject* (*synt_function)(call_context* cc);
private:
	int hit_count;
	method_info* mi;
	synt_function val;
	SyntheticFunction(method_info* m);
	SyntheticFunction* clone()
	{
		return new SyntheticFunction(*this);
	}
	method_info* getMethodInfo() const { return mi; }
public:
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args);
	void finalize();
	std::vector<scope_entry> func_scope;
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}
	void acquireScope(const std::vector<scope_entry>& scope)
	{
		assert_and_throw(func_scope.empty());
		func_scope=scope;
	}
	void addToScope(const scope_entry& s)
	{
		func_scope.emplace_back(s);
	}
};

//Specialized class for IFunction
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>():Class_base(QName("Function","")){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen);
public:
	static Class<IFunction>* getClass();
	static _R<Class<IFunction>> getRef()
	{
		Class<IFunction>* ret = getClass();
		ret->incRef();
		return _MR(ret);
	}
	static Function* getFunction(Function::as_function v)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		Function* ret=new Function(v);
		ret->setClass(c);
		return ret;
	}
	static SyntheticFunction* getSyntheticFunction(method_info* m)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		SyntheticFunction* ret=new SyntheticFunction(m);
		ret->setClass(c);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const
	{
	}
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	int32_t toInt();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	ASObject *describeType() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class Null: public ASObject
{
public:
	Null(){type=T_NULL;}
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	int32_t toInt();
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class ASQName: public ASObject
{
friend class multiname;
friend class Namespace;
CLASSBUILDABLE(ASQName);
private:
	bool uri_is_null;
	tiny_string uri;
	tiny_string local_name;
	ASQName(){type=T_QNAME; uri_is_null=false;}
public:
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getURI);
	ASFUNCTION(_getLocalName);
	ASFUNCTION(_toString);
	tiny_string getURI() const { return uri; }
	tiny_string getLocalName() const { return local_name; }
	bool isEqual(ASObject* o);
	tiny_string toString();
};

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
CLASSBUILDABLE(Namespace);
private:
	bool prefix_is_undefined;
	tiny_string uri;
	tiny_string prefix;
	Namespace(){type=T_NAMESPACE; prefix_is_undefined=false;}
	Namespace(const tiny_string& _uri):uri(_uri){type=T_NAMESPACE; prefix_is_undefined=false;}
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getURI);
	ASFUNCTION(_setURI);
	ASFUNCTION(_getPrefix);
	ASFUNCTION(_setPrefix);
	ASFUNCTION(_toString);
	ASFUNCTION(_valueOf);
	ASFUNCTION(_ECMA_valueOf);
	bool isEqual(ASObject* o);
};

class Integer : public ASObject
{
friend class Number;
friend class Array;
friend class ABCVm;
friend class ABCContext;
friend ASObject* abstract_i(int32_t i);
CLASSBUILDABLE(Integer);
private:
	Integer(int32_t v=0):val(v){type=T_INTEGER;}
public:
	int32_t val;
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(_toString);
	tiny_string toString();
	static tiny_string toString(int32_t val);
	int32_t toInt()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(generator);
	std::string toDebugString() { return toString()+"i"; }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class UInteger: public ASObject
{
friend ASObject* abstract_ui(uint32_t i);
CLASSBUILDABLE(UInteger);
private:
public:
	uint32_t val;
	UInteger(uint32_t v=0):val(v){type=T_UINTEGER;}

	static void sinit(Class_base* c);
	tiny_string toString();
	static tiny_string toString(uint32_t val);
	int32_t toInt()
	{
		return val;
	}
	uint32_t toUInt()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(generator);
	ASFUNCTION(_toString);
	std::string toDebugString() { return toString()+"ui"; }
	//CHECK: should this have a special serialization?
};

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
friend class ABCContext;
friend class ABCVm;
CLASSBUILDABLE(Number);
private:
	Number():val(0) {type=T_NUMBER;}
	Number(double v):val(v){type=T_NUMBER;}
	static void purgeTrailingZeroes(char* buf);
public:
	static const number_t NaN;
	double val;
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	tiny_string toString();
	static tiny_string toString(number_t val);
	static bool isInteger(number_t val)
	{
		return floor(val) == val;
	}
	unsigned int toUInt()
	{
		return (unsigned int)(val);
	}
	int32_t toInt()
	{
		if(val<0)
			return int(val);
		else
		{
			uint32_t ret=val;
			return ret;
		}
	}
	TRISTATE isLess(ASObject* o);
	bool isEqual(ASObject* o);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(generator);
	std::string toDebugString() { return toString()+"d"; }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class RegExp: public ASObject
{
CLASSBUILDABLE(RegExp);
friend class ASString;
private:
	RegExp();
	RegExp(const tiny_string& _re);
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASObject *match(const tiny_string& str);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(exec);
	ASFUNCTION(test);
	ASFUNCTION(_toString);
	ASPROPERTY_GETTER(bool, dotall);
	ASPROPERTY_GETTER(bool, global);
	ASPROPERTY_GETTER(bool, ignoreCase);
	ASPROPERTY_GETTER(bool, extended);
	ASPROPERTY_GETTER(bool, multiline);
	ASPROPERTY_GETTER_SETTER(int, lastIndex);
	ASPROPERTY_GETTER(tiny_string, source);
};

class Global : public ASObject
{
CLASSBUILDABLE(Global);
private:
	Global(ABCContext* c, int s): context(c), scriptId(s) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {};
	ABCContext* context;
	int scriptId;
public:
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
};

class GlobalObject
{
public:
	void registerGlobalScope(Global* scope);
	std::vector<Global*> globalScopes;
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	ASObject* getVariableAndTargetByMultiname(const multiname& name, ASObject*& target);
	~GlobalObject();
};

ASObject* eval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseInt(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseFloat(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isNaN(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isFinite(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* escape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* unescape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* print(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* trace(ASObject* obj,ASObject* const* args, const unsigned int argslen);
bool isXMLName(ASObject* obj);
ASObject* _isXMLName(ASObject* obj,ASObject* const* args, const unsigned int argslen);


inline void Manager::put(ASObject* o)
{
	if(available.size()>maxCache)
		delete o;
	else
	{
		//The Manager now owns this object
		if(o->classdef)
			o->classdef->abandonObject(o);
		available.push_back(o);
	}
}

template<class T>
T* Manager::get()
{
	if(available.size())
	{
		T* ret=static_cast<T*>(available.back());
		available.pop_back();
		ret->incRef();
		//Transfer ownership back to the classdef
		if(ret->getClass())
			ret->getClass()->acquireObject(ret);
		//std::cout << "getting[" << name << "] " << ret << std::endl;
		return ret;
	}
	else
	{
		T* ret=Class<T>::getInstanceS();
		ret->manager = this;
		//std::cout << "newing" << ret << std::endl;
		return ret;
	}
}

inline Manager::~Manager()
{
	for(auto i = available.begin(); i != available.end(); ++i)
	{
		// ~ASObject will call abandonObject() again
		if((*i)->classdef)
			(*i)->classdef->acquireObject(*i);
		delete *i;
	}
}

};

#endif