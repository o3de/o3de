/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyCRCCtrl.h"
#include "PropertyQTConstants.h"

#include <AzCore/Math/Crc.h>

#include <QRegExp>
#include <QRegExpValidator>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QString>
#include <QEvent>

namespace AzToolsFramework
{
    PropertyCRCCtrl::PropertyCRCCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        setFocusPolicy(Qt::StrongFocus);
        
        QRegExp hexes("(0x)?([0-9a-fA-F]{1,8})", Qt::CaseInsensitive);
        QRegExpValidator *validator = new QRegExpValidator(hexes, this);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_lineEdit->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_lineEdit->setFixedHeight(PropertyQTConstant_DefaultHeight);
        m_lineEdit->setFocusPolicy(Qt::StrongFocus);
        m_lineEdit->setValidator(validator);
        connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onLineEditChange(QString)));

        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);
        pLayout->addWidget(m_lineEdit);
        setLayout(pLayout);
        setFocusProxy(m_lineEdit);
        
        m_lineEdit->installEventFilter(this);
    };

    bool PropertyCRCCtrl::eventFilter(QObject* target, QEvent* event)
    {
        if (target == m_lineEdit)
        {
            if (event->type() == QEvent::FocusIn)
            {
                FocusChangedEdit(true);
            }
            else if (event->type() == QEvent::FocusOut)
            {
                FocusChangedEdit(false);
            }
        }
        return QWidget::eventFilter(target, event);
    }

    QWidget* PropertyCRCCtrl::GetFirstInTabOrder()
    {
        return m_lineEdit;
    }
    QWidget* PropertyCRCCtrl::GetLastInTabOrder()
    {
        return m_lineEdit;
    }

    void PropertyCRCCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property, no reason to deal with tab order changes internally.
    }

    void PropertyCRCCtrl::setValue(AZ::u32 value)
    {
        m_lineEdit->blockSignals(true);
        m_currentValue = value;
        UpdateValueText();
        m_lineEdit->blockSignals(false);
    }

    void PropertyCRCCtrl::UpdateValueText()
    {
        QString textRepresentation = QString("0x%1").arg(m_currentValue, 8, 16, QChar('0'));
        m_lineEdit->setText(textRepresentation);
    }

    void PropertyCRCCtrl::FocusChangedEdit(bool hasFocusNow)
    {
        if (hasFocusNow)
        {
            m_lineEdit->selectAll();
        }
        else
        {
            m_lineEdit->blockSignals(true);
            UpdateValueText();
            m_lineEdit->blockSignals(false);
        }
    }

    AZ::u32 PropertyCRCCtrl::value() const
    {
        return m_currentValue;
    }

    void PropertyCRCCtrl::onLineEditChange(QString newText)
    {
        // parse newText.
        QRegExp hexes("(0x)?([0-9a-fA-F]{1,8})", Qt::CaseInsensitive);
        if ((hexes.exactMatch(newText) && hexes.captureCount() > 1))
        {
            QString actualCap = hexes.cap(2);
            // this will be a string like FF11BB
            bool ok = false;
            AZ::u32 newValue = actualCap.toUInt(&ok, 16);
            if ((ok) && (m_currentValue != newValue))
            {
                m_currentValue = newValue;
                Q_EMIT valueChanged(m_currentValue);
            }
        }
    }

    PropertyCRCCtrl::~PropertyCRCCtrl()
    {
    }

    void U32CRCHandler::ConsumeAttribute(PropertyCRCCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(GUI)
        Q_UNUSED(attrib)
        Q_UNUSED(attrValue)
        Q_UNUSED(debugName)
    }

    QWidget* U32CRCHandler::CreateGUI(QWidget* pParent)
    {
        PropertyCRCCtrl* newCtrl = aznew PropertyCRCCtrl(pParent);

        auto requestWriteCall = [newCtrl](AZ::u32)
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        };

        QObject::connect(newCtrl, &PropertyCRCCtrl::valueChanged, this, requestWriteCall);

        return newCtrl;
    }

    void U32CRCHandler::WriteGUIValuesIntoProperty(size_t index, PropertyCRCCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZ::u32 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool U32CRCHandler::ReadValuesIntoGUI(size_t index, PropertyCRCCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->setValue(instance);
        return false;
    }

    void CRC32Handler::ConsumeAttribute(PropertyCRCCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(GUI)
        Q_UNUSED(attrib)
        Q_UNUSED(attrValue)
        Q_UNUSED(debugName)
    }

    QWidget* CRC32Handler::CreateGUI(QWidget* pParent)
    {
        PropertyCRCCtrl* newCtrl = aznew PropertyCRCCtrl(pParent);

        auto requestWriteCall = [newCtrl](AZ::u32)
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        };

        QObject::connect(newCtrl, &PropertyCRCCtrl::valueChanged, this, requestWriteCall);

        return newCtrl;
    }

    void CRC32Handler::WriteGUIValuesIntoProperty(size_t index, PropertyCRCCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZ::u32 val = GUI->value();
        instance = static_cast<property_t>(AZ::Crc32(val));
    }

    bool CRC32Handler::ReadValuesIntoGUI(size_t index, PropertyCRCCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->setValue(static_cast<property_t>(instance));
        return false;
    }


    void RegisterCrcHandler()
    {
        PropertyTypeRegistrationMessageBus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew U32CRCHandler());
        PropertyTypeRegistrationMessageBus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew CRC32Handler());
        
    }
}

#include "UI/PropertyEditor/moc_PropertyCRCCtrl.cpp"
