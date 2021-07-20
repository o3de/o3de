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
#include <QtWidgets/QComboBox>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyStringComboBoxCtrl::PropertyStringComboBoxCtrl(QWidget* pParent)
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

    void PropertyStringComboBoxCtrl::setValue(const AZStd::string& str)
    {
        m_pComboBox->blockSignals(true);

        bool indexWasFound = false;
        if (str.empty())
        {
            // Value may just not be populated. Don't warn if that's the case.
            indexWasFound = true;
            m_pComboBox->setCurrentIndex(-1);
        }

        for (size_t enumValIndex = 0; enumValIndex < m_values.size(); enumValIndex++)
        {
            if (m_values[enumValIndex].compare(str.c_str()) == 0)
            {
                m_pComboBox->setCurrentIndex(static_cast<int>(enumValIndex));
                indexWasFound = true;
                break;
            }
        }

        if (!indexWasFound)
        {
            if (m_pComboBox->isEditable())
            {
                m_pComboBox->setEditText(QString(str.c_str()));
            }
            else
            {
                // item not found void out the combo box
                m_pComboBox->setCurrentIndex(-1);
            }
        }

        AZ_Warning("AzToolsFramework", indexWasFound == true, "No index in property enum for value %s", str.c_str());

        m_pComboBox->blockSignals(false);
    }

    int PropertyStringComboBoxCtrl::GetCount() const
    {
        return m_pComboBox->count();
    }

    uint32_t PropertyStringComboBoxCtrl::GetCurrentIndex() const
    {
        return static_cast<uint32_t>(m_pComboBox->currentIndex());
    }

    void PropertyStringComboBoxCtrl::Add(const AZStd::string& val)
    {
        m_pComboBox->blockSignals(true);
        m_values.push_back(val);
        if (val != "---")
        {
            m_pComboBox->addItem(val.c_str());
        }
        else
        {
            m_pComboBox->insertSeparator(m_pComboBox->count());
        }
        m_pComboBox->blockSignals(false);
    }

    void PropertyStringComboBoxCtrl::Add(const AZStd::vector<AZStd::string>& vals)
    {
        m_pComboBox->blockSignals(true);
        m_values.clear();
        m_pComboBox->clear();
        for (size_t valIndex = 0; valIndex < vals.size(); valIndex++)
        {
            m_values.push_back(vals[valIndex]);
            if (vals[valIndex] != "---")
            {
                m_pComboBox->addItem(vals[valIndex].c_str());
            }
            else
            {
                m_pComboBox->insertSeparator(m_pComboBox->count());
            }
        }
        m_pComboBox->blockSignals(false);
    }

    AZStd::string PropertyStringComboBoxCtrl::value() const
    {
        AZ_Assert(m_pComboBox->currentIndex() >= 0 &&
            m_pComboBox->currentIndex() < static_cast<int>(m_values.size()), "Out of range combo box index %d", m_pComboBox->currentIndex());
        return m_values[m_pComboBox->currentIndex()];
    }

    void PropertyStringComboBoxCtrl::onChildComboBoxValueChange(int comboBoxIndex)
    {
        AZ_Assert(comboBoxIndex >= 0 &&
            comboBoxIndex < static_cast<int>(m_values.size()), "Out of range combo box index %d", comboBoxIndex);

        emit valueChanged(m_values[comboBoxIndex]);
    }

    PropertyStringComboBoxCtrl::~PropertyStringComboBoxCtrl()
    {
    }

    QWidget* PropertyStringComboBoxCtrl::GetFirstInTabOrder()
    {
        return m_pComboBox;
    }
    QWidget* PropertyStringComboBoxCtrl::GetLastInTabOrder()
    {
        return m_pComboBox;
    }

    void PropertyStringComboBoxCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
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
                GUI->m_pComboBox->setEditable(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EditableCombBox' attribute from property '%s' into string combo box", debugName);
            }
            return;
        }
    }

    QWidget* StringEnumPropertyComboBoxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyStringComboBoxCtrl* newCtrl = aznew PropertyStringComboBoxCtrl(pParent);
        connect(newCtrl, &PropertyStringComboBoxCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
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
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew StringEnumPropertyComboBoxHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyStringComboBoxCtrl.cpp"
