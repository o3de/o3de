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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCESELECTOR_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCESELECTOR_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyRowField.h"
#include <Serialization.h>
#include "Serialization/Decorators/Resources.h"
#include "IResourceSelectorHost.h"
#include <QIcon>
#endif

using Serialization::IResourceSelector;
namespace Serialization {
    struct INavigationProvider;
}

class PropertyRowResourceSelector
    : public PropertyRowField
{
public:
    PropertyRowResourceSelector()
        : provider_(0)
        , id_(0)
        , searchHandle_(0) {}
    void clear();

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }

    void jumpTo(QPropertyTree* tree);
    bool createFile(QPropertyTree* tree);
    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    bool assignTo(const Serialization::SStruct& ser) const override;
    bool onActivate(const PropertyActivationEvent& ev) override;
    bool onActivateButton(int button, const PropertyActivationEvent& e) override;
    bool getHoverInfo(PropertyHoverInfo* hover, const QPoint& cursorPos, const QPropertyTree* tree) const override;
    const void* searchHandle() const override { return searchHandle_; }
    Serialization::TypeID typeId() const override { return wrappedType_; }

    int buttonCount() const override;
    virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
    virtual bool usePathEllipsis() const override { return true; }
    string valueAsString() const;
    void serializeValue(Serialization::IArchive& ar);
    void redraw(const PropertyDrawContext& context);

    bool onContextMenu(QMenu& menu, QPropertyTree* tree);
    bool pickResource(QPropertyTree* tree);
    const char* typeNameForFilter([[maybe_unused]] QPropertyTree* tree) const override { return !type_.empty() ? type_.c_str() : "ResourceSelector"; }

    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;

private:
    SResourceSelectorContext context_;
    Serialization::INavigationProvider* provider_;
    const void* searchHandle_;
    Serialization::TypeID wrappedType_;
    QIcon icon_;

    string type_;
    string value_;
    string defaultPath_;
    int id_;
};

struct ResourceSelectorMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    PropertyRowResourceSelector* self;

    ResourceSelectorMenuHandler(QPropertyTree* tree, PropertyRowResourceSelector* container);
public slots:
    void onMenuCreateFile();
    void onMenuJumpTo();
    void onMenuClear();
    void onMenuPickResource();
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCESELECTOR_H
