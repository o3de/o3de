/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "Serialization/ClassFactory.h"

#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "Color.h"
#include "Serialization/Decorators/IconXPM.h"
using Serialization::IconXPM;
using Serialization::IconXPMToggle;

class PropertyRowIconXPM : public PropertyRow{
public:
    void redraw(const PropertyDrawContext& context)
    {
        QRect rect = context.widgetRect;
        context.drawIcon(rect, icon_);
    }

    bool isLeaf() const{ return true; }
    bool isStatic() const{ return false; }
    bool isSelectable() const{ return false; }

    bool onActivate([[maybe_unused]] const PropertyActivationEvent& e)
    {
        return false;
    }
    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override {
        YASLI_ESCAPE(ser.size() == sizeof(IconXPM), return);
        icon_ = *(IconXPM*)(ser.pointer());
    }
    wstring valueAsWString() const{ return L""; }
    WidgetPlacement widgetPlacement() const{ return WIDGET_ICON; }
    void serializeValue([[maybe_unused]] Serialization::IArchive& ar) {}
    int widgetSizeMin(const QPropertyTree* tree) const override{ return tree->_defaultRowHeight(); }
    int height() const{ return 16; }
protected:
    IconXPM icon_;
};

class PropertyRowIconToggle : public PropertyRow{
public:
    void redraw(const PropertyDrawContext& context) override
    {
        IconXPM& icon = value_ ? iconTrue_ : iconFalse_;
        context.drawIcon(context.widgetRect, icon);
    }

    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override {
        YASLI_ESCAPE(ser.size() == sizeof(IconXPMToggle), return);
        const IconXPMToggle* icon = (IconXPMToggle*)(ser.pointer());
        iconTrue_ = icon->iconTrue_;
        iconFalse_ = icon->iconFalse_;
        value_ = icon->value_;
    }

    bool assignTo(const Serialization::SStruct& ser) const override
    {
        IconXPMToggle* toggle = (IconXPMToggle*)ser.pointer();
        toggle->value_ = value_;
        return true;
    }

    bool isLeaf() const override{ return true; }
    bool isStatic() const override{ return false; }
    bool isSelectable() const override{ return true; }
    bool onActivate(const PropertyActivationEvent& e)
    {
        if (e.reason != e.REASON_RELEASE)
        {
            e.tree->model()->rowAboutToBeChanged(this);
            value_ = !value_;
            e.tree->model()->rowChanged(this);
            return true;
        }
        return false;
    }
    DragCheckBegin onMouseDragCheckBegin() override
    {
        if (userReadOnly())
            return DRAG_CHECK_IGNORE;
        return value_ ? DRAG_CHECK_UNSET : DRAG_CHECK_SET;
    }
    bool onMouseDragCheck(QPropertyTree* tree, bool value) override
    {
        if (value_ != value) {
            tree->model()->rowAboutToBeChanged(this);
            value_ = value;
            tree->model()->rowChanged(this);
            return true;
        }
        return false;
    }
    wstring valueAsWString() const{ return value_ ? L"true" : L"false"; }
    WidgetPlacement widgetPlacement() const{ return WIDGET_ICON; }

    int widgetSizeMin(const QPropertyTree* tree) const{ return tree->_defaultRowHeight(); }
    int height() const{ return 16; }

    IconXPM iconTrue_;
    IconXPM iconFalse_;
    bool value_;
};

REGISTER_PROPERTY_ROW(IconXPM, PropertyRowIconXPM); 
REGISTER_PROPERTY_ROW(IconXPMToggle, PropertyRowIconToggle); 
DECLARE_SEGMENT(PropertyRowIconXPM)
