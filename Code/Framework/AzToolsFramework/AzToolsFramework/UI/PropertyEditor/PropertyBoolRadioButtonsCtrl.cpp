/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/UI/PropertyEditor/PropertyBoolRadioButtonsCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

#include <QButtonGroup>
#include <QRadioButton>
#include <QSignalBlocker>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    const int PropertyBoolRadioButtonsCtrl::s_trueButtonIndex = 1;
    const int PropertyBoolRadioButtonsCtrl::s_falseButtonIndex = 0;

    PropertyBoolRadioButtonsCtrl::PropertyBoolRadioButtonsCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_buttonGroup = new QButtonGroup(this);

        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(4);

        auto trueBtn = new QRadioButton("True", this);
        auto falseBtn = new QRadioButton("False", this);
        falseBtn->setFocusPolicy(Qt::StrongFocus);

        pLayout->addWidget(falseBtn);
        pLayout->addWidget(trueBtn);

        m_buttonGroup->addButton(falseBtn, s_falseButtonIndex);
        m_buttonGroup->addButton(trueBtn, s_trueButtonIndex);
        m_buttonGroup->setExclusive(true);

        setLayout(pLayout);
        setFocusProxy(falseBtn);
        setFocusPolicy(falseBtn->focusPolicy());

        connect(m_buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &PropertyBoolRadioButtonsCtrl::onRadioButtonSelected);
    };

    void PropertyBoolRadioButtonsCtrl::setValue(bool value)
    {
        QSignalBlocker blocker(m_buttonGroup);
        m_buttonGroup->button(value ? s_trueButtonIndex : s_falseButtonIndex)->setChecked(true);
    }

    bool PropertyBoolRadioButtonsCtrl::value() const
    {
        return (m_buttonGroup->id(m_buttonGroup->checkedButton()) == s_trueButtonIndex);
    }

    void PropertyBoolRadioButtonsCtrl::onRadioButtonSelected(int selectedButton)
    {
        emit valueChanged(selectedButton == s_trueButtonIndex);
    }

    QWidget* PropertyBoolRadioButtonsCtrl::GetFirstInTabOrder()
    {
        return m_buttonGroup->button(s_falseButtonIndex);
    }

    QWidget* PropertyBoolRadioButtonsCtrl::GetLastInTabOrder()
    {
        return m_buttonGroup->button(s_trueButtonIndex);
    }

    void PropertyBoolRadioButtonsCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    void PropertyBoolRadioButtonsCtrl::SetButtonText(bool value, const char* description)
    {
        int btnIndex = value ? s_trueButtonIndex : s_falseButtonIndex;
        m_buttonGroup->button(btnIndex)->setText(QString::fromUtf8(description));
    }

    AZ::u32 BoolPropertyRadioButtonsHandler::GetHandlerName() const
    {
        return AZ::Edit::UIHandlers::RadioButton;
    }

    QWidget* BoolPropertyRadioButtonsHandler::GetFirstInTabOrder(PropertyBoolRadioButtonsCtrl* widget)
    {
        return widget->GetFirstInTabOrder();
    }

    QWidget* BoolPropertyRadioButtonsHandler::GetLastInTabOrder(PropertyBoolRadioButtonsCtrl* widget)
    {
        return widget->GetLastInTabOrder();
    }

    void BoolPropertyRadioButtonsHandler::UpdateWidgetInternalTabbing(PropertyBoolRadioButtonsCtrl* widget)
    {
        widget->UpdateTabOrder();
    }

    QWidget* BoolPropertyRadioButtonsHandler::CreateGUI(QWidget* pParent)
    {
        PropertyBoolRadioButtonsCtrl* newCtrl = aznew PropertyBoolRadioButtonsCtrl(pParent);
        connect(newCtrl, &PropertyBoolRadioButtonsCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, newCtrl);
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    bool BoolPropertyRadioButtonsHandler::ResetGUIToDefaults(PropertyBoolRadioButtonsCtrl* GUI)
    {
        GUI->SetButtonText(false, "");
        GUI->SetButtonText(true, "");
        GUI->setValue(false);
        return true;
    }

    void BoolPropertyRadioButtonsHandler::ConsumeAttribute(PropertyBoolRadioButtonsCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        AZ_UNUSED(debugName);

        if (attrib == AZ::Edit::Attributes::TrueText)
        {
            AZStd::string desc;
            if (attrValue->Read<AZStd::string>(desc))
            {
                GUI->SetButtonText(true, desc.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::FalseText)
        {
            AZStd::string desc;
            if (attrValue->Read<AZStd::string>(desc))
            {
                GUI->SetButtonText(false, desc.c_str());
            }
        }
    }

    void BoolPropertyRadioButtonsHandler::WriteGUIValuesIntoProperty(size_t index, PropertyBoolRadioButtonsCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        bool val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool BoolPropertyRadioButtonsHandler::ReadValuesIntoGUI(size_t index, PropertyBoolRadioButtonsCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        bool val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterBoolRadioButtonsHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew BoolPropertyRadioButtonsHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyBoolRadioButtonsCtrl.cpp"
