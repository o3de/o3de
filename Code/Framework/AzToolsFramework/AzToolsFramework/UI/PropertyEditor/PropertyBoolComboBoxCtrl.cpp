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

//just a test to see how it would work to pop a dialog

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

        pLayout->addWidget(m_pComboBox);

        m_pComboBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pComboBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_pComboBox->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pComboBox);
        setFocusPolicy(m_pComboBox->focusPolicy());

        connect(m_pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChildComboBoxValueChange(int)));
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
        return m_pComboBox;
    }

    void PropertyBoolComboBoxCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
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
        AZ_UNUSED(GUI);
        AZ_UNUSED(attrib);
        AZ_UNUSED(attrValue);
        AZ_UNUSED(debugName);
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
