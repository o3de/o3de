/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_BOOLCOMBOBOX_CTRL
#define PROPERTY_BOOLCOMBOBOX_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <QToolButton>
#include "PropertyEditorAPI.h"
#include <UI/PropertyEditor/GenericComboBoxCtrl.h>
#endif

class QComboBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    class PropertyBoolComboBoxCtrl
        : public GenericComboBoxCtrl<bool>
    {
        Q_OBJECT
        using ComboBoxBase = GenericComboBoxCtrl<bool>;
    public:

        AZ_RTTI(PropertyBoolComboBoxCtrl, "{44255BDF-38E1-43E1-B920-2F5118B66996}", ComboBoxBase);
        AZ_CLASS_ALLOCATOR(PropertyBoolComboBoxCtrl, AZ::SystemAllocator);
        PropertyBoolComboBoxCtrl(QWidget* pParent);

        virtual ~PropertyBoolComboBoxCtrl();

    public: 
        void UpdateTabOrder() override;
    };

    class BoolPropertyComboBoxHandler
        : QObject
        , public PropertyHandler<bool, PropertyBoolComboBoxCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BoolPropertyComboBoxHandler, AZ::SystemAllocator);

        virtual AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::ComboBox; }
        virtual void UpdateWidgetInternalTabbing(PropertyBoolComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyBoolComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyBoolComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyBoolComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterBoolComboBoxHandler();
};

#endif
