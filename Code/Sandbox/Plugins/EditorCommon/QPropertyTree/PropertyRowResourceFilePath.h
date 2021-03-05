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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCEFILEPATH_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCEFILEPATH_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyDrawContext.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/ResourceFilePath.h>
#include <Serialization/Decorators/ResourceFilePathImpl.h>
#include <Serialization/Decorators/IconXPM.h>
#endif

using Serialization::ResourceFilePath;

class PropertyRowResourceFilePath
    : public PropertyRowField
{
public:
    void clear();

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }

    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    bool assignTo(const Serialization::SStruct& ser) const override;
    bool onActivate(const PropertyActivationEvent& e) override;

    int buttonCount() const override { return 1; }
    virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
    virtual bool usePathEllipsis() const override { return true; }
    string valueAsString() const;
    void serializeValue(Serialization::IArchive& ar);
    const void* searchHandle() const override { return handle_; }
    Serialization::TypeID typeId() const override { return Serialization::TypeID::get<string>(); }

    bool onContextMenu(QMenu& menu, QPropertyTree* tree);

    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;

private:
    string filter_;
    string path_;
    string startFolder_;
    bool m_group;
    int flags_;
    const void* handle_;
};

struct ResourceFilePathMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    PropertyRowResourceFilePath* self;

    ResourceFilePathMenuHandler(QPropertyTree* tree, PropertyRowResourceFilePath* container);
public slots:
    void onMenuClear();
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWRESOURCEFILEPATH_H
