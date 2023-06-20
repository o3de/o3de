/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_INTSLIDER_CTRL
#define PROPERTY_INTSLIDER_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyIntCtrlCommon.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#endif

namespace AzToolsFramework
{
    class PropertyIntSliderCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyIntSliderCtrl, AZ::SystemAllocator);

        explicit PropertyIntSliderCtrl(QWidget* parent = nullptr);
        virtual ~PropertyIntSliderCtrl();

    public slots:
        void setValue(AZ::s64 val);
        void setMinimum(AZ::s64 val);
        void setMaximum(AZ::s64 val);
        void setStep(AZ::s64 val);
        void setMultiplier(AZ::s64 val);
        void setSoftMinimum(AZ::s64 val);
        void setSoftMaximum(AZ::s64 val);
        void setPrefix(QString val);
        void setSuffix(QString val);

        AZ::s64 minimum() const;
        AZ::s64 maximum() const;
        AZ::s64 step() const;
        AZ::s64 multiplier() const;
        AZ::s64 value() const;
        AZ::s64 softMinimum() const;
        AZ::s64 softMaximum() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        void onValueChange();
    signals:
        void valueChanged(AZ::s64 val);
        void editingFinished();

    private:
        AzQtComponents::SliderCombo* m_sliderCombo = nullptr;
        AZ::s64 m_multiplier = 1;

        Q_DISABLE_COPY(PropertyIntSliderCtrl)
    };

    // Base class to allow QObject inheritance and definitions for IntSpinBoxHandlerCommon class template
    class IntSliderHandlerQObject
        : public QObject
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    };

    template <class ValueType>
    class IntSliderHandler
        : public IntWidgetHandler<ValueType, PropertyIntSliderCtrl, IntSliderHandlerQObject>
    {
        using BaseHandler = IntWidgetHandler<ValueType, PropertyIntSliderCtrl, IntSliderHandlerQObject>;
    public:
        AZ_CLASS_ALLOCATOR(IntSliderHandler, AZ::SystemAllocator);
    protected:
        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Slider; }
        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(PropertyIntSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        QWidget* GetFirstInTabOrder(PropertyIntSliderCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(PropertyIntSliderCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(PropertyIntSliderCtrl* widget) override { widget->UpdateTabOrder(); }
    };

    template <class ValueType>
    QWidget* IntSliderHandler<ValueType>::CreateGUI(QWidget* parent)
    {
        PropertyIntSliderCtrl* newCtrl = static_cast<PropertyIntSliderCtrl*>(BaseHandler::CreateGUI(parent));
        this->connect(newCtrl, &PropertyIntSliderCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });

        return newCtrl;
    }

    template <class ValueType>
    void IntSliderHandler<ValueType>::ConsumeAttribute(PropertyIntSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        AZ::s64 value;
        if (attrib == AZ::Edit::Attributes::SoftMin)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                auto clampedValue = GetSafeAttributeValue<ValueType>(value, debugName, "SoftMin");
                GUI->setSoftMinimum(clampedValue);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'SoftMin' attribute from property '%s' into Slider", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::SoftMax)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                auto clampedValue = GetSafeAttributeValue<ValueType>(value, debugName, "SoftMax");
                GUI->setSoftMaximum(clampedValue);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'SoftMax' attribute from property '%s' into Slider", debugName);
            }
            return;
        }
        else
        {
            BaseHandler::ConsumeAttribute(GUI, attrib, attrValue, debugName);
        }
    }

    void RegisterIntSliderHandlers();
};

#endif
