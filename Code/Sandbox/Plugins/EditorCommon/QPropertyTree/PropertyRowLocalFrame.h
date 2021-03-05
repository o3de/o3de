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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWLOCALFRAME_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWLOCALFRAME_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Serialization/Decorators/IGizmoSink.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#endif

struct IGizmoSink;

class PropertyRowLocalFrameBase
    : public PropertyRow
{
public:
    PropertyRowLocalFrameBase();
    ~PropertyRowLocalFrameBase();

    bool isLeaf() const override { return m_reset; }
    bool isStatic() const override { return false; }

    bool onActivate(const PropertyActivationEvent& e) override;

    WidgetPlacement widgetPlacement() const override { return WIDGET_AFTER_PULLED; }
    int widgetSizeMin(const QPropertyTree* tree) const override { return tree->_defaultRowHeight(); }

    string valueAsString() const override;
    bool onContextMenu(QMenu& menu, QPropertyTree* tree) override;
    const void* searchHandle() const override { return m_handle; }
    void redraw(const PropertyDrawContext& context) override;

    void reset(QPropertyTree* tree);
protected:
    Serialization::IGizmoSink* m_sink;
    const void* m_handle;
    int m_gizmoIndex;
    mutable Serialization::GizmoFlags m_gizmoFlags;
    bool m_reset;
};

struct LocalFrameMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    PropertyRowLocalFrameBase* self;

    LocalFrameMenuHandler(QPropertyTree* tree, PropertyRowLocalFrameBase* self)
        : tree(tree)
        , self(self) {}
public slots:
    void onMenuReset();
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWLOCALFRAME_H
