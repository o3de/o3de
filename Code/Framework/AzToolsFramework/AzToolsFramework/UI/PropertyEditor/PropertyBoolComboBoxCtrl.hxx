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
#include "PropertyEditorAPI.h"
#endif

class QComboBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    //just a test to see how it would work to pop a dialog

    class PropertyBoolComboBoxCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyBoolComboBoxCtrl, AZ::SystemAllocator, 0);

        PropertyBoolComboBoxCtrl(QWidget* pParent = NULL);
        virtual ~PropertyBoolComboBoxCtrl();

        bool value() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(bool newValue);

    public slots:
        void setValue(bool val);

    protected slots:
        void onChildComboBoxValueChange(int value);

    private:
        QComboBox* m_pComboBox;
    };

    class BoolPropertyComboBoxHandler
        : QObject
        , public PropertyHandler<bool, PropertyBoolComboBoxCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BoolPropertyComboBoxHandler, AZ::SystemAllocator, 0);

        virtual AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::ComboBox; }
        virtual QWidget* GetFirstInTabOrder(PropertyBoolComboBoxCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyBoolComboBoxCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyBoolComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyBoolComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyBoolComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyBoolComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterBoolComboBoxHandler();
};

#endif
