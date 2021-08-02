/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyEnumComboBoxCtrl.hxx"
#include "PropertyQTConstants.h"
#include "DHQComboBox.hxx"
#include <QtWidgets/QComboBox>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyEnumComboBoxCtrl::PropertyEnumComboBoxCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pComboBox = aznew DHQComboBox(this);

        // many UI elements hide 1 pixel of their size in a border area that only shows up when they are selected.
        // The combo box used in this layout does not do this, so adding 1 to the left and right margins will make
        // sure that it has the same dimensions as the other UI elements when they are unselected.
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

    void PropertyEnumComboBoxCtrl::setValue(AZ::s64 value)
    {
        m_pComboBox->blockSignals(true);
        bool indexWasFound = false;
        for (size_t enumValIndex = 0; enumValIndex < m_enumValues.size(); enumValIndex++)
        {
            if (m_enumValues[enumValIndex].first == value)
            {
                m_pComboBox->setCurrentIndex(static_cast<int>(enumValIndex));
                indexWasFound = true;
                break;
            }
        }

        AZ_Warning("AzToolsFramework", indexWasFound == true || (value == 0 && m_enumValues.size() > 0), "No index in property enum for value %d", value);

        m_pComboBox->blockSignals(false);
    }

    void PropertyEnumComboBoxCtrl::addEnumValue(AZStd::pair<AZ::s64, AZStd::string>& val)
    {
        m_pComboBox->blockSignals(true);
        if (AZStd::find(m_enumValues.begin(), m_enumValues.end(), val) == m_enumValues.end())
        {
            m_enumValues.push_back(val);
            m_pComboBox->addItem(val.second.c_str());
        }
        m_pComboBox->blockSignals(false);
    }

    void PropertyEnumComboBoxCtrl::setEnumValues(AZStd::vector< AZStd::pair<AZ::s64, AZStd::string> >& vals)
    {
        m_pComboBox->blockSignals(true);
        m_pComboBox->clear();
        m_enumValues.clear();
        for (size_t valIndex = 0; valIndex < vals.size(); valIndex++)
        {
            if (AZStd::find(m_enumValues.begin(), m_enumValues.end(), vals[valIndex]) == m_enumValues.end())
            {
                m_enumValues.push_back(vals[valIndex]);
                m_pComboBox->addItem(vals[valIndex].second.c_str());
            }
        }
        m_pComboBox->blockSignals(false);
    }

    AZ::s64 PropertyEnumComboBoxCtrl::value() const
    {
        AZ_Assert(m_pComboBox->currentIndex() >= 0 &&
            m_pComboBox->currentIndex() < static_cast<int>(m_enumValues.size()), "Out of range combo box index %d", m_pComboBox->currentIndex());
        return m_enumValues[m_pComboBox->currentIndex()].first;
    }

    void PropertyEnumComboBoxCtrl::onChildComboBoxValueChange(int comboBoxIndex)
    {
        AZ_Assert(comboBoxIndex >= 0 &&
            comboBoxIndex < static_cast<int>(m_enumValues.size()), "Out of range combo box index %d", comboBoxIndex);

        emit valueChanged(m_enumValues[comboBoxIndex].first);
    }

    PropertyEnumComboBoxCtrl::~PropertyEnumComboBoxCtrl()
    {
    }

    QWidget* PropertyEnumComboBoxCtrl::GetFirstInTabOrder()
    {
        return m_pComboBox;
    }
    QWidget* PropertyEnumComboBoxCtrl::GetLastInTabOrder()
    {
        return m_pComboBox;
    }

    void PropertyEnumComboBoxCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    void RegisterEnumComboBoxHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew defaultEnumPropertyComboBoxHandler());

        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew charEnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew s8EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew u8EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew s16EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew u16EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew u32EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew s64EnumPropertyComboBoxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew u64EnumPropertyComboBoxHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyEnumComboBoxCtrl.cpp"
