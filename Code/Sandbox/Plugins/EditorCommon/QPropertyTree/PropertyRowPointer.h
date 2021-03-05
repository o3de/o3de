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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWPOINTER_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWPOINTER_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Color.h"
#include "Serialization/StringList.h"
using Serialization::StringList;

#include "PropertyRow.h"
#endif

class QPropertyTree;
class PropertyRowPointer;
struct CreatePointerMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    PropertyRowPointer* row;
    int index;
    bool useDefaultValue;
public slots:
    void onMenuCreateByIndex();
};

class QMenu;
struct ClassMenuItemAdder
{
    virtual void addAction(QMenu& menu, const char* text, int index);
    virtual QMenu* addMenu(QMenu& menu, const char* text);
    void generateMenu(QMenu& createItem, const StringList& comboStrings);
};

class PropertyRowPointer
    : public PropertyRow
{
public:
    PropertyRowPointer();

    bool assignTo(Serialization::IPointer& ptr);
    void setValueAndContext(const Serialization::IPointer& ptr, Serialization::IArchive& ar);
    using PropertyRow::assignTo;
    using PropertyRow::setValueAndContext;
    using PropertyRow::onActivate;

    Serialization::TypeID baseType() const{ return baseType_; }
    void setBaseType(const Serialization::TypeID& baseType) { baseType_ = baseType; }
    const char* derivedTypeName() const{ return derivedTypeName_.c_str(); }
    void setDerivedType(const char* typeName, Serialization::IClassFactory* factory);
    void setFactory(Serialization::IClassFactory* factory) { factory_ = factory; }
    Serialization::IClassFactory* factory() const{ return factory_; }
    bool onActivate(QPropertyTree* tree, bool force);
    bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed);
    bool onContextMenu(QMenu& root, QPropertyTree* tree);
    bool isStatic() const{ return false; }
    bool isPointer() const{ return true; }
    int widgetSizeMin(const QPropertyTree* tree) const override;
    wstring generateLabel() const;
    string valueAsString() const;
    const char* typeNameForFilter([[maybe_unused]] QPropertyTree* tree) const override { return baseType_.name(); }
    void redraw(const PropertyDrawContext& context);
    WidgetPlacement widgetPlacement() const{ return WIDGET_VALUE; }
    void serializeValue(Serialization::IArchive& ar);
    const void* searchHandle() const override { return searchHandle_; }
    Serialization::TypeID typeId() const override { return pointerType_; }
protected:

    Serialization::TypeID baseType_;
    string derivedTypeName_;
    string derivedLabel_;

    // this member is available for instances deserialized from clipboard:
    Serialization::IClassFactory* factory_;
    const void* searchHandle_;
    Serialization::TypeID  pointerType_;
    Color colorOverride_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWPOINTER_H
