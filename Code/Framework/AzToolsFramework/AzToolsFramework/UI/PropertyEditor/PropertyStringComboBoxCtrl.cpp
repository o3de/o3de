/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyStringComboBoxCtrl.hxx"
#include "PropertyQTConstants.h"
#include "DHQComboBox.hxx"
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyStringComboBoxCtrl::PropertyStringComboBoxCtrl(QWidget* pParent)
        : ComboBoxBase(pParent)
    {
    };

    int PropertyStringComboBoxCtrl::GetCount()
    {
        return GetComboBox()->count();
    }

    uint32_t PropertyStringComboBoxCtrl::GetCurrentIndex()
    {
        return static_cast<uint32_t>(GetComboBox()->currentIndex());
    }

    void PropertyStringComboBoxCtrl::Add(const AZStd::string& val)
    {
        if (val != "---")
        {
            const AZStd::pair<AZStd::string, AZStd::string> valueToAdd = AZStd::pair(val, val);
            ComboBoxBase::addElement(valueToAdd);
        }
        else
        {
            GetComboBox()->blockSignals(true);
            GetComboBox()->insertSeparator(GetCount());
            GetComboBox()->blockSignals(false);
        }
    }

    void PropertyStringComboBoxCtrl::Add(const AZStd::vector<AZStd::string>& vals)
    {
        ComboBoxBase::clearElements();
        for (size_t valIndex = 0; valIndex < vals.size(); valIndex++)
        {
            auto value = vals[valIndex];
            if (value != "---")
            {
                const AZStd::pair<AZStd::string, AZStd::string> valueToAdd = AZStd::pair(value, value);
                ComboBoxBase::addElement(valueToAdd);
            }
            else
            {
                GetComboBox()->blockSignals(true);
                GetComboBox()->insertSeparator(GetCount());
                GetComboBox()->blockSignals(false);
            }
        }
    }

    PropertyStringComboBoxCtrl::~PropertyStringComboBoxCtrl()
    {
    }

    void PropertyStringComboBoxCtrl::UpdateTabOrder()
    {
        setTabOrder(GetFirstInTabOrder(), GetLastInTabOrder());
    }

    template<class ValueType>
    void PropertyComboBoxHandlerCommon<ValueType>::ConsumeAttribute(PropertyStringComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::StringList)
        {
            AZStd::vector<AZStd::string> value;
            if (attrValue->Read<AZStd::vector<AZStd::string> >(value))
            {
                GUI->Add(value);
            }
            else
            {
                (void)debugName;
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'StringList' attribute from property '%s' into string combo box. Expected string vector.", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ComboBoxEditable)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->GetComboBox()->setEditable(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EditableCombBox' attribute from property '%s' into string combo box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC_CE("EditButtonVisible"))
        {
            bool visible;
            if (attrValue->Read<bool>(visible))
            {
                GUI->GetEditButton()->setVisible(visible);
            }
        }
        else if (attrib == AZ_CRC_CE("EditButtonCallback"))
        {
            if (auto* editButtonInvokable = azrtti_cast<AZ::AttributeInvocable<GenericEditResultOutcome<AZStd::string>(AZStd::string)>*>(attrValue->GetAttribute()))
            {
                GUI->SetEditButtonCallBack(editButtonInvokable->GetCallable());
            };
        }
        else if (attrib == AZ_CRC_CE("EditButtonToolTip"))
        {
            AZStd::string toolTip;
            if (attrValue->Read<AZStd::string>(toolTip))
            {
                GUI->GetEditButton()->setToolTip(toolTip.c_str());
            }
        }
    }

    QWidget* StringEnumPropertyComboBoxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyStringComboBoxCtrl* newCtrl = aznew PropertyStringComboBoxCtrl(pParent);
        connect(newCtrl, &PropertyStringComboBoxCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void StringEnumPropertyComboBoxHandler::WriteGUIValuesIntoProperty(size_t /*index*/, PropertyStringComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)node;
        AZStd::string val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool StringEnumPropertyComboBoxHandler::ReadValuesIntoGUI(size_t /*index*/, PropertyStringComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)node;
        const AZStd::string& val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterStringComboBoxHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew StringEnumPropertyComboBoxHandler());
    }

} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyStringComboBoxCtrl.cpp"
