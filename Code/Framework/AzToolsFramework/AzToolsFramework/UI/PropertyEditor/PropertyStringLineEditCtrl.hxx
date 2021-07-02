/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_STRINGLINEEDIT_CTRL
#define PROPERTY_STRINGLINEEDIT_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include "PropertyEditorAPI.h"
#endif

class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    class PropertyStringLineEditCtrl
        : public QWidget
    {
        friend class StringPropertyLineEditHandler;
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyStringLineEditCtrl, AZ::SystemAllocator, 0);

        PropertyStringLineEditCtrl(QWidget* pParent = NULL);
        virtual ~PropertyStringLineEditCtrl();

        AZStd::string value() const;
        QLineEdit* GetLineEdit() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(AZStd::string& newValue);

    public slots:
        virtual void setValue(AZStd::string& val);
        void setMaxLen(int maxLen);

    protected slots:
        void onChildLineEditValueChange(const QString& value);

    protected:
        virtual void ConnectWidgets();
        virtual void focusInEvent(QFocusEvent* e);

        QLineEdit* m_pLineEdit;
    };

    class StringPropertyLineEditHandler
        : QObject
        , public PropertyHandler<AZStd::string, PropertyStringLineEditCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(StringPropertyLineEditHandler, AZ::SystemAllocator, 0);

        virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("LineEdit", 0x3f15f4ba); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyStringLineEditCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyStringLineEditCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyStringLineEditCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyStringLineEditCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyStringLineEditCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyStringLineEditCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterStringLineEditHandler();
};

#endif
