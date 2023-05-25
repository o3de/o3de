/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_CLASS_ALLOCATOR(PropertyStringLineEditCtrl, AZ::SystemAllocator);

        PropertyStringLineEditCtrl(QWidget* pParent = NULL);
        virtual ~PropertyStringLineEditCtrl();

        //! This helper method is used to replicate a user input of editing the value
        //! will call setValue on the QLineEdit but will also emit the editingFinished signal
        void UpdateValue(const QString& newValue);

        AZStd::string value() const;
        QLineEdit* GetLineEdit() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    public slots:
        virtual void setValue(AZStd::string& val);
        void setMaxLen(int maxLen);

    protected:
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
        AZ_CLASS_ALLOCATOR(StringPropertyLineEditHandler, AZ::SystemAllocator);

        virtual AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::LineEdit; }
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
