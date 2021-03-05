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

#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include <Serialization.h>
#endif

struct IPropertyRowColor
{
    virtual bool pickColor(QPropertyTree* tree) = 0;
};

template <class ColorClass>
class PropertyRowColor
    : public PropertyRow
    , public IPropertyRowColor
{
public:
    PropertyRowColor()
        : colorChanged_(false) {}

    bool isLeaf() const override { return colorChanged_; }
    bool isStatic() const override { return false; }
    WidgetPlacement widgetPlacement() const{ return WIDGET_AFTER_PULLED; }
    int widgetSizeMin(const QPropertyTree* tree) const { return userWidgetSize() >= 0 ? userWidgetSize() : tree->_defaultRowHeight()* 2 - 4; }
    void handleChildrenChange() override;

    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    bool assignTo(const Serialization::SStruct& ser) const override;
    void closeNonLeaf(const Serialization::SStruct& ser, Serialization::IArchive& ar);

    bool onActivate(const PropertyActivationEvent& ev) override;

    string valueAsString() const;
    void redraw(const PropertyDrawContext& context);

    bool onContextMenu(QMenu& menu, QPropertyTree* tree);

    bool pickColor(QPropertyTree* tree) override;

private:
    QColor color_;
    bool colorChanged_;
};

struct ColorMenuHandler
    : PropertyRowMenuHandler
{
    Q_OBJECT
public:
    QPropertyTree * tree;
    IPropertyRowColor* propertyRowColor;

    ColorMenuHandler(QPropertyTree* tree, IPropertyRowColor* propertyRowColor);
    ~ColorMenuHandler(){};

public slots:
    void onMenuPickColor();
};
