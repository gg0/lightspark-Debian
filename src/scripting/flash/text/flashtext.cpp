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

#include <libxml++/nodes/element.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/exceptions/exception.h>
#include <libxml/tree.h>
#include "scripting/flash/text/flashtext.h"
#include "scripting/class.h"
#include "compat.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void lightspark::AntiAliasType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("ADVANCED","",Class<ASString>::getInstanceS("advanced"),DECLARED_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),DECLARED_TRAIT);
}

void ASFont::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	//c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("enumerateFonts","",Class<IFunction>::getFunction(enumerateFonts),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("registerFont","",Class<IFunction>::getFunction(registerFont),NORMAL_METHOD,false);

	REGISTER_GETTER(c,fontName);
	REGISTER_GETTER(c,fontStyle);
	REGISTER_GETTER(c,fontType);
}
void ASFont::SetFont(tiny_string& fontname,bool is_bold,bool is_italic, bool is_Embedded, bool is_EmbeddedCFF)
{
	fontName = fontname;
	fontStyle = (is_bold ? 
					 (is_italic ? "boldItalic" : "bold") :
					 (is_italic ? "italic" : "regular")
					 );
	fontType = (is_Embedded ?
					(is_EmbeddedCFF ? "embeddedCFF" : "embedded") :
					"device");
}

std::vector<ASObject*>* ASFont::getFontList()
{
	static std::vector<ASObject*> fontlist;
	return &fontlist;
}
ASFUNCTIONBODY_GETTER(ASFont, fontName);
ASFUNCTIONBODY_GETTER(ASFont, fontStyle);
ASFUNCTIONBODY_GETTER(ASFont, fontType);

ASFUNCTIONBODY(ASFont,enumerateFonts)
{
	bool enumerateDeviceFonts=false;
	ARG_UNPACK(enumerateDeviceFonts,false);

	LOG(LOG_NOT_IMPLEMENTED,"Font::enumerateFonts: flag enumerateDeviceFonts is not handled");
	Array* ret = Class<Array>::getInstanceS();
	std::vector<ASObject*>* fontlist = getFontList();
	for(auto i = fontlist->begin(); i != fontlist->end(); ++i)
	{
		(*i)->incRef();
		ret->push(_MR(*i));
	}
	return ret;
}
ASFUNCTIONBODY(ASFont,registerFont)
{
	getFontList()->push_back(args[0]);
	return NULL;
}

TextField::TextField(Class_base* c, const TextData& textData, bool _selectable, bool readOnly)
	: InteractiveObject(c), TextData(textData), type(READ_ONLY), 
	  mouseWheelEnabled(true), selectable(_selectable)
{
	if (!readOnly)
	{
		type = EDITABLE;
		tabEnabled = true;
	}
}

void TextField::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<InteractiveObject>::getRef());
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(TextField::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(TextField::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",Class<IFunction>::getFunction(TextField::_getHtmlText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("htmlText","",Class<IFunction>::getFunction(TextField::_setHtmlText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("textHeight","",Class<IFunction>::getFunction(TextField::_getTextHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("textWidth","",Class<IFunction>::getFunction(TextField::_getTextWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_getText),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(TextField::_setText),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(TextField::_setWordWrap),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wordWrap","",Class<IFunction>::getFunction(TextField::_getWordWrap),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(TextField::_setAutoSize),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("autoSize","",Class<IFunction>::getFunction(TextField::_getAutoSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendText","",Class<IFunction>::getFunction(TextField:: appendText),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTextFormat","",Class<IFunction>::getFunction(_getTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextFormat","",Class<IFunction>::getFunction(_setTextFormat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLineMetrics","",Class<IFunction>::getFunction(_getLineMetrics),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(TextField::_getDefaultTextFormat),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultTextFormat","",Class<IFunction>::getFunction(TextField::_setDefaultTextFormat),SETTER_METHOD,true);

	REGISTER_GETTER_SETTER(c, background);
	REGISTER_GETTER_SETTER(c, backgroundColor);
	REGISTER_GETTER_SETTER(c, border);
	REGISTER_GETTER_SETTER(c, borderColor);
	REGISTER_GETTER_SETTER(c, multiline);
	REGISTER_GETTER_SETTER(c, mouseWheelEnabled);
	REGISTER_GETTER_SETTER(c, selectable);
	REGISTER_GETTER_SETTER(c, textColor);
	REGISTER_GETTER_SETTER(c, type);
}

ASFUNCTIONBODY_GETTER_SETTER(TextField, background);
ASFUNCTIONBODY_GETTER_SETTER(TextField, backgroundColor);
ASFUNCTIONBODY_GETTER_SETTER(TextField, border);
ASFUNCTIONBODY_GETTER_SETTER(TextField, borderColor);
ASFUNCTIONBODY_GETTER_SETTER(TextField, multiline);
ASFUNCTIONBODY_GETTER_SETTER(TextField, mouseWheelEnabled);
ASFUNCTIONBODY_GETTER_SETTER(TextField, selectable);
ASFUNCTIONBODY_GETTER_SETTER(TextField, textColor);

void TextField::buildTraits(ASObject* o)
{
}

bool TextField::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

_NR<DisplayObject> TextField::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	/* I suppose one does not have to actually hit a character */
	number_t xmin,xmax,ymin,ymax;
	boundsRect(xmin,xmax,ymin,ymax);
	if( xmin <= x && x <= xmax && ymin <= y && y <= ymax && isHittable(type))
		return last;
	else
		return NullRef;
}

ASFUNCTIONBODY(TextField,_getWordWrap)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_b(th->wordWrap);
}

ASFUNCTIONBODY(TextField,_setWordWrap)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->wordWrap=Boolean_concrete(args[0]);
	if(th->onStage)
		th->requestInvalidation(getSys());
	return NULL;
}

ASFUNCTIONBODY(TextField,_getAutoSize)
{
	TextField* th=Class<TextField>::cast(obj);
	switch(th->autoSize)
	{
		case AS_NONE:
			return Class<ASString>::getInstanceS("none");
		case AS_LEFT:
			return Class<ASString>::getInstanceS("left");
		case AS_RIGHT:
			return Class<ASString>::getInstanceS("right");
		case AS_CENTER:
			return Class<ASString>::getInstanceS("center");
	}
	return NULL;
}

ASFUNCTIONBODY(TextField,_setAutoSize)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	tiny_string temp = args[0]->toString();
	if(temp == "none")
		th->autoSize = AS_NONE;//TODO: take care of corner cases : what to do with sizes when changing the autoSize
	else if (temp == "left")
		th->autoSize = AS_LEFT;
	else if (temp == "right")
		th->autoSize = AS_RIGHT;
	else if (temp == "center")
		th->autoSize = AS_CENTER;
	else
		throw Class<ArgumentError>::getInstanceS("Wrong argument in TextField.autoSize");
	if(th->onStage)
		th->requestInvalidation(getSys());//TODO:check if there was any change
	return NULL;
}

ASFUNCTIONBODY(TextField,_getWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(TextField,_setWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	//The width needs to be updated only if autoSize is off or wordWrap is on TODO:check this, adobe's behavior is not clear
	if((th->autoSize == AS_NONE)||(th->wordWrap == true))
	{
		th->width=args[0]->toInt();
		if(th->onStage)
			th->requestInvalidation(getSys());
		else
			th->updateSizes();
	}
	return NULL;
}

ASFUNCTIONBODY(TextField,_getHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(TextField,_setHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	if(th->autoSize == AS_NONE)
	{
		th->height=args[0]->toInt();
		if(th->onStage)
			th->requestInvalidation(getSys());
		else
			th->updateSizes();
	}
	//else do nothing as the height is determined by autoSize
	return NULL;
}

ASFUNCTIONBODY(TextField,_getTextWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->textWidth);
}

ASFUNCTIONBODY(TextField,_getTextHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->textHeight);
}

ASFUNCTIONBODY(TextField,_getHtmlText)
{
	TextField* th=Class<TextField>::cast(obj);
	return Class<ASString>::getInstanceS(th->toHtmlText());
}

ASFUNCTIONBODY(TextField,_setHtmlText)
{
	TextField* th=Class<TextField>::cast(obj);
	tiny_string value;
	ARG_UNPACK(value);
	th->setHtmlText(value);
	return NULL;
}

ASFUNCTIONBODY(TextField,_getText)
{
	TextField* th=Class<TextField>::cast(obj);
	return Class<ASString>::getInstanceS(th->text);
}

ASFUNCTIONBODY(TextField,_setText)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->updateText(args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(TextField, appendText)
{
	TextField* th=Class<TextField>::cast(obj);
	assert_and_throw(argslen==1);
	th->updateText(th->text + args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(TextField,_getTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	TextFormat *format=Class<TextFormat>::getInstanceS();

	format->color=_MNR(abstract_ui(th->textColor.toUInt()));
	format->font = th->font;
	format->size = th->fontSize;

	LOG(LOG_NOT_IMPLEMENTED, "getTextFormat is not fully implemeted");

	return format;
}

ASFUNCTIONBODY(TextField,_setTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	_NR<TextFormat> tf;
	int beginIndex;
	int endIndex;

	ARG_UNPACK(tf)(beginIndex, -1)(endIndex, -1);

	if(beginIndex!=-1 || endIndex!=-1)
		LOG(LOG_NOT_IMPLEMENTED,"setTextFormat with beginIndex or endIndex");

	if(tf->color)
		th->textColor = tf->color->toUInt();
	if (tf->font != "")
		th->font = tf->font;
	th->fontSize = tf->size;

	LOG(LOG_NOT_IMPLEMENTED,"setTextFormat does not read all fields of TextFormat");
	return NULL;
}

ASFUNCTIONBODY(TextField,_getDefaultTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	
	TextFormat* tf = Class<TextFormat>::getInstanceS();
	tf->font = th->font;
	LOG(LOG_NOT_IMPLEMENTED,"getDefaultTextFormat does not get all fields of TextFormat");
	return tf;
}

ASFUNCTIONBODY(TextField,_setDefaultTextFormat)
{
	TextField* th=Class<TextField>::cast(obj);
	_NR<TextFormat> tf;

	ARG_UNPACK(tf);

	if(tf->color)
		th->textColor = tf->color->toUInt();
	if (tf->font != "")
		th->font = tf->font;
	th->fontSize = tf->size;
	LOG(LOG_NOT_IMPLEMENTED,"setDefaultTextFormat does not set all fields of TextFormat");
	return NULL;
}

ASFUNCTIONBODY(TextField, _getter_type)
{
	TextField* th=Class<TextField>::cast(obj);
	if (th->type == READ_ONLY)
		return Class<ASString>::getInstanceS("dynamic");
	else
		return Class<ASString>::getInstanceS("input");
}

ASFUNCTIONBODY(TextField, _setter_type)
{
	TextField* th=Class<TextField>::cast(obj);

	tiny_string value;
	ARG_UNPACK(value);

	if (value == "dynamic")
		th->type = READ_ONLY;
	else if (value == "input")
		th->type = EDITABLE;
	else
		throwError<ArgumentError>(kInvalidEnumError, "type");

	return NULL;
}

ASFUNCTIONBODY(TextField,_getLineMetrics)
{
	LOG(LOG_NOT_IMPLEMENTED, "TextField.getLineMetrics() returns bogus values");
	return Class<TextLineMetrics>::getInstanceS(19, 280, 14, 11, 3.5, 0);
}

void TextField::updateSizes()
{
	uint32_t w,h,tw,th;
	w = width;
	h = height;
	//Compute (text)width, (text)height
	CairoPangoRenderer::getBounds(*this, w, h, tw, th);
	width = w; //TODO: check the case when w,h == 0
	textWidth=tw;
	height = h;
	textHeight=th;
}

tiny_string TextField::toHtmlText()
{
	xmlpp::DomParser parser;
	xmlpp::Document *doc = parser.get_document();
	xmlpp::Element *root = doc->create_root_node("font");

	ostringstream ss;
	ss << fontSize;
	root->set_attribute("size", ss.str());
	root->set_attribute("color", textColor.toString());
	root->set_attribute("face", font);

	//Split text into paragraphs and wraps them into <p> tags
	uint32_t para_start = 0;
	uint32_t para_end;
	do
	{
		para_end = text.find("\n", para_start);
		if (para_end == text.npos)
			para_end = text.numChars();

		xmlpp::Element *pNode = root->add_child("p");
		pNode->add_child_text(text.substr(para_start, para_end));
		para_start = para_end + 1;
	} while (para_end < text.numChars());

	xmlBufferPtr buf = xmlBufferCreateSize(4096);
	xmlNodeDump(buf, doc->cobj(), doc->get_root_node()->cobj(), 0, 0);
	tiny_string ret = tiny_string((char*)buf->content,true);
	xmlBufferFree(buf);
	return ret;
}

void TextField::setHtmlText(const tiny_string& html)
{
	HtmlTextParser parser;
	parser.parseTextAndFormating(html, this);
}

void TextField::updateText(const tiny_string& new_text)
{
	text = new_text;
	if(onStage)
		requestInvalidation(getSys());
	else
		updateSizes();
}

void TextField::requestInvalidation(InvalidateQueue* q)
{
	incRef();
	updateSizes();
	q->addToInvalidateQueue(_MR(this));
}

IDrawable* TextField::invalidate(DisplayObject* target, const MATRIX& initialMatrix)
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return NULL;
	}

	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;
	computeMasksAndMatrix(target, masks, totalMatrix);
	totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
	if(width==0 || height==0)
		return NULL;
	if(totalMatrix.getScaleX() != 1 || totalMatrix.getScaleY() != 1)
		LOG(LOG_NOT_IMPLEMENTED, "TextField when scaled is not correctly implemented");
	/**  TODO: The scaling is done differently for textfields : height changes are applied directly
		on the font size. In some cases, it can change the width (if autosize is on and wordwrap off).
		Width changes do not change the font size, and do nothing when autosize is on and wordwrap off.
		Currently, the TextField is stretched in case of scaling.
	*/
	return new CairoPangoRenderer(*this,
				totalMatrix, x, y, width, height, 1.0f,
				getConcatenatedAlpha(), masks);
}

void TextField::renderImpl(RenderContext& ctxt) const
{
	defaultRender(ctxt);
}

void TextField::HtmlTextParser::parseTextAndFormating(const tiny_string& html,
						      TextData *dest)
{
	textdata = dest;
	if (!textdata)
		return;

	textdata->text = "";

	tiny_string rooted = tiny_string("<root>") + html + tiny_string("</root>");
	try
	{
		parse_memory_raw((const unsigned char*)rooted.raw_buf(), rooted.numBytes());
	}
	catch (xmlpp::exception& exc)
	{
		LOG(LOG_ERROR, "TextField HTML parser error");
		return;
	}
}

void TextField::HtmlTextParser::on_start_element(const Glib::ustring& name,
						 const xmlpp::SaxParser::AttributeList& attributes)
{
	if (!textdata)
		return;

	if (name == "root")
	{
		return;
	}
	else if (name == "br")
	{
		if (textdata->multiline)
			textdata->text += "\n";
			
	}
	else if (name == "p")
	{
		if (textdata->multiline)
		{
			if (!textdata->text.empty() && 
			    !textdata->text.endsWith("\n"))
				textdata->text += "\n";
		}
	}
	else if (name == "font")
	{
		if (!textdata->text.empty())
		{
			LOG(LOG_NOT_IMPLEMENTED, "Font can be defined only in the beginning");
			return;
		}

		for (auto it=attributes.begin(); it!=attributes.end(); ++it)
		{
			if (it->name == "face")
			{
				textdata->font = it->value;
			}
			else if (it->name == "size")
			{
				textdata->fontSize = parseFontSize(it->value, textdata->fontSize);
			}
			else if (it->name == "color")
			{
				textdata->textColor = RGB(tiny_string(it->value));
			}
		}
	}
	else if (name == "a" || name == "img" || name == "u" ||
		 name == "li" || name == "b" || name == "i" ||
		 name == "span" || name == "textformat" || name == "tab")
	{
		LOG(LOG_NOT_IMPLEMENTED, _("Unsupported tag in TextField: ") + name);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED, _("Unknown tag in TextField: ") + name);
	}
}

void TextField::HtmlTextParser::on_end_element(const Glib::ustring& name)
{
	if (!textdata)
		return;

	if (name == "p")
	{
		if (textdata->multiline)
		{
			if (!textdata->text.empty() && 
			    !textdata->text.endsWith("\n"))
				textdata->text += "\n";
		}
	}
}

void TextField::HtmlTextParser::on_characters(const Glib::ustring& characters)
{
	if (!textdata)
		return;

	textdata->text += characters;
}

uint32_t TextField::HtmlTextParser::parseFontSize(const Glib::ustring& sizestr,
						  uint32_t currentFontSize)
{
	const char *s = sizestr.c_str();
	if (!s)
		return currentFontSize;

	uint32_t basesize = 0;
	int multiplier = 1;
	if (s[0] == '+' || s[0] == '-')
	{
		// relative size
		basesize = currentFontSize;
		if (s[0] == '-')
			multiplier = -1;
	}

	int64_t size = basesize + multiplier*g_ascii_strtoll(s, NULL, 10);
	if (size < 1)
		size = 1;
	if (size > G_MAXUINT32)
		size = G_MAXUINT32;
	
	return (uint32_t)size;
}

void TextFieldAutoSize ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"),DECLARED_TRAIT);
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"),DECLARED_TRAIT);
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"),DECLARED_TRAIT);
}

void TextFieldType::sinit(Class_base* c)
{
	c->setVariableByQName("DYNAMIC","",Class<ASString>::getInstanceS("dynamic"),DECLARED_TRAIT);
	c->setVariableByQName("INPUT","",Class<ASString>::getInstanceS("input"),DECLARED_TRAIT);
}

void TextFormatAlign ::sinit(Class_base* c)
{
	c->setVariableByQName("CENTER","",Class<ASString>::getInstanceS("center"),DECLARED_TRAIT);
	c->setVariableByQName("END","",Class<ASString>::getInstanceS("end"),DECLARED_TRAIT);
	c->setVariableByQName("JUSTIFY","",Class<ASString>::getInstanceS("justify"),DECLARED_TRAIT);
	c->setVariableByQName("LEFT","",Class<ASString>::getInstanceS("left"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<ASString>::getInstanceS("right"),DECLARED_TRAIT);
	c->setVariableByQName("START","",Class<ASString>::getInstanceS("start"),DECLARED_TRAIT);
}

void TextFormat::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c,align);
	REGISTER_GETTER_SETTER(c,blockIndent);
	REGISTER_GETTER_SETTER(c,bold);
	REGISTER_GETTER_SETTER(c,bullet);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,font);
	REGISTER_GETTER_SETTER(c,indent);
	REGISTER_GETTER_SETTER(c,italic);
	REGISTER_GETTER_SETTER(c,kerning);
	REGISTER_GETTER_SETTER(c,leading);
	REGISTER_GETTER_SETTER(c,leftMargin);
	REGISTER_GETTER_SETTER(c,letterSpacing);
	REGISTER_GETTER_SETTER(c,rightMargin);
	REGISTER_GETTER_SETTER(c,size);
	REGISTER_GETTER_SETTER(c,tabStops);
	REGISTER_GETTER_SETTER(c,target);
	REGISTER_GETTER_SETTER(c,underline);
	REGISTER_GETTER_SETTER(c,url);
}

void TextFormat::finalize()
{
	ASObject::finalize();
	blockIndent.reset();
	bold.reset();
	bullet.reset();
	color.reset();
	indent.reset();
	italic.reset();
	kerning.reset();
	leading.reset();
	leftMargin.reset();
	letterSpacing.reset();
	rightMargin.reset();
	tabStops.reset();
	underline.reset();
}

ASFUNCTIONBODY(TextFormat,_constructor)
{
	TextFormat* th=static_cast<TextFormat*>(obj);
	ARG_UNPACK (th->font, "")
		(th->size, 12)
		(th->color,_MNR(getSys()->getNullRef()))
		(th->bold,_MNR(getSys()->getNullRef()))
		(th->italic,_MNR(getSys()->getNullRef()))
		(th->underline,_MNR(getSys()->getNullRef()))
		(th->url,"")
		(th->target,"")
		(th->align,"left")
		(th->leftMargin,_MNR(getSys()->getNullRef()))
		(th->rightMargin,_MNR(getSys()->getNullRef()))
		(th->indent,_MNR(getSys()->getNullRef()))
		(th->leading,_MNR(getSys()->getNullRef()));
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER_CB(TextFormat,align,onAlign);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,blockIndent);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bold);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,bullet);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,color);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,font);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,indent);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,italic);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,kerning);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leading);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,leftMargin);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,letterSpacing);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,rightMargin);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,size);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,tabStops);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,target);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,underline);
ASFUNCTIONBODY_GETTER_SETTER(TextFormat,url);

void TextFormat::buildTraits(ASObject* o)
{
}

void TextFormat::onAlign(const tiny_string& old)
{
	if (align != "center" && align != "end" && align != "justify" && 
	    align != "left" && align != "right" && align != "start")
	{
		align = old;
		throwError<ArgumentError>(kInvalidEnumError, "align");
	}
}

void StyleSheet::finalize()
{
	EventDispatcher::finalize();
	styles.clear();
}

void StyleSheet::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("styleNames","",Class<IFunction>::getFunction(_getStyleNames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("setStyle","",Class<IFunction>::getFunction(setStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getStyle","",Class<IFunction>::getFunction(getStyle),NORMAL_METHOD,true);
}

void StyleSheet::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(StyleSheet,setStyle)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==2);
	const tiny_string& arg0=args[0]->toString();
	args[1]->incRef(); //TODO: should make a copy, see reference
	map<tiny_string, _R<ASObject>>::iterator it=th->styles.find(arg0);
	//NOTE: we cannot use the [] operator as References cannot be non initialized
	if(it!=th->styles.end()) //Style already exists
		it->second=_MR(args[1]);
	else
		th->styles.insert(make_pair(arg0,_MR(args[1])));
	return NULL;
}

ASFUNCTIONBODY(StyleSheet,getStyle)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	map<tiny_string, _R<ASObject>>::iterator it=th->styles.find(arg0);
	if(it!=th->styles.end()) //Style already exists
	{
		//TODO: should make a copy, see reference
		it->second->incRef();
		return it->second.getPtr();
	}
	else
	{
		// Tested behaviour is to return an empty ASObject
		// instead of Null as is said in the documentation
		return Class<ASObject>::getInstanceS();
	}
	return NULL;
}

ASFUNCTIONBODY(StyleSheet,_getStyleNames)
{
	StyleSheet* th=Class<StyleSheet>::cast(obj);
	assert_and_throw(argslen==0);
	Array* ret=Class<Array>::getInstanceS();
	map<tiny_string, _R<ASObject>>::const_iterator it=th->styles.begin();
	for(;it!=th->styles.end();++it)
		ret->push(_MR(Class<ASString>::getInstanceS(it->first)));
	return ret;
}

void StaticText::sinit(Class_base* c)
{
	//TODO: spec says that constructor should throw ArgumentError
	c->setConstructor(NULL);
	c->setSuper(Class<DisplayObject>::getRef());
	c->setDeclaredMethodByQName("text","",Class<IFunction>::getFunction(_getText),GETTER_METHOD,true);
}

ASFUNCTIONBODY(StaticText,_getText)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.StaticText.text is not implemented");
	return Class<ASString>::getInstanceS("");
}

void FontStyle::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("BOLD","",Class<ASString>::getInstanceS("bold"),DECLARED_TRAIT);
	c->setVariableByQName("BOLD_ITALIC","",Class<ASString>::getInstanceS("boldItalic"),DECLARED_TRAIT);
	c->setVariableByQName("ITALIC","",Class<ASString>::getInstanceS("italic"),DECLARED_TRAIT);
	c->setVariableByQName("REGULAR","",Class<ASString>::getInstanceS("regular"),DECLARED_TRAIT);
}

void FontType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("DEVICE","",Class<ASString>::getInstanceS("device"),DECLARED_TRAIT);
	c->setVariableByQName("EMBEDDED","",Class<ASString>::getInstanceS("embedded"),DECLARED_TRAIT);
	c->setVariableByQName("EMBEDDED_CFF","",Class<ASString>::getInstanceS("embeddedCFF"),DECLARED_TRAIT);
}

void TextDisplayMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("CRT","",Class<ASString>::getInstanceS("crt"),DECLARED_TRAIT);
	c->setVariableByQName("DEFAULT","",Class<ASString>::getInstanceS("default"),DECLARED_TRAIT);
	c->setVariableByQName("LCD","",Class<ASString>::getInstanceS("lcd"),DECLARED_TRAIT);
}

void TextColorType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("DARK_COLOR","",Class<ASString>::getInstanceS("dark"),DECLARED_TRAIT);
	c->setVariableByQName("LIGHT_COLOR","",Class<ASString>::getInstanceS("light"),DECLARED_TRAIT);
}

void GridFitType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
	c->setVariableByQName("PIXEL","",Class<ASString>::getInstanceS("pixel"),DECLARED_TRAIT);
	c->setVariableByQName("SUBPIXEL","",Class<ASString>::getInstanceS("subpixel"),DECLARED_TRAIT);
}

void TextLineMetrics::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c, ascent);
	REGISTER_GETTER_SETTER(c, descent);
	REGISTER_GETTER_SETTER(c, height);
	REGISTER_GETTER_SETTER(c, leading);
	REGISTER_GETTER_SETTER(c, width);
	REGISTER_GETTER_SETTER(c, x);
}

ASFUNCTIONBODY(TextLineMetrics, _constructor)
{
	if (argslen == 0)
	{
		//Assume that the values were initialized by the C++
		//constructor
		return NULL;
	}

	TextLineMetrics* th=static_cast<TextLineMetrics*>(obj);
	ARG_UNPACK (th->x) (th->width) (th->height) (th->ascent) (th->descent) (th->leading);
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, ascent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, descent);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, height);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, leading);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, width);
ASFUNCTIONBODY_GETTER_SETTER(TextLineMetrics, x);
