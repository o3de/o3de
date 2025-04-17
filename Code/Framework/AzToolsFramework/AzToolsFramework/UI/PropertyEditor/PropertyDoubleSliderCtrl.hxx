/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_DOUBLESLIDER_CTRL
#define PROPERTY_DOUBLESLIDER_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include "PropertyEditorAPI.h"

#endif

namespace AzToolsFramework
{
    class PropertyDoubleSliderCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyDoubleSliderCtrl, AZ::SystemAllocator);

        explicit PropertyDoubleSliderCtrl(QWidget* parent = nullptr);

    public slots:
        void setValue(double val);
        void setMinimum(double val);
        void setMaximum(double val);
        void setRange(double min, double max);
        void setStep(double val);
        void setMultiplier(double val);
        void setSoftMinimum(double val);
        void setSoftMaximum(double val);
        void setPrefix(QString val);
        void setSuffix(QString val);
        void setDecimals(int decimals);
        void setDisplayDecimals(int displayDecimals);
        void setCurveMidpoint(double midpoint);

        double minimum() const;
        double maximum() const;
        double step() const;
        double value() const;
        double softMinimum() const;
        double softMaximum() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        void ClearSavedState();

        void onValueChange();
    signals:
        void valueChanged(double val);
        void editingFinished();

    private:
        AzQtComponents::SliderDoubleCombo* m_sliderCombo = nullptr;
        double m_multiplier = 1;
        Q_DISABLE_COPY(PropertyDoubleSliderCtrl)
    };

    template <class ValueType>
    class DoubleSliderHandlerCommon
        : public PropertyHandler<ValueType, PropertyDoubleSliderCtrl>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::Slider; }
        QWidget* GetFirstInTabOrder(PropertyDoubleSliderCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(PropertyDoubleSliderCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(PropertyDoubleSliderCtrl* widget) override { widget->UpdateTabOrder(); }

        void BeforeConsumeAttributes(PropertyDoubleSliderCtrl* widget, InstanceDataNode* /*attrValue*/) override
        {
            widget->ClearSavedState();
        }
    };

    class doublePropertySliderHandler
        : QObject
        , public DoubleSliderHandlerCommon<double>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(doublePropertySliderHandler, AZ::SystemAllocator);

        // common to all double sliders
        static void ConsumeAttributeCommon(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSliderCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyDoubleSliderCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class floatPropertySliderHandler
        : QObject
        , public DoubleSliderHandlerCommon<float>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(floatPropertySliderHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSliderCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyDoubleSliderCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };


    void RegisterDoubleSliderHandlers();
}

#endif
