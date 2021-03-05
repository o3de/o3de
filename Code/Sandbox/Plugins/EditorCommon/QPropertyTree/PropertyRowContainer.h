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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCONTAINER_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCONTAINER_H
#pragma once
#if !defined(Q_MOC_RUN)
#include "PropertyRow.h"
#endif


class EDITOR_COMMON_API PropertyRowContainer;
struct EDITOR_COMMON_API ContainerMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:

    QPropertyTree * tree;
    PropertyRowContainer* container;
    PropertyRow* element;
    int pointerIndex;

    ContainerMenuHandler(QPropertyTree* tree, PropertyRowContainer* container);

public slots:
    virtual void onMenuAddElement();
    virtual void onMenuAppendElement();
    virtual void onMenuAppendPointerByIndex();
    virtual void onMenuRemoveAll();
    virtual void onMenuChildInsertBefore();
    virtual void onMenuChildRemove();
};

class EDITOR_COMMON_API PropertyRowContainer
    : public PropertyRow
{
public:
    PropertyRowContainer();
    bool isContainer() const{ return true; }
    bool onActivate(const PropertyActivationEvent& e);
    bool onContextMenu(QMenu& item, QPropertyTree* tree);
    virtual ContainerMenuHandler* createMenuHandler(QPropertyTree* tree, PropertyRowContainer* container) override;
    void redraw(const PropertyDrawContext& context);
    bool processesKeyContainer(QPropertyTree* tree, const QKeyEvent* ev);
    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDownContainer(QPropertyTree* tree, const QKeyEvent* key);
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* key) override;

    void labelChanged() override;
    bool isStatic() const{ return false; }
    bool isSelectable() const{ return userWidgetSize() == 0 ? false : true; }
    PropertyRow* addElement(QPropertyTree* tree, bool append);
    void setInlined(bool inlined) { inlined_ = inlined; }
    bool isInlined() const{ return inlined_; }

    PropertyRow* defaultRow(PropertyTreeModel* model);
    const PropertyRow* defaultRow(const PropertyTreeModel* model) const;
    void serializeValue(Serialization::IArchive& ar);

    const char* elementTypeName() const{ return elementTypeName_; }
    using PropertyRow::setValueAndContext;
    virtual void setValueAndContext(const Serialization::IContainer& value, [[maybe_unused]] Serialization::IArchive& ar)
    {
        fixedSize_ = value.isFixedSize();
        elementTypeName_ = value.elementType().name();
        serializer_.setPointer(value.pointer());
        serializer_.setType(value.containerType());
    }
    const char* typeNameForFilter(QPropertyTree* tree) const override;
    string valueAsString() const;
    // C-array is an example of fixed size container
    bool isFixedSize() const{ return fixedSize_; }
    WidgetPlacement widgetPlacement() const override { return inlined_ ? WIDGET_NONE : WIDGET_AFTER_NAME; }
    int widgetSizeMin(const QPropertyTree* tree) const override;

protected:
    virtual void generateMenu(QMenu& menu, QPropertyTree* tree, bool addActions);

    const char* elementTypeName_;
    wchar_t buttonLabel_[8];
    bool fixedSize_;
    bool inlined_;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCONTAINER_H
