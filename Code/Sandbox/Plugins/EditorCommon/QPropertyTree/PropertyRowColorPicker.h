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
#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCOLORPICKER_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCOLORPICKER_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyDrawContext.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/ColorPicker.h>
#include <Serialization/Decorators/ColorPickerImpl.h>
#include <Serialization/Decorators/IconXPM.h>
#endif

using Serialization::ColorPicker;

class PropertyRowColorPicker
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
    string valueAsString() const override;
    void serializeValue(Serialization::IArchive& ar);

    bool onContextMenu(QMenu& menu, QPropertyTree* tree);

    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;

private:
    ColorF color_;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWCOLORPICKER_H
