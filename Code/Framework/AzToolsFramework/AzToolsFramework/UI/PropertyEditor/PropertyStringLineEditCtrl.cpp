/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyStringLineEditCtrl.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/StyledLineEdit.h>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QtWidgets/QHBoxLayout>
#include <QFocusEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyStringLineEditCtrl::PropertyStringLineEditCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pLineEdit = new AzQtComponents::StyledLineEdit(this);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        pLayout->addWidget(m_pLineEdit);

        m_pLineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pLineEdit->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pLineEdit->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_pLineEdit->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pLineEdit);
        setFocusPolicy(m_pLineEdit->focusPolicy());
    };

    void PropertyStringLineEditCtrl::setValue(AZStd::string& value)
    {
        QString text = m_pLineEdit->text();
        if (text.compare(value.data()) != 0)
        {
            m_pLineEdit->blockSignals(true);
            m_pLineEdit->setText(value.c_str());
            m_pLineEdit->blockSignals(false);
        }
    }

    void PropertyStringLineEditCtrl::focusInEvent(QFocusEvent* e)
    {
        m_pLineEdit->event(e);
        m_pLineEdit->selectAll();
    }

    void PropertyStringLineEditCtrl::UpdateValue(const QString& newValue)
    {
        if (m_pLineEdit->text() != newValue)
        {
            m_pLineEdit->setText(newValue);
            m_pLineEdit->editingFinished();
        }
    }

    AZStd::string PropertyStringLineEditCtrl::value() const
    {
        return AZStd::string(m_pLineEdit->text().toUtf8().data());
    }

    QLineEdit* PropertyStringLineEditCtrl::GetLineEdit() const
    {
        return m_pLineEdit;
    }

    void PropertyStringLineEditCtrl::setMaxLen(int maxLen)
    {
        m_pLineEdit->blockSignals(true);
        m_pLineEdit->setMaxLength(maxLen);
        m_pLineEdit->blockSignals(false);
    }

    PropertyStringLineEditCtrl::~PropertyStringLineEditCtrl()
    {
    }


    QWidget* PropertyStringLineEditCtrl::GetFirstInTabOrder()
    {
        return m_pLineEdit;
    }
    QWidget* PropertyStringLineEditCtrl::GetLastInTabOrder()
    {
        return m_pLineEdit;
    }

    void PropertyStringLineEditCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    QWidget* StringPropertyLineEditHandler::CreateGUI(QWidget* pParent)
    {
        PropertyStringLineEditCtrl* newCtrl = aznew PropertyStringLineEditCtrl(pParent);
        connect(newCtrl->GetLineEdit(), &QLineEdit::editingFinished, this, [newCtrl]()
            {
                PropertyEditorGUIMessagesBus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, newCtrl);
                PropertyEditorGUIMessagesBus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void StringPropertyLineEditHandler::ConsumeAttribute(PropertyStringLineEditCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
         Q_UNUSED(debugName);

        GUI->blockSignals(true);
        if (attrib == AZ::Edit::Attributes::MaxLength)
        {
            AZ::s64 value;
            if (attrValue->Read<int>(value))
            {
                GUI->setMaxLen(static_cast<int>(value));
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'MaxLength' attribute from property '%s' into text field", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::PlaceholderText)
        {
            AZStd::string placeholderText;
            if (attrValue->Read<AZStd::string>(placeholderText))
            {
                GUI->GetLineEdit()->setPlaceholderText(placeholderText.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false,
                    "Failed to read 'PlaceholderText' attribute from property '%s' into string text field.", debugName);
            }
        }
        GUI->blockSignals(false);
    }

    void StringPropertyLineEditHandler::WriteGUIValuesIntoProperty(size_t index, PropertyStringLineEditCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZStd::string val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool StringPropertyLineEditHandler::ReadValuesIntoGUI(size_t index, PropertyStringLineEditCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZStd::string val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterStringLineEditHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew StringPropertyLineEditHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyStringLineEditCtrl.cpp"
