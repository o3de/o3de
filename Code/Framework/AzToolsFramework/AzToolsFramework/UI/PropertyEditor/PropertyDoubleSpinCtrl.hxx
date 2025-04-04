/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_DOUBLESPINBOX_CTRL
#define PROPERTY_DOUBLESPINBOX_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include "PropertyEditorAPI.h"
#endif

namespace AzQtComponents
{
    class DoubleSpinBox;
}

namespace AzToolsFramework
{
    class PropertyDoubleSpinCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyDoubleSpinCtrl, AZ::SystemAllocator);

        PropertyDoubleSpinCtrl(QWidget* pParent = NULL);
        virtual ~PropertyDoubleSpinCtrl();

        double value() const;
        double minimum() const;
        double maximum() const;
        double step() const;
        double multiplier() const;

        int GetDefaultDecimals() const;
        int GetDefaultDisplayDecimals() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(double newValue);
        void editingFinished();

    public slots:
        void setValue(double val);
        void setMinimum(double val);
        void setMaximum(double val);
        void setStep(double val);
        void setMultiplier(double val);
        void setPrefix(QString val);
        void setSuffix(QString val);
        void setDecimals(int precision);
        void setDisplayDecimals(int displayDecimals);

    protected slots:
        void onChildSpinboxValueChange(double value);

    private:
        AzQtComponents::DoubleSpinBox* m_pSpinBox;
        double m_multiplier;
        int m_defaultDecimals;
        int m_defaultDisplayDecimals;

    protected:
        void focusInEvent(QFocusEvent* e) override;
        void focusOutEvent(QFocusEvent* e) override;
    };

    template <class ValueType>
    class DoubleSpinBoxHandlerCommon
        : public PropertyHandler<ValueType, PropertyDoubleSpinCtrl>
    {
    public:
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::SpinBox; }
        bool IsDefaultHandler() const override { return true; }
        QWidget* GetFirstInTabOrder(PropertyDoubleSpinCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(PropertyDoubleSpinCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(PropertyDoubleSpinCtrl* widget) override { widget->UpdateTabOrder(); }
    };

    class doublePropertySpinboxHandler
        : QObject
        , public DoubleSpinBoxHandlerCommon<double>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(doublePropertySpinboxHandler, AZ::SystemAllocator);

        // common to all double spinners
        static void ConsumeAttributeCommon(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);
        static void ResetGUIToDefaultsCommon(PropertyDoubleSpinCtrl* GUI);
        static bool GetDefaultDecimals(PropertyDoubleSpinCtrl* GUI);
        static bool GetDefaultDisplayDecimals(PropertyDoubleSpinCtrl* GUI);

        QWidget* CreateGUI(QWidget* pParent) override;
        bool ResetGUIToDefaults(PropertyDoubleSpinCtrl* GUI) override;
        void ConsumeAttribute(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyDoubleSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class floatPropertySpinboxHandler
        : QObject
        , public DoubleSpinBoxHandlerCommon<float>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(floatPropertySpinboxHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* pParent) override;
        bool ResetGUIToDefaults(PropertyDoubleSpinCtrl* GUI) override;
        void ConsumeAttribute(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyDoubleSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };


    void RegisterDoubleSpinBoxHandlers();
};

#endif
