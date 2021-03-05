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
#include <QIcon>

#include "Serialization/ClassFactory.h"
#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "Color.h"
#include "Unicode.h"
#include "Serialization/Decorators/ToggleButton.h"
using Serialization::ToggleButton;
using Serialization::RadioButton;

class PropertyRowToggleButton
    : public PropertyRow
{
public:
    PropertyRowToggleButton()
        : underMouse_(false)
        , value_(false)
    {
    }

    bool isLeaf() const{ return true; }
    bool isStatic() const{ return false; }
    bool isSelectable() const{ return true; }

    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        ToggleButton* value = (ToggleButton*)(ser.pointer());
        value_ = *value->value;
    }

    bool assignTo(const Serialization::SStruct& ser) const override
    {
        ToggleButton* value = (ToggleButton*)(ser.pointer());
        *value->value = value_;
        return true;
    }
    wstring valueAsWString() const override { return L""; }
    WidgetPlacement widgetPlacement() const override { return WIDGET_INSTEAD_OF_TEXT; }
    void serializeValue([[maybe_unused]] Serialization::IArchive& ar) {}
    int widgetSizeMin([[maybe_unused]] const QPropertyTree* tree) const override { return 36; }

    bool onActivate(const PropertyActivationEvent& e) override
    {
        if (e.reason == PropertyActivationEvent::REASON_KEYBOARD)
        {
            value_;
        }
        return true;
    }

    bool onMouseDown(QPropertyTree* tree, QPoint point, [[maybe_unused]] bool& changed) override
    {
        if (widgetRect(tree).contains(point))
        {
            underMouse_ = true;
            pressed_ = true;
            tree->update();
            return true;
        }
        return false;
    }

    void onMouseDrag(const PropertyDragEvent& e) override
    {
        bool underMouse = widgetRect(e.tree).contains(e.pos);
        if (underMouse != underMouse_)
        {
            underMouse_ = underMouse;
            e.tree->update();
        }
    }

    void onMouseUp(QPropertyTree* tree, QPoint point) override
    {
        if (widgetRect(tree).contains(point))
        {
            tree->model()->rowAboutToBeChanged(this);
            pressed_ = false;
            value_ = !value_;
            tree->model()->rowChanged(this);
        }
    }

    void redraw(const PropertyDrawContext& context) override
    {
        QRect rect = context.widgetRect;

        wstring text = toWideChar(labelUndecorated());
        int buttonFlags = BUTTON_CENTER;
        if ((value_ || pressed_) && underMouse_)
        {
            buttonFlags |= BUTTON_PRESSED;
        }
        if (selected() || pressed_)
        {
            buttonFlags |= BUTTON_FOCUSED;
        }
        if (userReadOnly())
        {
            buttonFlags |= BUTTON_DISABLED;
        }
        context.drawButton(rect, text.c_str(), buttonFlags, &context.tree->font());
    }
protected:
    bool pressed_ : 1;
    bool underMouse_ : 1;
    bool value_ : 1;
};

class PropertyRowRadioButton
    : public PropertyRow
{
public:

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }
    bool isSelectable() const override { return false; }

    bool onActivate(const PropertyActivationEvent& e) override
    {
        if (!m_justSet)
        {
            e.tree->model()->rowAboutToBeChanged(this);
            m_justSet = true;
            e.tree->model()->rowChanged(this);
        }
        return true;
    }
    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        RadioButton* value = (RadioButton*)(ser.pointer());
        m_value = value->buttonValue;
        m_toggled = m_value == *value->value;
        m_justSet = false;
    }
    bool assignTo(const Serialization::SStruct& ser) const override
    {
        if (m_justSet)
        {
            *((RadioButton*)ser.pointer())->value = m_value;
        }
        return true;
    }
    wstring valueAsWString() const override { return L""; }
    WidgetPlacement widgetPlacement() const override { return WIDGET_INSTEAD_OF_TEXT; }
    void serializeValue(Serialization::IArchive& ar) override
    {
        bool oldToggled = m_toggled;
        ar(m_toggled, "toggled");
        if (m_toggled && !oldToggled)
        {
            m_justSet = true;
        }
        ar(m_value, "value");
    }
    int widgetSizeMin([[maybe_unused]] const QPropertyTree* tree) const override { return 40; }

    void redraw(const PropertyDrawContext& context)
    {
        QRect rect = context.widgetRect;
        bool pressed = context.m_pressed || m_toggled || m_justSet;

        wstring text = toWideChar(labelUndecorated());
        int buttonFlags = BUTTON_CENTER;
        if (pressed)
        {
            buttonFlags |= BUTTON_PRESSED;
        }
        if (selected())
        {
            buttonFlags |= BUTTON_FOCUSED;
        }
        if (userReadOnly())
        {
            buttonFlags |= BUTTON_DISABLED;
        }
        context.drawButton(rect, text.c_str(), buttonFlags, &context.tree->font());
    }
protected:
    bool m_toggled;
    bool m_justSet;
    int m_value;
};

REGISTER_PROPERTY_ROW(ToggleButton, PropertyRowToggleButton);
REGISTER_PROPERTY_ROW(RadioButton, PropertyRowRadioButton);
