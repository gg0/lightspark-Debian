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

#ifndef URLSTREAM_H_
#define URLSTREAM_H_

#include "compat.h"
#include "asobject.h"
#include "flash/events/flashevents.h"
#include "thread_pool.h"
#include "backends/netutils.h"

namespace lightspark
{

class URLStream;

class URLStreamThread : public DownloaderThreadBase
{
private:
	_R<URLStream> loader;
	_R<ByteArray> data;
	void execute();
public:
	URLStreamThread(_R<URLRequest> request, _R<URLStream> ldr, _R<ByteArray> bytes);
};

class URLStream: public EventDispatcher, public IDataInput, public IDownloaderThreadListener
{
private:
	URLInfo url;
	_NR<ByteArray> data;
	URLStreamThread *job;
	Spinlock spinlock;
	void finalize();
	ASFUNCTION(_constructor);
	ASFUNCTION(_getEndian);
	ASFUNCTION(_setEndian);
	ASFUNCTION(_getObjectEncoding);
	ASFUNCTION(_setObjectEncoding);
	ASFUNCTION(load);
	ASFUNCTION(close);
	ASFUNCTION(bytesAvailable);
	ASFUNCTION(readBoolean);
	ASFUNCTION(readByte);
	ASFUNCTION(readBytes);
	ASFUNCTION(readDouble);
	ASFUNCTION(readFloat);
	ASFUNCTION(readInt);
	ASFUNCTION(readMultiByte);
	ASFUNCTION(readObject);
	ASFUNCTION(readShort);
	ASFUNCTION(readUnsignedByte);
	ASFUNCTION(readUnsignedInt);
	ASFUNCTION(readUnsignedShort);
	ASFUNCTION(readUTF);
	ASFUNCTION(readUTFBytes);
public:
	URLStream(Class_base* c):EventDispatcher(c),data(_MNR(Class<ByteArray>::getInstanceS())) {}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void threadFinished(IThreadJob *job);
};

}
#endif /* URLSTREAM_H_ */
