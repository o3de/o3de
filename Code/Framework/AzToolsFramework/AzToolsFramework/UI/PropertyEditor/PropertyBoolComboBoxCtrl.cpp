/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyBoolComboBoxCtrl.hxx"
#include "PropertyQTConstants.h"
#include "DHQComboBox.hxx"
#include <QtWidgets/QComboBox>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyBoolComboBoxCtrl::PropertyBoolComboBoxCtrl(QWidget* pParent)
        : ComboBoxBase(pParent)
    {
        AZStd::vector<AZStd::pair<bool, AZStd::string>> options = { AZStd::pair(false, "False"), AZStd::pair(true, "True") };
        ComboBoxBase::addElements(options);
    };

    PropertyBoolComboBoxCtrl::~PropertyBoolComboBoxCtrl()
    {
    }

    void PropertyBoolComboBoxCtrl::UpdateTabOrder()
    {
        setTabOrder(GetFirstInTabOrder(), GetLastInTabOrder());
    }

    QWidget* BoolPropertyComboBoxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyBoolComboBoxCtrl* newCtrl = aznew PropertyBoolComboBoxCtrl(pParent);
        connect(newCtrl, &PropertyBoolComboBoxCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void BoolPropertyComboBoxHandler::ConsumeAttribute(PropertyBoolComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        AZ_UNUSED(debugName);

        if (attrib == AZ_CRC_CE("EditButtonVisible"))
        {
            bool visible = false;
            if (attrValue->Read<bool>(visible))
            {
                GUI->GetEditButton()->setVisible(visible);
            }
        }
        else if (attrib == AZ_CRC_CE("SetTrueLabel"))
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->GetComboBox()->setItemText(1, label.c_str());
            }
        }
        else if (attrib == AZ_CRC_CE("SetFalseLabel"))
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->GetComboBox()->setItemText(0, label.c_str());
            }
        }
        else if (attrib == AZ_CRC_CE("EditButtonCallback"))
        {
            if (auto* editButtonInvokable = azrtti_cast<AZ::AttributeInvocable<GenericEditResultOutcome<bool>(bool)>*>(attrValue->GetAttribute()))
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

    void BoolPropertyComboBoxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyBoolComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        bool val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool BoolPropertyComboBoxHandler::ReadValuesIntoGUI(size_t index, PropertyBoolComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        bool val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterBoolComboBoxHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew BoolPropertyComboBoxHandler());
    }
}

#include "UI/PropertyEditor/moc_PropertyBoolComboBoxCtrl.cpp"
