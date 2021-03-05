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

#include "EditorCommon_precompiled.h"
#include "Serialization/ClassFactory.h"
#include "PropertyTreeModel.h"
#include "PropertyRowNumber.h"
#include "MathUtils.h"

#include <QKeyEvent>
#include <QStyleOption>
#include <QStyleOptionSlider>
#include <QPainter>
#include "QPropertyTree.h"
#include "PropertyRow.h"
#include "PropertyDrawContext.h"

#include "Serialization.h"
#include "Serialization/Decorators/Slider.h"
#include "Serialization/Decorators/SliderImpl.h"
#include <math.h>

using Serialization::SSliderF;
using Serialization::SSliderI;

class PropertyRowSliderF
    : public PropertyRowNumberField
{
public:
    static const bool Custom = true;
    PropertyRowSliderF();
    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    int floorHeight() const override { return 18; }
    void redraw(const PropertyDrawContext& context) override;

    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed) override;
    void onMouseDrag(const PropertyDragEvent& e) override;
    void onMouseUp(QPropertyTree* tree, QPoint point) override;
    bool assignTo(const Serialization::SStruct& ser) const override
    {
        if (ser.size() != sizeof(SSliderF))
        {
            return false;
        }
        SSliderF* slider = (SSliderF*)(ser.pointer());
        *slider->valuePointer = clamp(localValue_, slider->minLimit, slider->maxLimit);
        return true;
    }
    bool handleMouse(QPropertyTree* tree, QPoint point);

    bool setValueFromString(const char* str) override
    {
        float newValue = aznumeric_cast<float>(atof(str));
        if (localValue_ != newValue)
        {
            localValue_ = newValue;
            return true;
        }
        else
        {
            return false;
        }
    }

    string valueAsString() const override
    {
        return numberAsString(localValue_);
    }

    void startIncrement() override
    {
        incrementStartValue_ = localValue_;
    }

    void endIncrement(QPropertyTree* tree) override
    {
        if (localValue_ != incrementStartValue_)
        {
            float localValue = localValue_;
            localValue_ = incrementStartValue_;
            tree->model()->rowAboutToBeChanged(this);
            localValue_ = localValue;
            tree->model()->rowChanged(this);
        }
    }

    void incrementLog(float screenFraction, [[maybe_unused]] float valueFieldFraction)
    {
        double startPower = log10(fabs(double(incrementStartValue_) + 1.0)) - 3.0;
        double power = startPower + fabs(screenFraction) * 10.0f;
        double delta = powf(10.0f, aznumeric_cast<float>(power)) - powf(10.0f, aznumeric_cast<float>(startPower)) + 10.0f * fabsf(screenFraction);
        double newValue;
        if (screenFraction > 0.0f)
        {
            newValue = double(incrementStartValue_) + delta;
        }
        else
        {
            newValue = double(incrementStartValue_) - delta;
        }
        if (_isnan(newValue))
        {
            if (screenFraction > 0.0f)
            {
                newValue = DBL_MAX;
            }
            else
            {
                newValue = -DBL_MAX;
            }
        }
        clampToType(&localValue_, newValue);
    }

    void serializeValue(Serialization::IArchive& ar)
    {
        ar(value_.minLimit, "min");
        ar(value_.maxLimit, "max");
        ar(localValue_, "value");
    }

    double sliderPosition() const override { return 0.0; }

    SSliderF value_;
    float localValue_;
    float incrementStartValue_;
    bool captured_;
};

bool PropertyRowSliderF::handleMouse(QPropertyTree* tree, QPoint point)
{
    QStyleOptionSlider slider;
    slider.rect = floorRect(tree);
    QSlider widgetForContext;
    QRect sliderGroove = tree->style()->subControlRect(QStyle::CC_Slider, &slider, QStyle::SC_SliderGroove, &widgetForContext);
    QRect sliderHandle = tree->style()->subControlRect(QStyle::CC_Slider, &slider, QStyle::SC_SliderHandle, &widgetForContext);

    int sliderLength = sliderGroove.width() - sliderHandle.width();

    float valRelative = float(point.x() - (sliderGroove.left() + sliderHandle.width() / 2)) / sliderLength;
    if (valRelative < 0.0f)
    {
        valRelative = 0.0f;
    }
    if (valRelative > 1.0f)
    {
        valRelative = 1.0f;
    }
    float newValue = float(valRelative * (value_.maxLimit - value_.minLimit) + value_.minLimit);
    if (newValue != localValue_)
    {
        localValue_ = newValue;
        setMultiValue(false);
        return true;
    }
    else
    {
        return false;
    }
}

bool PropertyRowSliderF::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    float step = (value_.maxLimit - value_.minLimit) * 0.01f;
    if (ev->key() == Qt::Key_Left)
    {
        tree->model()->rowAboutToBeChanged(this);
        localValue_ = clamp(localValue_ - step, value_.minLimit, value_.maxLimit);
        tree->model()->rowChanged(this);
        return true;
    }
    if (ev->key() == Qt::Key_Right)
    {
        tree->model()->rowAboutToBeChanged(this);
        localValue_ = clamp(localValue_ + step, value_.minLimit, value_.maxLimit);
        tree->model()->rowChanged(this);
        return true;
    }
    return PropertyRowNumberField::onKeyDown(tree, ev);
}

bool PropertyRowSliderF::onMouseDown(QPropertyTree* tree, QPoint point, [[maybe_unused]] bool& changed)
{
    if (floorRect(tree).contains(point) && !userReadOnly())
    {
        tree->model()->rowAboutToBeChanged(this);
        if (handleMouse(tree, point))
        {
            tree->update();
        }
        captured_ = true;
        return true;
    }
    captured_ = false;
    return true;
}

void PropertyRowSliderF::onMouseDrag(const PropertyDragEvent& e)
{
    if (!captured_)
    {
        return;
    }
    if (userReadOnly())
    {
        return;
    }
    if (handleMouse(e.tree, e.pos))
    {
        e.tree->update();
    }
}

void PropertyRowSliderF::onMouseUp(QPropertyTree* tree, QPoint point)
{
    if (!captured_)
    {
        return;
    }
    handleMouse(tree, point);
    tree->model()->rowChanged(this);
}

void PropertyRowSliderF::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    value_ = *(SSliderF*)ser.pointer();
    localValue_ = *value_.valuePointer;
    value_.valuePointer = 0;
}

static void drawSlider(const PropertyDrawContext& context, float relativeVal, bool userReadOnly, bool selected)
{
    // Eliminate x-shift offset in the control rect as it triggers a bug in
    // Fusion theme, that causes blue area in the slider groove to stand out to
    // the right from slider.
    int xOffset = context.lineRect.left();
    context.painter->translate(xOffset, 0);

    QStyleOptionSlider sliderOptions;
    sliderOptions.rect = context.lineRect.translated(-xOffset, 0);
    sliderOptions.minimum = 0;
    QSlider widgetForContext;
    QRect sliderGroove = context.tree->style()->subControlRect(QStyle::CC_Slider, &sliderOptions, QStyle::SC_SliderGroove, &widgetForContext);
    QRect sliderHandle = context.tree->style()->subControlRect(QStyle::CC_Slider, &sliderOptions, QStyle::SC_SliderHandle, &widgetForContext);
    int width = sliderGroove.width() - sliderHandle.width() + 1;
    sliderOptions.maximum = width;
    sliderOptions.pageStep = width / 100;
    sliderOptions.sliderPosition = aznumeric_cast<int>(width * relativeVal);
    sliderOptions.state = !userReadOnly ? (QStyle::State_Enabled | (selected ? QStyle::State_HasFocus : QStyle::State())) : QStyle::State();

    context.tree->style()->drawComplexControl(QStyle::CC_Slider, &sliderOptions, context.painter,  &widgetForContext);

    context.painter->translate(-xOffset, 0);
}

void PropertyRowSliderF::redraw(const PropertyDrawContext& context)
{
    PropertyRowNumberField::redraw(context);
    float val = localValue_;
    float valRange = value_.maxLimit - value_.minLimit;
    if (valRange == 0.0f)
    {
        valRange = 0.00001f;
    }
    float relativeVal = clamp((val - value_.minLimit) / valRange, 0.0f, 1.0f);

    drawSlider(context, relativeVal, userReadOnly(), selected());
}

PropertyRowSliderF::PropertyRowSliderF()
    : captured_(false)
    , localValue_()
    , incrementStartValue_()
{
}

DECLARE_SEGMENT(PropertyRowSliderF)
REGISTER_PROPERTY_ROW(SSliderF, PropertyRowSliderF)

// ---------------------------------------------------------------------------

class PropertyRowSliderI
    : public PropertyRowNumberField
{
public:
    static const bool Custom = true;
    PropertyRowSliderI();
    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    int floorHeight() const override { return 18; }
    void redraw(const PropertyDrawContext& context) override;

    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed) override;
    void onMouseDrag(const PropertyDragEvent& e) override;
    void onMouseUp(QPropertyTree* tree, QPoint point) override;
    bool assignTo(const Serialization::SStruct& ser) const override
    {
        if (ser.size() != sizeof(SSliderI))
        {
            return false;
        }
        SSliderI* slider = (SSliderI*)(ser.pointer());
        *slider->valuePointer = clamp(localValue_, slider->minLimit, slider->maxLimit);
        return true;
    }
    bool handleMouse(QPropertyTree* tree, QPoint point);

    bool setValueFromString(const char* str) override
    {
        int newValue = atoi(str);
        if (localValue_ != newValue)
        {
            localValue_ = newValue;
            return true;
        }
        else
        {
            return false;
        }
    }
    Serialization::string valueAsString() const override
    {
        return numberAsString(localValue_);
    }

    void startIncrement() override
    {
        incrementStartValue_ = localValue_;
    }

    void endIncrement(QPropertyTree* tree) override
    {
        if (localValue_ != incrementStartValue_)
        {
            int localValue = localValue_;
            localValue_ = incrementStartValue_;
            tree->model()->rowAboutToBeChanged(this);
            localValue_ = localValue;
            tree->model()->rowChanged(this);
        }
    }

    void incrementLog(float screenFraction, [[maybe_unused]] float valueFieldFraction)
    {
        double startPower = log10(fabs(double(incrementStartValue_) + 1.0)) - 3.0;
        double power = startPower + fabs(screenFraction) * 10.0f;
        double delta = powf(10.0f, aznumeric_cast<float>(power)) - powf(10.0f, aznumeric_cast<float>(startPower)) + 1000.0f * fabsf(screenFraction);
        double newValue;
        if (screenFraction > 0.0f)
        {
            newValue = double(incrementStartValue_) + delta;
        }
        else
        {
            newValue = double(incrementStartValue_) - delta;
        }
        if (_isnan(newValue))
        {
            if (screenFraction > 0.0f)
            {
                newValue = DBL_MAX;
            }
            else
            {
                newValue = -DBL_MAX;
            }
        }
        clampToType(&localValue_, newValue);
    }

    void serializeValue(Serialization::IArchive& ar)
    {
        ar(value_.minLimit, "min");
        ar(value_.maxLimit, "max");
        ar(localValue_, "value");
    }

    double sliderPosition() const override { return 0.0; }

    SSliderI value_;
    int localValue_;
    int incrementStartValue_;
    bool captured_;
};

bool PropertyRowSliderI::handleMouse(QPropertyTree* tree, QPoint point)
{
    QStyleOptionSlider slider;
    slider.rect = floorRect(tree);

    QSlider widgetForContext;
    QRect sliderGroove = tree->style()->subControlRect(QStyle::CC_Slider, &slider, QStyle::SC_SliderGroove, &widgetForContext);
    QRect sliderHandle = tree->style()->subControlRect(QStyle::CC_Slider, &slider, QStyle::SC_SliderHandle, &widgetForContext);

    int sliderLength = sliderGroove.width() - sliderHandle.width();

    float valRelative = float(point.x() - (sliderGroove.left() + sliderHandle.width() / 2)) / sliderLength;
    if (valRelative < 0.0f)
    {
        valRelative = 0.0f;
    }
    if (valRelative > 1.0f)
    {
        valRelative = 1.0f;
    }
    int newValue = int(valRelative * (value_.maxLimit - value_.minLimit) + value_.minLimit);
    if (newValue != localValue_)
    {
        localValue_ = newValue;
        setMultiValue(false);
        return true;
    }
    else
    {
        return false;
    }
}

bool PropertyRowSliderI::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    int step = aznumeric_cast<int>((value_.maxLimit - value_.minLimit) * 0.01f);
    if (ev->key() == Qt::Key_Left)
    {
        tree->model()->rowAboutToBeChanged(this);
        localValue_ = clamp(localValue_ - step, value_.minLimit, value_.maxLimit);
        tree->model()->rowChanged(this);
        return true;
    }
    if (ev->key() == Qt::Key_Right)
    {
        tree->model()->rowAboutToBeChanged(this);
        localValue_ = clamp(localValue_ + step, value_.minLimit, value_.maxLimit);
        tree->model()->rowChanged(this);
        return true;
    }
    return PropertyRowNumberField::onKeyDown(tree, ev);
}

bool PropertyRowSliderI::onMouseDown(QPropertyTree* tree, QPoint point, [[maybe_unused]] bool& changed)
{
    if (floorRect(tree).contains(point) && !userReadOnly())
    {
        tree->model()->rowAboutToBeChanged(this);
        if (handleMouse(tree, point))
        {
            tree->update();
        }
        captured_ = true;
        return true;
    }
    captured_ = false;
    return true;
}

void PropertyRowSliderI::onMouseDrag(const PropertyDragEvent& e)
{
    if (!captured_)
    {
        return;
    }
    if (userReadOnly())
    {
        return;
    }
    if (handleMouse(e.tree, e.pos))
    {
        e.tree->update();
    }
}

void PropertyRowSliderI::onMouseUp(QPropertyTree* tree, QPoint point)
{
    if (!captured_)
    {
        return;
    }
    handleMouse(tree, point);
    tree->model()->rowChanged(this);
}

void PropertyRowSliderI::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    value_ = *(SSliderI*)ser.pointer();
    localValue_ = *value_.valuePointer;
    value_.valuePointer = 0;
}

void PropertyRowSliderI::redraw(const PropertyDrawContext& context)
{
    PropertyRowNumberField::redraw(context);
    int val = localValue_;
    int valRange = value_.maxLimit - value_.minLimit;
    if (valRange == 0)
    {
        valRange = 1;
    }
    float relativeVal = clamp(float(val - value_.minLimit) / valRange, 0.0f, 1.0f);

    drawSlider(context, relativeVal, userReadOnly(), selected());
}

PropertyRowSliderI::PropertyRowSliderI()
    : captured_(false)
    , localValue_()
    , incrementStartValue_()
{
}

DECLARE_SEGMENT(PropertyRowSliderI)
REGISTER_PROPERTY_ROW(SSliderI, PropertyRowSliderI)
