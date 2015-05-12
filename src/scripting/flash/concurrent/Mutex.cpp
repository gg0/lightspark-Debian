/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "scripting/flash/concurrent/Mutex.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ASMutex::ASMutex(Class_base* c):ASObject(c),lockcount(0)
{
	
}
void ASMutex::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->setDeclaredMethodByQName("lock","",Class<IFunction>::getFunction(_lock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unlock","",Class<IFunction>::getFunction(_unlock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("tryLock","",Class<IFunction>::getFunction(_trylock),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(ASMutex,_constructor)
{
	return NULL;
}
ASFUNCTIONBODY(ASMutex,_lock)
{
	ASMutex* th=obj->as<ASMutex>();
	th->mutex.lock();
	th->lockcount++;
	return NULL;
}
ASFUNCTIONBODY(ASMutex,_unlock)
{
	ASMutex* th=obj->as<ASMutex>();
	th->mutex.unlock();
	th->lockcount--;
	return NULL;
}
ASFUNCTIONBODY(ASMutex,_trylock)
{
	ASMutex* th=obj->as<ASMutex>();
	return abstract_b(th->mutex.trylock());
}

