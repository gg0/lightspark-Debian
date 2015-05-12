/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013 Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/display/BitmapData.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/filters/flashfilters.h"
#include "backends/rendering_context.h"

using namespace lightspark;
using namespace std;

BitmapData::BitmapData(Class_base* c):ASObject(c),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),transparent(true)
{
}

BitmapData::BitmapData(Class_base* c, _R<BitmapContainer> b):ASObject(c),pixels(b),locked(0),transparent(true)
{
}

BitmapData::BitmapData(Class_base* c, const BitmapData& other)
  : ASObject(c),pixels(other.pixels),locked(other.locked),transparent(other.transparent)
{
}

BitmapData::BitmapData(Class_base* c, uint32_t width, uint32_t height)
 : ASObject(c),pixels(_MR(new BitmapContainer(c->memoryAccount))),locked(0),transparent(true)
{
	uint32_t *pixelArray=new uint32_t[width*height];
	if (width!=0 && height!=0)
	{
		memset(pixelArray,0,width*height*sizeof(uint32_t));
		pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	}
}

void BitmapData::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass());
	c->setDeclaredMethodByQName("draw","",Class<IFunction>::getFunction(draw),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel","",Class<IFunction>::getFunction(getPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixel32","",Class<IFunction>::getFunction(getPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel","",Class<IFunction>::getFunction(setPixel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixel32","",Class<IFunction>::getFunction(setPixel32),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyPixels","",Class<IFunction>::getFunction(copyPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fillRect","",Class<IFunction>::getFunction(fillRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("generateFilterRect","",Class<IFunction>::getFunction(generateFilterRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scroll","",Class<IFunction>::getFunction(scroll),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyChannel","",Class<IFunction>::getFunction(copyChannel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lock","",Class<IFunction>::getFunction(lock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unlock","",Class<IFunction>::getFunction(unlock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("floodFill","",Class<IFunction>::getFunction(floodFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("histogram","",Class<IFunction>::getFunction(histogram),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getColorBoundsRect","",Class<IFunction>::getFunction(getColorBoundsRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPixels","",Class<IFunction>::getFunction(getPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVector","",Class<IFunction>::getFunction(getVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPixels","",Class<IFunction>::getFunction(setPixels),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVector","",Class<IFunction>::getFunction(setVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(colorTransform),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("compare","",Class<IFunction>::getFunction(compare),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("applyFilter","",Class<IFunction>::getFunction(applyFilter),NORMAL_METHOD,true);

	// properties
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rect","",Class<IFunction>::getFunction(getRect),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	REGISTER_GETTER(c,transparent);

	IBitmapDrawable::linkTraits(c);
}

void BitmapData::addUser(Bitmap* b)
{
	users.insert(b);
}

void BitmapData::removeUser(Bitmap* b)
{
	users.erase(b);
}

void BitmapData::notifyUsers() const
{
	if (locked > 0)
		return;

	for(auto it=users.begin();it!=users.end();it++)
		(*it)->updatedData();
}

ASFUNCTIONBODY(BitmapData,_constructor)
{
	int32_t width;
	int32_t height;
	bool transparent;
	uint32_t fillColor;
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	ARG_UNPACK(width, 0)(height, 0)(transparent, true)(fillColor, 0xFFFFFFFF);

	ASObject::_constructor(obj,NULL,0);
	//If the bitmap is already initialized, just return
	if(width==0 || height==0 || !th->pixels->isEmpty())
		return NULL;
	if(width<0 || height<0)
		throw Class<ArgumentError>::getInstanceS("invalid height or width", kInvalidArgumentError);
	if(width>8191 || height>8191)
		throw Class<ArgumentError>::getInstanceS("invalid height or width", kInvalidArgumentError);

	uint32_t *pixelArray=new uint32_t[width*height];
	uint32_t c=GUINT32_TO_BE(fillColor); // fromRGB expects big endian data
	if(!transparent)
	{
		uint8_t *alpha=reinterpret_cast<uint8_t *>(&c);
		*alpha=0xFF;
	}
	for(uint32_t i=0; i<width*height; i++)
		pixelArray[i]=c;
	th->pixels->fromRGB(reinterpret_cast<uint8_t *>(pixelArray), width, height, BitmapContainer::ARGB32);
	th->transparent=transparent;

	return NULL;
}

ASFUNCTIONBODY_GETTER(BitmapData, transparent);

ASFUNCTIONBODY(BitmapData,dispose)
{
	BitmapData* th = obj->as<BitmapData>();
	th->pixels.reset();
	th->notifyUsers();
	return NULL;
}

void BitmapData::drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix)
{
	//Create an InvalidateQueue to store all the hierarchy of objects that must be drawn
	SoftwareInvalidateQueue queue;
	d->requestInvalidation(&queue);
	CairoRenderContext ctxt(pixels->getData(), pixels->getWidth(), pixels->getHeight());
	for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
	{
		DisplayObject* target=(*it).getPtr();
		//Get the drawable from each of the added objects
		IDrawable* drawable=target->invalidate(d, initialMatrix);
		if(drawable==NULL)
			continue;

		//Compute the matrix for this object
		uint8_t* buf=drawable->getPixelBuffer();
		//Construct a CachedSurface using the data
		CachedSurface& surface=ctxt.allocateCustomSurface(target,buf);
		surface.tex.width=drawable->getWidth();
		surface.tex.height=drawable->getHeight();
		surface.xOffset=drawable->getXOffset();
		surface.yOffset=drawable->getYOffset();
		delete drawable;
	}
	d->Render(ctxt);
}

ASFUNCTIONBODY(BitmapData,draw)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	_NR<ASObject> drawable;
	_NR<Matrix> matrix;
	_NR<ColorTransform> ctransform;
	_NR<ASString> blendMode;
	_NR<Rectangle> clipRect;
	bool smoothing;
	ARG_UNPACK (drawable) (matrix, NullRef) (ctransform, NullRef) (blendMode, NullRef)
					(clipRect, NullRef) (smoothing, false);

	if(!drawable->getClass() || !drawable->getClass()->isSubClass(InterfaceClass<IBitmapDrawable>::getClass()) )
		throwError<TypeError>(kCheckTypeFailedError, 
				      drawable->getClassName(),
				      "IBitmapDrawable");

	if(!ctransform.isNull() || !blendMode.isNull() || !clipRect.isNull() || smoothing)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support many parameters");

	if(drawable->is<BitmapData>())
	{
		BitmapData* data=drawable->as<BitmapData>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		CairoRenderContext ctxt(th->pixels->getData(), th->pixels->getWidth(), th->pixels->getHeight());
		//Blit the data while transforming it
		ctxt.transformedBlit(initialMatrix, data->pixels->getData(),
				data->pixels->getWidth(), data->pixels->getHeight(),
				CairoRenderContext::FILTER_NONE);
	}
	else if(drawable->is<DisplayObject>())
	{
		DisplayObject* d=drawable->as<DisplayObject>();
		//Compute the initial matrix, if any
		MATRIX initialMatrix;
		if(!matrix.isNull())
			initialMatrix=matrix->getMATRIX();
		th->drawDisplayObject(d, initialMatrix);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.draw does not support " << drawable->toDebugString());

	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,getPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y);
	return abstract_ui(pix & 0xffffff);
}

ASFUNCTIONBODY(BitmapData,getPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	ARG_UNPACK(x)(y);

	uint32_t pix=th->pixels->getPixel(x, y);
	return abstract_ui(pix);
}

ASFUNCTIONBODY(BitmapData,setPixel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);

	th->pixels->setPixel(x, y, color, false);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,setPixel32)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK(x)(y)(color);

	th->pixels->setPixel(x, y, color, th->transparent);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,getRect)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	return rect;
}

ASFUNCTIONBODY(BitmapData,_getHeight)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	return abstract_i(th->getHeight());
}

ASFUNCTIONBODY(BitmapData,_getWidth)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	return abstract_i(th->getWidth());
}

ASFUNCTIONBODY(BitmapData,fillRect)
{
	BitmapData* th=obj->as<BitmapData>();
	_NR<Rectangle> rect;
	uint32_t color;
	ARG_UNPACK(rect)(color);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	th->pixels->fillRectangle(rect->getRect(), color, th->transparent);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,copyPixels)
{
	BitmapData* th=obj->as<BitmapData>();
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapData> alphaBitmapData;
	_NR<Point> alphaPoint;
	bool mergeAlpha;
	ARG_UNPACK(source)(sourceRect)(destPoint)(alphaBitmapData, NullRef)(alphaPoint, NullRef)(mergeAlpha,false);

	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	if (source.isNull())
		throwError<TypeError>(kNullPointerError, "source");
	if (sourceRect.isNull())
		throwError<TypeError>(kNullPointerError, "sourceRect");
	if (destPoint.isNull())
		throwError<TypeError>(kNullPointerError, "destPoint");

	if(!alphaBitmapData.isNull())
		LOG(LOG_NOT_IMPLEMENTED, "BitmapData.copyPixels doesn't support alpha bitmap");

	th->pixels->copyRectangle(source->pixels, sourceRect->getRect(),
				  destPoint->getX(), destPoint->getY(),
				  mergeAlpha);
	th->notifyUsers();

	return NULL;
}

ASFUNCTIONBODY(BitmapData,generateFilterRect)
{
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData::generateFilterRect is just a stub");
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	Rectangle *rect=Class<Rectangle>::getInstanceS();
	rect->width=th->pixels->getWidth();
	rect->height=th->pixels->getHeight();
	return rect;
}

ASFUNCTIONBODY(BitmapData,hitTest)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	_NR<Point> firstPoint;
	uint32_t firstAlphaThreshold;
	_NR<ASObject> secondObject;
	_NR<Point> secondBitmapDataPoint;
	uint32_t secondAlphaThreshold;
	ARG_UNPACK (firstPoint) (firstAlphaThreshold) (secondObject) (secondBitmapDataPoint, NullRef)
					(secondAlphaThreshold,1);

	if(!secondObject->getClass() || !secondObject->getClass()->isSubClass(Class<Point>::getClass()))
		throwError<TypeError>(kCheckTypeFailedError, 
				      secondObject->getClassName(),
				      "Point");

	if(!secondBitmapDataPoint.isNull() || secondAlphaThreshold!=1)
		LOG(LOG_NOT_IMPLEMENTED,"BitmapData.hitTest does not expect some parameters");

	Point* secondPoint = secondObject->as<Point>();

	uint32_t pix=th->pixels->getPixel(secondPoint->getX()-firstPoint->getX(), secondPoint->getY()-firstPoint->getY());
	if((pix>>24)>=firstAlphaThreshold)
		return abstract_b(true);
	else
		return abstract_b(false);
}

ASFUNCTIONBODY(BitmapData,scroll)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	int x;
	int y;
	ARG_UNPACK (x) (y);

	if (th->pixels->scroll(x, y))
		th->notifyUsers();

	return NULL;
}

ASFUNCTIONBODY(BitmapData,clone)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	return Class<BitmapData>::getInstanceS(*th);
}

ASFUNCTIONBODY(BitmapData,copyChannel)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	
	_NR<BitmapData> source;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	uint32_t sourceChannel;
	uint32_t destChannel;
	ARG_UNPACK (source) (sourceRect) (destPoint) (sourceChannel) (destChannel);

	if (source.isNull())
		throwError<TypeError>(kNullPointerError, "source");
	if (sourceRect.isNull())
		throwError<TypeError>(kNullPointerError, "sourceRect");
	if (destPoint.isNull())
		throwError<TypeError>(kNullPointerError, "destPoint");

	unsigned int sourceShift = BitmapDataChannel::channelShift(sourceChannel);
	unsigned int destShift = BitmapDataChannel::channelShift(destChannel);

	RECT clippedSourceRect;
	int32_t clippedDestX;
	int32_t clippedDestY;
	th->pixels->clipRect(source->pixels, sourceRect->getRect(),
			     destPoint->getX(), destPoint->getY(),
			     clippedSourceRect, clippedDestX, clippedDestY);
	int regionWidth = clippedSourceRect.Xmax - clippedSourceRect.Xmin;
	int regionHeight = clippedSourceRect.Ymax - clippedSourceRect.Ymin;

	if (regionWidth < 0 || regionHeight < 0)
		return NULL;

	uint32_t constantChannelsMask = ~(0xFF << destShift);
	for (int32_t y=0; y<regionHeight; y++)
	{
		for (int32_t x=0; x<regionWidth; x++)
		{
			int32_t sx = clippedSourceRect.Xmin+x;
			int32_t sy = clippedSourceRect.Ymin+y;
			uint32_t sourcePixel = source->pixels->getPixel(sx, sy);
			uint32_t channel = (sourcePixel >> sourceShift) & 0xFF;
			uint32_t destChannelValue = channel << destShift;

			int32_t dx = clippedDestX + x;
			int32_t dy = clippedDestY + y;
			uint32_t oldPixel = th->pixels->getPixel(dx, dy);
			uint32_t newColor = (oldPixel & constantChannelsMask) | destChannelValue;
			th->pixels->setPixel(dx, dy, newColor, true);
		}
	}

	th->notifyUsers();

	return NULL;
}

ASFUNCTIONBODY(BitmapData,lock)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	th->locked++;
	return NULL;
}

ASFUNCTIONBODY(BitmapData,unlock)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	if (th->locked > 0)
	{
		th->locked--;
		if (th->locked == 0)
			th->notifyUsers();
	}
		
	return NULL;
}

ASFUNCTIONBODY(BitmapData,floodFill)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	int32_t x;
	int32_t y;
	uint32_t color;
	ARG_UNPACK (x) (y) (color);

	if (!th->transparent)
		color = 0xFF000000 | color;

	th->pixels->floodFill(x, y, color);
	th->notifyUsers();
	return NULL;
}

ASFUNCTIONBODY(BitmapData,histogram)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	_NR<Rectangle> inputRect;
	ARG_UNPACK (inputRect);

	RECT rect;
	if (inputRect.isNull()) {
		rect = RECT(0, th->getWidth(), 0, th->getHeight());
	} else {
		th->pixels->clipRect(inputRect->getRect(), rect);
	}

	unsigned int counts[4][256] = {{0}};
	for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
	{
		for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);
			for (int i=0; i<4; i++)
			{
				counts[i][(pixel >> (8*i)) & 0xFF]++;
			}
		}
	}

	Vector *result = Template<Vector>::getInstanceS(Template<Vector>::getTemplateInstance(Class<Number>::getClass()).getPtr());
	int channelOrder[4] = {2, 1, 0, 3}; // red, green, blue, alpha
	for (int j=0; j<4; j++)
	{
		Vector *histogram = Template<Vector>::getInstanceS(Class<Number>::getClass());
		for (int level=0; level<256; level++)
		{
			histogram->append(abstract_d(counts[channelOrder[j]][level]));
		}
		result->append(histogram);
	}

	return result;
}

ASFUNCTIONBODY(BitmapData,getColorBoundsRect)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);

	uint32_t mask;
	uint32_t color;
	bool findColor;
	ARG_UNPACK (mask) (color) (findColor, true);

	int xmin = th->getWidth();
	int xmax = 0;
	int ymin = th->getHeight();
	int ymax = 0;
	for (int32_t x=0; x<th->getWidth(); x++)
	{
		for (int32_t y=0; y<th->getHeight(); y++)
		{
			uint32_t pixel = th->pixels->getPixel(x, y);
			if ((findColor && ((pixel & mask) == color)) ||
			    (!findColor && ((pixel & mask) != color)))
			{
				if (x < xmin)
					xmin = x;
				if (x > xmax)
					xmax = x;
				if (y < ymin)
					ymin = y;
				if (y > ymax)
					ymax = y;
			}
		}
	}

	Rectangle *bounds = Class<Rectangle>::getInstanceS();
	if ((xmin <= xmax) && (ymin <= ymax))
	{
		bounds->x = xmin;
		bounds->y = ymin;
		bounds->width = xmax - xmin + 1;
		bounds->height = ymax - ymin + 1;
	}
	return bounds;
}

ASFUNCTIONBODY(BitmapData,getPixels)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	
	_NR<Rectangle> rect;
	ARG_UNPACK (rect);

	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	ByteArray *ba = Class<ByteArray>::getInstanceS();
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect->getRect());
	vector<uint32_t>::const_iterator it;
	for (it=pixelvec.begin(); it!=pixelvec.end(); ++it)
		ba->writeUnsignedInt(ba->endianIn(*it));
	return ba;
}

ASFUNCTIONBODY(BitmapData,getVector)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	
	_NR<Rectangle> rect;
	ARG_UNPACK (rect);

	if (rect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");

	Vector *result = Template<Vector>::getInstanceS(Class<UInteger>::getClass());
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect->getRect());
	vector<uint32_t>::const_iterator it;
	for (it=pixelvec.begin(); it!=pixelvec.end(); ++it)
		result->append(abstract_ui(*it));
	return result;
}

ASFUNCTIONBODY(BitmapData,setPixels)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	
	_NR<Rectangle> inputRect;
	_NR<ByteArray> inputByteArray;
	ARG_UNPACK (inputRect) (inputByteArray);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputByteArray.isNull())
		throwError<TypeError>(kNullPointerError, "inputByteArray");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			uint32_t pixel;
			if (!inputByteArray->readUnsignedInt(pixel))
				throwError<EOFError>(kEOFError);
			th->pixels->setPixel(x, y, pixel, th->transparent);
		}
	}

	return NULL;
}

ASFUNCTIONBODY(BitmapData,setVector)
{
	BitmapData* th = obj->as<BitmapData>();
	if(th->pixels.isNull())
		throw Class<ArgumentError>::getInstanceS("Disposed BitmapData", 2015);
	
	_NR<Rectangle> inputRect;
	_NR<Vector> inputVector;
	ARG_UNPACK (inputRect) (inputVector);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputVector.isNull())
		throwError<TypeError>(kNullPointerError, "inputVector");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			if (i >= inputVector->size())
				throwError<RangeError>(kParamRangeError);

			uint32_t pixel = inputVector->at(i)->toUInt();
			th->pixels->setPixel(x, y, pixel, th->transparent);
			i++;
		}
	}

	return NULL;
}

ASFUNCTIONBODY(BitmapData,colorTransform)
{
	BitmapData* th = obj->as<BitmapData>();
	
	_NR<Rectangle> inputRect;
	_NR<ColorTransform> inputColorTransform;
	ARG_UNPACK (inputRect) (inputColorTransform);

	if (inputRect.isNull())
		throwError<TypeError>(kNullPointerError, "rect");
	if (inputColorTransform.isNull())
		throwError<TypeError>(kNullPointerError, "inputVector");

	RECT rect;
	th->pixels->clipRect(inputRect->getRect(), rect);
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect);

	unsigned int i = 0;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{

			uint32_t pixel = pixelvec[i];

			int a, r, g, b;
			a = ((pixel >> 24 )&0xff) * inputColorTransform->alphaMultiplier + inputColorTransform->alphaOffset;
			if (a > 255) a = 255;
			if (a < 0) a = 0;
			r = ((pixel >> 16 )&0xff) * inputColorTransform->redMultiplier + inputColorTransform->redOffset;
			if (r > 255) r = 255;
			if (r < 0) r = 0;
			g = ((pixel >> 8 )&0xff) * inputColorTransform->greenMultiplier + inputColorTransform->greenOffset;
			if (g > 255) g = 255;
			if (g < 0) g = 0;
			b = ((pixel )&0xff) * inputColorTransform->blueMultiplier + inputColorTransform->blueOffset;
			if (b > 255) b = 255;
			if (b < 0) b = 0;
			
			pixel = (a<<24) | (r<<16) | (g<<8) | b;
			
			th->pixels->setPixel(x, y, pixel, th->transparent);
			i++;
		}
	}

	return NULL;
}
ASFUNCTIONBODY(BitmapData,compare)
{
	BitmapData* th = obj->as<BitmapData>();
	
	_NR<BitmapData> otherBitmapData;
	ARG_UNPACK (otherBitmapData);

	if (otherBitmapData.isNull())
		throwError<TypeError>(kNullPointerError, "otherBitmapData");

	if (th->getWidth() != otherBitmapData->getWidth())
		return abstract_d(-3);
	if (th->getHeight() != otherBitmapData->getHeight())
		return abstract_d(-4);
	RECT rect;
	rect.Xmin = 0;
	rect.Xmax = th->getWidth();
	rect.Ymin = 0;
	rect.Ymax = th->getHeight();
	
	vector<uint32_t> pixelvec = th->pixels->getPixelVector(rect);
	vector<uint32_t> otherpixelvec = otherBitmapData->pixels->getPixelVector(rect);
	
	BitmapData* res = Class<BitmapData>::getInstanceS(rect.Xmax,rect.Ymax);
	unsigned int i = 0;
	bool different = false;
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{

			uint32_t pixel = pixelvec[i];
			uint32_t otherpixel = otherpixelvec[i];
			if (pixel == otherpixel)
				res->pixels->setPixel(x, y, 0, true);
			else if ((pixel & 0x00FFFFFF) == (otherpixel & 0x00FFFFFF))
			{
				different = true;
				res->pixels->setPixel(x, y, ((pixel & 0xFF000000) - (otherpixel & 0xFF000000)) | 0x00FFFFFF , true);
			}
			else 
			{
				different = true;
				res->pixels->setPixel(x, y, ((pixel & 0x00FFFFFF) - (otherpixel & 0x00FFFFFF)), true);
			}
			i++;
		}
	}
	if (!different)
		return abstract_d(0);
	return res;
}

ASFUNCTIONBODY(BitmapData,applyFilter)
{
	BitmapData* th = obj->as<BitmapData>();
	
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<BitmapFilter> filter;
	ARG_UNPACK (sourceBitmapData)(sourceRect)(destPoint)(filter);
	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.applyFilter not implemented");
	return NULL;
}
