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
#include "platform.h"

#include <QIcon>

#include "Serialization/ClassFactory.h"
#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "Color.h"
#include "Unicode.h"
#include "Serialization/Decorators/ActionButton.h"

using Serialization::IActionButton;
using Serialization::IActionButtonPtr;

class PropertyRowActionButton
    : public PropertyRow
{
public:
    PropertyRowActionButton()
        : underMouse_()
        , pressed_()
        , minimalWidth_() {}
    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }
    bool isSelectable() const override { return true; }

    bool onActivate(const PropertyActivationEvent& e) override
    {
        if (e.reason == PropertyActivationEvent::REASON_KEYBOARD)
        {
            if (value_)
            {
                value_->Callback();
            }
        }
        return true;
    }

    bool onMouseDown(QPropertyTree* tree, QPoint point, [[maybe_unused]] bool& changed) override
    {
        if (userReadOnly())
        {
            return false;
        }
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
        if (userReadOnly())
        {
            return;
        }
        bool underMouse = widgetRect(e.tree).contains(e.pos);
        if (underMouse != underMouse_)
        {
            underMouse_ = underMouse;
            e.tree->update();
        }
    }

    void onMouseUp(QPropertyTree* tree, QPoint point) override
    {
        if (userReadOnly())
        {
            return;
        }
        if (widgetRect(tree).contains(point))
        {
            pressed_ = false;
            if (value_)
            {
                value_->Callback();
            }
            tree->update();
        }
    }
    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        value_ = static_cast<IActionButton*>(ser.pointer())->Clone();
        const char* icon = value_->Icon();
        icon_ = icon && icon[0] ? QIcon() : QIcon(QString::fromLocal8Bit(icon));
    }
    bool assignTo([[maybe_unused]] const Serialization::SStruct& ser) const override { return true; }
    wstring valueAsWString() const override { return L""; }
    WidgetPlacement widgetPlacement() const override { return WIDGET_INSTEAD_OF_TEXT; }
    void serializeValue([[maybe_unused]] Serialization::IArchive& ar) override { }

    int widgetSizeMin(const QPropertyTree* tree) const override
    {
        if (minimalWidth_ == 0)
        {
            QFontMetrics fm(tree->font());
            minimalWidth_ = (int)fm.horizontalAdvance(QString::fromLocal8Bit(labelUndecorated())) + 6 + (icon_.isNull() ? 0 : 18);
        }
        return minimalWidth_;
    }

    void redraw(const PropertyDrawContext& context)
    {
        QRect rect = context.widgetRect.adjusted(-1, -1, 1, 1);
        bool pressed = pressed_ && underMouse_;

        wstring text = toWideChar(labelUndecorated());
        if (icon_.isNull())
        {
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
        else
        {
            context.drawButtonWithIcon(icon_, rect, text.c_str(), selected(), pressed, selected(), !userReadOnly(), true, &context.tree->font());
        }
    }
    bool isFullRow(const QPropertyTree* tree) const override
    {
        if (PropertyRow::isFullRow(tree))
        {
            return true;
        }
        return !userFixedWidget();
    }
protected:
    mutable int minimalWidth_;
    bool underMouse_;
    bool pressed_;
    QIcon icon_;
    IActionButtonPtr value_;
};

REGISTER_PROPERTY_ROW(IActionButton, PropertyRowActionButton);
