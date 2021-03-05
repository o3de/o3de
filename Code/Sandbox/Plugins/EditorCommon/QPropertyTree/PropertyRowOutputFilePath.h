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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOUTPUTFILEPATH_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOUTPUTFILEPATH_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyDrawContext.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/OutputFilePath.h>
#include <Serialization/Decorators/OutputFilePathImpl.h>
#endif

using Serialization::OutputFilePath;

class PropertyRowOutputFilePath
    : public PropertyRowField
{
public:
    void clear();

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }

    bool onActivate(const PropertyActivationEvent& e) override;
    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    bool assignTo(const Serialization::SStruct& ser) const override;
    string valueAsString() const;
    void serializeValue(Serialization::IArchive& ar);
    bool onContextMenu(QMenu& menu, QPropertyTree* tree);
    const void* searchHandle() const { return handle_; }

    int buttonCount() const override { return 1; }
    virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
    virtual bool usePathEllipsis() const override { return true; }

    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;

private:
    string path_;
    string filter_;
    string startFolder_;
    const void* handle_;
};

struct OutputFilePathMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    PropertyRowOutputFilePath* self;

    OutputFilePathMenuHandler(QPropertyTree* tree, PropertyRowOutputFilePath* container);
public slots:
    void onMenuClear();
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOUTPUTFILEPATH_H
