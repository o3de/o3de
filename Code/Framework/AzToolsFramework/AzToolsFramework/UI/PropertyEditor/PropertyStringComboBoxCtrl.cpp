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
#include <QHBoxLayout>
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
