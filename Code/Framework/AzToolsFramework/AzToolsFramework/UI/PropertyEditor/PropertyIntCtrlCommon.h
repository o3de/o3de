/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <AzToolsFramework/UI/PropertyEditor/QtWidgetLimits.h>
#include <QtWidgets/QWidget>
#include <QLocale> 

namespace AzToolsFramework
{   
    // Tool tip string generation for signed and unsigned integer type limits
    template<typename T, typename ControlType, bool IsSigned = std::is_signed<T>::value>
    struct ToolTip 
    {
        static bool Modify(QWidget* widget, QString& toolTipString)
        {
            if constexpr (IsSigned)
            {
                return SignedToolTip(widget, toolTipString);
            }
            else
            {
                return UnsignedToolTip(widget, toolTipString);
            }
        }        

    private:
        static bool SignedToolTip(QWidget* widget, QString& toolTipString);
        static bool UnsignedToolTip(QWidget* widget, QString& toolTipString);
    };

    //! Base class for integer widget handlers to provide functionality independent
    //! of widget type.
    //! @tparam ValueType The integer primitive type of the handler.
    //! @tparam PropertyControl The widget type of the handler.
    //! @tparam HandlerQObject The QObject base to for this handler.
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    class IntWidgetHandler
        : public PropertyHandler<ValueType, PropertyControl>
        , protected HandlerQObject
    {
    public:
        AZ_CLASS_ALLOCATOR(IntWidgetHandler, AZ::SystemAllocator);
    protected:
        QWidget* GetFirstInTabOrder(PropertyControl* widget) override;
        QWidget* GetLastInTabOrder(PropertyControl* widget) override;
        void UpdateWidgetInternalTabbing(PropertyControl* widget) override;
        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyControl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    public:
        bool ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyControl* GUI, const ValueType& instance, [[maybe_unused]] InstanceDataNode* node)  override;
        void WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, PropertyControl* GUI, ValueType& instance, [[maybe_unused]] InstanceDataNode* node) override;
    };

    // Clamps the attribute value to not exceed the range of either ValueType or QtWidgetValueType
    template <typename ValueType>
    AZ::s64 GetSafeAttributeValue(AZ::s64 value, [[maybe_unused]]const char* debugName, [[maybe_unused]]const char* attribName)
    {
        auto [clampedValue, overflow] = QtWidgetLimits<ValueType>::Clamp(value);
        
        // The value exceeded the range of either ValueType or QtWidgetValueType
        if (overflow)
        {
            AZ_Warning(
                "AzToolsFramework", false, 
                "Property '%s': '%s' attribute value '%i' for widget exceeds [%i, %i] range, value has been clamped back to range.", 
                debugName, attribName, value, QtWidgetLimits<ValueType>::Min(), QtWidgetLimits<ValueType>::Max());
        }

        return clampedValue;
    }

    // Tooltip for signed integer types
    template<typename T, typename ControlType, bool IsSigned>
    bool ToolTip<T, ControlType, IsSigned>::SignedToolTip(QWidget* widget, QString& toolTipString)
    {
        ControlType* propertyControl = qobject_cast<ControlType*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }

            const QString minString = QLocale().toString(propertyControl->minimum());
            const QString maxString = QLocale().toString(propertyControl->maximum());
            toolTipString += QString("[%1, %2]").arg(minString).arg(maxString);

            return true;
        }
        return false;
    }

    // Tooltip for unsigned integer types
    template<typename T, typename ControlType, bool IsSigned>
    bool ToolTip<T, ControlType, IsSigned>::UnsignedToolTip(QWidget* widget, QString& toolTipString)
    {
        ControlType* propertyControl = qobject_cast<ControlType*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }

            const QString minString = QLocale().toString(propertyControl->minimum());
            const QString maxString = QLocale().toString(propertyControl->maximum());
            toolTipString += QString("[%1, %2]").arg(minString).arg(maxString);

            return true;
        }
        return false;
    }

    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    QWidget* IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::GetFirstInTabOrder(PropertyControl* widget)
    { 
        return widget->GetFirstInTabOrder(); 
    }
    
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    QWidget* IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::GetLastInTabOrder(PropertyControl* widget)
    { 
        return widget->GetLastInTabOrder(); 
    }
    
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    void IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::UpdateWidgetInternalTabbing(PropertyControl* widget)
    { 
        widget->UpdateTabOrder(); 
    }
    
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    QWidget* IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::CreateGUI(QWidget* pParent)
    {
        PropertyControl* newCtrl = aznew PropertyControl(pParent);
        this->connect(newCtrl, &PropertyControl::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
        });
        this->connect(newCtrl, &PropertyControl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // Set the value range to that of ValueType as clamped to the range of QtWidgetValueType
        newCtrl->setMinimum(QtWidgetLimits<ValueType>::Min());
        newCtrl->setMaximum(QtWidgetLimits<ValueType>::Max());

        return newCtrl;
    }

    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    void IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::ConsumeAttribute(
        PropertyControl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        AZ::s64 value{0};
        if (attrib == AZ::Edit::Attributes::Min)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                auto clampedValue = GetSafeAttributeValue<ValueType>(value, debugName, "Min");
                GUI->setMinimum(clampedValue);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Min' attribute from property '%s' into widget", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                auto clampedValue = GetSafeAttributeValue<ValueType>(value, debugName, "Max");
                GUI->setMaximum(clampedValue);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Max' attribute from property '%s' into widget", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setStep(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Step' attribute from property '%s' into widget", debugName);
            }
        }
        else if (attrib == AZ_CRC_CE("Multiplier"))
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setMultiplier(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Multiplier' attribute from property '%s' into widget", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string stringValue;
            if (attrValue->Read<AZStd::string>(stringValue))
            {
                GUI->setSuffix(stringValue.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Suffix' attribute from property '%s' into widget", debugName);
            }
        }
        else if (attrib == AZ_CRC_CE("Prefix"))
        {
            AZStd::string stringValue;
            if (attrValue->Read<AZStd::string>(stringValue))
            {
                GUI->setPrefix(stringValue.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Prefix' attribute from property '%s' into widget", debugName);
            }
        }
    }

    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    void IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index, PropertyControl* GUI, ValueType& instance, [[maybe_unused]] InstanceDataNode* node)
    {
        AZ::s64 val = GUI->value();
        instance = aznumeric_cast<ValueType>(val);
    }
    
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    bool IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::ReadValuesIntoGUI([[maybe_unused]] size_t index, 
                                                                                         PropertyControl* GUI, 
                                                                                         const ValueType& instance, 
                                                                                         [[maybe_unused]] InstanceDataNode* node)
    {
        GUI->setValue(instance);
        return false;
    }
    
    template <typename ValueType, typename PropertyControl, typename HandlerQObject>
    bool IntWidgetHandler<ValueType, PropertyControl, HandlerQObject>::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ToolTip<ValueType, PropertyControl>::Modify(widget, toolTipString);
    }
} // AzToolsFramework
