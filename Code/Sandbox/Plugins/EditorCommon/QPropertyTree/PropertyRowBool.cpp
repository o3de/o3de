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
#include "PropertyRowBool.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "Serialization/ClassFactory.h"
#include "Serialization.h"
#include <QKeyEvent>

SERIALIZATION_CLASS_NAME(PropertyRow, PropertyRowBool, "PropertyRowBool", "bool");

PropertyRowBool::PropertyRowBool()
    : value_(false)
{
}

bool PropertyRowBool::assignToPrimitive(void* object, [[maybe_unused]] size_t size) const
{
    YASLI_ASSERT(size == sizeof(bool));
    *reinterpret_cast<bool*>(object) = value_;
    return true;
}

bool PropertyRowBool::assignToByPointer(void* instance, const Serialization::TypeID& type) const
{
    return assignToPrimitive(instance, type.sizeOf());
}

void PropertyRowBool::redraw(const PropertyDrawContext& context)
{
    context.drawCheck(widgetRect(context.tree), userReadOnly(), multiValue() ? CHECK_IN_BETWEEN : (value_ ? CHECK_SET : CHECK_NOT_SET));
}

bool PropertyRowBool::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (QKeySequence(ev->key()) == QKeySequence(Qt::Key_Space))
    {
        return true;
    }

    return PropertyRow::processesKey(tree, ev);
}

bool PropertyRowBool::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (QKeySequence(ev->key()) == QKeySequence(Qt::Key_Space))
    {
        PropertyActivationEvent e;
        e.tree = tree;
        e.reason = e.REASON_KEYBOARD;
        onActivate(e);
        return true;
    }

    return PropertyRow::onKeyDown(tree, ev);
}

bool PropertyRowBool::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason != e.REASON_RELEASE)
    {
        if (!this->userReadOnly())
        {
            e.tree->model()->rowAboutToBeChanged(this);
            value_ = !value_;
            e.tree->model()->rowChanged(this);
            return true;
        }
    }
    return false;
}

DragCheckBegin PropertyRowBool::onMouseDragCheckBegin()
{
    if (userReadOnly())
    {
        return DRAG_CHECK_IGNORE;
    }
    return value_ ? DRAG_CHECK_UNSET : DRAG_CHECK_SET;
}

bool PropertyRowBool::onMouseDragCheck(QPropertyTree* tree, bool value)
{
    if (value_ != value)
    {
        tree->model()->rowAboutToBeChanged(this);
        value_ = value;
        tree->model()->rowChanged(this);
        return true;
    }
    return false;
}

void PropertyRowBool::serializeValue(Serialization::IArchive& ar)
{
    ar(value_, "value", "Value");
}

int PropertyRowBool::widgetSizeMin(const QPropertyTree* tree) const
{
    return aznumeric_cast<int>(tree->_defaultRowHeight() * 0.9f);
}
