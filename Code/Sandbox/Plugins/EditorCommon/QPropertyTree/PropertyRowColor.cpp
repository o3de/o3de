/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "PropertyRowColor.h"
#include "Serialization/ClassFactory.h"
#include <Cry_Color.h>
#include <QMenu>
#include <QFileDialog>
#include <QPainter>

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

using Serialization::Vec3AsColor;
typedef SerializableColor_tpl<unsigned char> SerializableColorB;
typedef SerializableColor_tpl<float> SerializableColorF;

QColor ToQColor(const ColorB& v)
{
    return QColor(v.r, v.g, v.b, v.a);
}

void FromQColor(SerializableColorB& vColor, QColor color)
{
    vColor.r = color.red();
    vColor.g = color.green();
    vColor.b = color.blue();
    vColor.a = color.alpha();
}


QColor ToQColor(const Vec3AsColor& v)
{
    return QColor(int(v.v.x * 255.0f), int(v.v.y * 255.0f), int(v.v.z * 255.0f));
}

void FromQColor(Vec3AsColor& vColor, QColor color)
{
    vColor.v.x = color.red() / 255.0f;
    vColor.v.y = color.green() / 255.0f;
    vColor.v.z = color.blue() / 255.0f;
}

QColor ToQColor(const SerializableColorF& v)
{
    return QColor::fromRgbF(v.r, v.g, v.b, v.a);
}

void FromQColor(SerializableColorF& vColor, QColor color)
{
    vColor.r = aznumeric_cast<float>(color.redF());
    vColor.g = aznumeric_cast<float>(color.greenF());
    vColor.b = aznumeric_cast<float>(color.blueF());
    vColor.a = aznumeric_cast<float>(color.alphaF());
}


template <class ColorClass>
bool PropertyRowColor<ColorClass>::pickColor(QPropertyTree* tree)
{
    const AZ::Color initialColor = AzQtComponents::fromQColor(color_);
    const AZ::Color color = AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGB, initialColor, QObject::tr("Select Color"));

    if (color != initialColor)
    {
        tree->model()->rowAboutToBeChanged(this);
        color_.setRed(color.GetR8());
        color_.setGreen(color.GetG8());
        color_.setBlue(color.GetB8());
        colorChanged_ = true;
        tree->model()->rowChanged(this);
        return true;
    }

    return false;
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::onActivate(const PropertyActivationEvent& e)
{
    return pickColor(e.tree);
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] IArchive& ar)
{
    color_ = ToQColor(*(ColorClass*)ser.pointer());
    colorChanged_ = false;
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::assignTo(const Serialization::SStruct& ser) const
{
    FromQColor(*((ColorClass*)ser.pointer()), color_);
    return true;
}

template<class ColorClass>
string PropertyRowColor<ColorClass>::valueAsString() const
{
    char buf[64];
    sprintf_s(buf, "%d %d %d", (int)color_.red(), (int)color_.green(), (int)color_.blue());
    return string(buf);
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    Serialization::SharedPtr<PropertyRowColor> selfPointer(this);
    ColorMenuHandler* handler = new ColorMenuHandler(tree, this);
    menu.addAction("Pick Color", handler, SLOT(onMenuPickColor()));
    tree->addMenuHandler(handler);
    return true;
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::redraw(const PropertyDrawContext& context)
{
    static QImage checkboardPattern;
    if (checkboardPattern.isNull())
    {
        int size = 12;
        static vector<int> pixels(size * size);
        for (int i = 0; i < pixels.size(); ++i)
        {
            pixels[i] = ((i / size) / (size / 2) + (i % size) / (size / 2)) % 2 ? 0xffffffff : 0x000000ff;
        }
        checkboardPattern = QImage((unsigned char*)pixels.data(), size, size, size * 4, QImage::Format_RGBA8888);
    }

    QRect r = context.widgetRect.adjusted(0, 0, 0, -1);

    context.painter->save();
    context.painter->setPen(QPen(Qt::NoPen));
    context.painter->setRenderHint(QPainter::Antialiasing, true);
    context.painter->setBrush(context.tree->palette().color(QPalette::Dark));
    context.painter->setPen(Qt::NoPen);
    context.painter->drawRoundedRect(r, 2, 2);
    r = r.adjusted(1, 1, -1, -1);
    QRect cr = r.adjusted(0, 0, -r.width() / 2, 0);
    context.painter->setBrushOrigin(cr.topRight() + QPoint(1, 0));
    context.painter->setBrush(QBrush(checkboardPattern));

    context.painter->setRenderHint(QPainter::Antialiasing, false);
    context.painter->drawRoundedRect(r, 2, 2);

    context.painter->setPen(QPen(Qt::NoPen));
    context.painter->setClipRect(cr);
    context.painter->setBrush(QBrush(color_));
    context.painter->drawRoundedRect(r, 2, 2);

    cr = r.adjusted(r.width() / 2, 0, 0, 0);
    context.painter->setClipRect(cr);
    context.painter->setBrush(QBrush(QColor(color_.red(), color_.green(), color_.blue(), 255)));
    context.painter->drawRoundedRect(r, 2, 2);
    context.painter->restore();
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::closeNonLeaf(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    color_ = ToQColor(*(ColorClass*)ser.pointer());
}

static int componentFromRowValue(const char* str, ColorB*)
{
    return clamp_tpl(atoi(str), 0, 255);
}

static int componentFromRowValue(const char* str, ColorF*)
{
    return clamp_tpl(int(atof(str) * 255.0f + 0.5f), 0, 255);
}

static int componentFromRowValue(const char* str, Vec3AsColor*)
{
    return clamp_tpl(int(atof(str) * 255.0f + 0.5f), 0, 255);
}

template<class ColorClass>
void PropertyRowColor<ColorClass>::handleChildrenChange()
{
    // generally is not needed unless we are using callbacks
    PropertyRow* rows[4] = {
        childByIndex(0),
        childByIndex(1),
        childByIndex(2),
        childByIndex(3)
    };

    if (rows[0])
    {
        color_.setRed(componentFromRowValue(rows[0]->valueAsString().c_str(), (ColorClass*)0));
    }
    if (rows[1])
    {
        color_.setGreen(componentFromRowValue(rows[1]->valueAsString().c_str(), (ColorClass*)0));
    }
    if (rows[2])
    {
        color_.setBlue(componentFromRowValue(rows[2]->valueAsString().c_str(), (ColorClass*)0));
    }
    if (rows[3])
    {
        color_.setAlpha(componentFromRowValue(rows[3]->valueAsString().c_str(), (ColorClass*)0));
    }
}


ColorMenuHandler::ColorMenuHandler(QPropertyTree* tree, IPropertyRowColor* propertyRowColor)
    : propertyRowColor(propertyRowColor)
    , tree(tree)
{
}

void ColorMenuHandler::onMenuPickColor()
{
    propertyRowColor->pickColor(tree);
}

typedef PropertyRowColor<SerializableColorB> PropertyRowColorB;
typedef PropertyRowColor<Vec3AsColor> PropertyRowVec3AsColor;
typedef PropertyRowColor<SerializableColorF> PropertyRowColorF;

REGISTER_PROPERTY_ROW(SerializableColorB, PropertyRowColorB);
REGISTER_PROPERTY_ROW(Vec3AsColor, PropertyRowVec3AsColor);
REGISTER_PROPERTY_ROW(SerializableColorF, PropertyRowColorF);

#include <QPropertyTree/moc_PropertyRowColor.cpp>
