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
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pComboBox = aznew DHQComboBox(this);
        m_pComboBox->addItem("False");
        m_pComboBox->addItem("True");

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        m_editButton = new QToolButton();
        m_editButton->setAutoRaise(true);
        m_editButton->setToolTip(QString("Edit"));
        m_editButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        m_editButton->setVisible(false);

        pLayout->addWidget(m_pComboBox);
        pLayout->addWidget(m_editButton);

        m_pComboBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pComboBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_pComboBox->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pComboBox);
        setFocusPolicy(m_pComboBox->focusPolicy());

        connect(m_pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChildComboBoxValueChange(int)));
        connect(m_editButton, &QToolButton::clicked, this, &PropertyBoolComboBoxCtrl::OnEditButtonClicked);
    };

    void PropertyBoolComboBoxCtrl::setValue(bool value)
    {
        m_pComboBox->blockSignals(true);
        m_pComboBox->setCurrentIndex(value ? 1 : 0);
        m_pComboBox->blockSignals(false);
    }

    bool PropertyBoolComboBoxCtrl::value() const
    {
        return m_pComboBox->currentIndex() == 1;
    }

    void PropertyBoolComboBoxCtrl::onChildComboBoxValueChange(int newValue)
    {
        emit valueChanged(newValue == 0 ? false : true);
    }

    PropertyBoolComboBoxCtrl::~PropertyBoolComboBoxCtrl()
    {
    }

    QWidget* PropertyBoolComboBoxCtrl::GetFirstInTabOrder()
    {
        return m_pComboBox;
    }
    QWidget* PropertyBoolComboBoxCtrl::GetLastInTabOrder()
    {
        return m_editButton;
    }

    void PropertyBoolComboBoxCtrl::UpdateTabOrder()
    {
        setTabOrder(GetFirstInTabOrder(), GetLastInTabOrder());
    }

    QComboBox* PropertyBoolComboBoxCtrl::GetComboBox()
    {
        return m_pComboBox;
    }

    QToolButton* PropertyBoolComboBoxCtrl::GetEditButton()
    {
        return m_editButton;
    }

    void PropertyBoolComboBoxCtrl::SetEditButtonCallBack(ComboBoxSelectFunc function)
    {
        m_editButtonCallback = function;
    }

    void PropertyBoolComboBoxCtrl::OnEditButtonClicked()
    {
        if (m_editButtonCallback)
        {
            // indexBool refers to the bool of the index value that will be set to on the return of the invoked callback
            auto indexBool = m_editButtonCallback->Invoke(nullptr, m_pComboBox->currentIndex());
            m_pComboBox->setCurrentIndex(indexBool ? 1 : 0);
        }
    }

    QWidget* BoolPropertyComboBoxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyBoolComboBoxCtrl* newCtrl = aznew PropertyBoolComboBoxCtrl(pParent);
        connect(newCtrl, &PropertyBoolComboBoxCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
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
            ComboBoxSelectFunc function = azdynamic_cast<ComboBoxSelectFunc>(attrValue->GetAttribute());

            if (function)
            {
                GUI->SetEditButtonCallBack(function);
            }
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
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew BoolPropertyComboBoxHandler());
    }
}

#include "UI/PropertyEditor/moc_PropertyBoolComboBoxCtrl.cpp"
