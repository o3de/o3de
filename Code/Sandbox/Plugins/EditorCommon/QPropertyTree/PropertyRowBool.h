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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWBOOL_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWBOOL_H
#pragma once

#include "PropertyRow.h"
#include "Unicode.h"

class PropertyRowBool
    : public PropertyRow
{
public:
    PropertyRowBool();
    bool assignToPrimitive(void* val, size_t size) const override;
    bool assignToByPointer(void* instance, const Serialization::TypeID& type) const;
    void setValue(bool value, const void* handle, [[maybe_unused]] const Serialization::TypeID& typeId) { value_ = value; serializer_.setPointer((void*)handle); serializer_.setType(Serialization::TypeID::get<bool>()); }

    void redraw(const PropertyDrawContext& context);
    bool isLeaf() const{ return true; }
    bool isStatic() const{ return false; }

    bool onActivate(const PropertyActivationEvent& e);
    DragCheckBegin onMouseDragCheckBegin() override;
    bool onMouseDragCheck(QPropertyTree* tree, bool value) override;
    wstring valueAsWString() const{ return value_ ? L"true" : L"false"; }
    string valueAsString() const{ return value_ ? "true" : "false"; }
    WidgetPlacement widgetPlacement() const{ return WIDGET_ICON; }
    void serializeValue(Serialization::IArchive& ar);
    int widgetSizeMin(const QPropertyTree* tree) const override;
    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;
protected:
    bool value_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWBOOL_H
