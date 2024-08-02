/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerEntityIdComboBox.h"
#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <AzToolsFramework/UI/PropertyEditor/DHQComboBox.hxx>

#include <QBoxLayout>

void PropertyHandlerEntityIdComboBox::WriteGUIValuesIntoProperty(size_t index, PropertyEntityIdComboBoxCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    instance = GUI->value();
}

bool PropertyHandlerEntityIdComboBox::ReadValuesIntoGUI(size_t index, PropertyEntityIdComboBoxCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    GUI->setValue(instance);
    return false;
}

QWidget* PropertyHandlerEntityIdComboBox::CreateGUI(QWidget* pParent)
{
    PropertyEntityIdComboBoxCtrl* newCtrl = aznew PropertyEntityIdComboBoxCtrl(pParent);
    QObject::connect(newCtrl, &PropertyEntityIdComboBoxCtrl::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
        });
    return newCtrl;
}

void PropertyHandlerEntityIdComboBox::ConsumeAttribute(PropertyEntityIdComboBoxCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    (void)debugName;

    if (attrib == AZ_CRC_CE("EnumValue"))
    {
        AZStd::pair<AZ::EntityId, AZStd::string>  guiEnumValue;
        AZStd::pair<AZ::EntityId, AZStd::string>  enumValue;
        AZ::Edit::EnumConstant<AZ::u64> enumConstant;
        if (attrValue->Read<AZ::Edit::EnumConstant<AZ::u64>>(enumConstant))
        {
            guiEnumValue.first = AZ::EntityId(enumConstant.m_value);
            guiEnumValue.second = enumConstant.m_description;
            GUI->addEnumValue(guiEnumValue);
        }
        else
        {
            // Legacy path. Support temporarily for compatibility.
            if (attrValue->Read< AZStd::pair<AZ::EntityId, AZStd::string> >(enumValue))
            {
                guiEnumValue = static_cast<AZStd::pair<AZ::EntityId, AZStd::string>>(enumValue);
                GUI->addEnumValue(guiEnumValue);
            }
            else
            {
                AZStd::pair<AZ::EntityId, const char*>  enumValueConst;
                if (attrValue->Read< AZStd::pair<AZ::EntityId, const char*> >(enumValueConst))
                {
                    AZStd::string consted = enumValueConst.second;
                    guiEnumValue = AZStd::make_pair(enumValueConst.first, consted);
                    GUI->addEnumValue(guiEnumValue);
                }
                else
                {
                    AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box. Expected pair<IntegerType, char*> or pair<IntegerType, AZStd::string>, where IntegerType is int or u32 ", debugName);
                }
            }
        }
    }
    else if (attrib == AZ::Edit::Attributes::EnumValues)
    {
        AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string>>  guiEnumValues;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string> > enumValues;
        AZStd::vector<AZ::Edit::EnumConstant<AZ::u64>> enumConstantValues;
        if (attrValue->Read<AZStd::vector<AZ::Edit::EnumConstant<AZ::u64>>>(enumConstantValues))
        {
            for (const AZ::Edit::EnumConstant<AZ::u64>& constantValue : enumConstantValues)
            {
                auto& enumValue = guiEnumValues.emplace_back();
                enumValue.first = AZ::EntityId(constantValue.m_value);
                enumValue.second = constantValue.m_description;
            }

            GUI->addEnumValues(guiEnumValues);
        }
        else
        {
            // Legacy path. Support temporarily for compatibility.
            if (attrValue->Read< AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string> > >(enumValues))
            {
                for (auto it = enumValues.begin(); it != enumValues.end(); ++it)
                    guiEnumValues.push_back(static_cast<AZStd::pair<AZ::EntityId, AZStd::string>>(*it));
                GUI->addEnumValues(guiEnumValues);
            }
            else
            {
                AZStd::vector<AZStd::pair<AZ::EntityId, const char*> > attempt2;
                if (attrValue->Read<AZStd::vector<AZStd::pair<AZ::EntityId, const char*> > >(attempt2))
                {
                    for (auto it = attempt2.begin(); it != attempt2.end(); ++it)
                        guiEnumValues.push_back(AZStd::pair<AZ::EntityId, AZStd::string>(it->first, it->second));
                    GUI->addEnumValues(guiEnumValues);
                }
                else
                {
                    // emit a warning!
                    AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box", debugName);
                }
            }
        }
    }
}
void PropertyHandlerEntityIdComboBox::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerEntityIdComboBox());
}

PropertyEntityIdComboBoxCtrl::PropertyEntityIdComboBoxCtrl(QWidget* pParent)
    : QWidget(pParent)
{
    // create the gui, it consists of a layout, and in that layout, a text field for the value
    // and then a slider for the value.
    QHBoxLayout* pLayout = new QHBoxLayout(this);
    m_pComboBox = aznew AzToolsFramework::DHQComboBox(this);

    // many UI elements hide 1 pixel of their size in a border area that only shows up when they are selected.
    // The combo box used in this layout does not do this, so adding 1 to the left and right margins will make
    // sure that it has the same dimensions as the other UI elements when they are unselected.
    pLayout->setSpacing(4);
    pLayout->setContentsMargins(1, 0, 1, 0);

    pLayout->addWidget(m_pComboBox);

    m_pComboBox->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
    m_pComboBox->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

    m_pComboBox->setFocusPolicy(Qt::StrongFocus);
    setLayout(pLayout);
    setFocusProxy(m_pComboBox);
    setFocusPolicy(m_pComboBox->focusPolicy());

    connect(m_pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChildComboBoxValueChange(int)));
};

void PropertyEntityIdComboBoxCtrl::setValue(AZ::EntityId value)
{
    m_pComboBox->blockSignals(true);
    [[maybe_unused]] bool indexWasFound = false;
    for (size_t enumValIndex = 0; enumValIndex < m_enumValues.size(); enumValIndex++)
    {
        if (m_enumValues[enumValIndex].first == value)
        {
            m_pComboBox->setCurrentIndex(static_cast<int>(enumValIndex));
            indexWasFound = true;
            break;
        }
    }
    AZ_Warning("AzToolsFramework", indexWasFound == true, "No index in property enum for value %d", value);

    m_pComboBox->blockSignals(false);
}

void PropertyEntityIdComboBoxCtrl::addEnumValue(AZStd::pair<AZ::EntityId, AZStd::string>& val)
{
    m_pComboBox->blockSignals(true);
    m_enumValues.push_back(val);
    m_pComboBox->addItem(val.second.c_str());
    m_pComboBox->blockSignals(false);
}

void PropertyEntityIdComboBoxCtrl::addEnumValues(AZStd::vector< AZStd::pair<AZ::EntityId, AZStd::string> >& vals)
{
    m_pComboBox->blockSignals(true);
    for (size_t valIndex = 0; valIndex < vals.size(); valIndex++)
    {
        m_enumValues.push_back(vals[valIndex]);
        m_pComboBox->addItem(vals[valIndex].second.c_str());
    }
    m_pComboBox->blockSignals(false);
}

AZ::EntityId PropertyEntityIdComboBoxCtrl::value() const
{
    AZ_Assert(m_pComboBox->currentIndex() >= 0 &&
        m_pComboBox->currentIndex() < static_cast<int>(m_enumValues.size()), "Out of range combo box index %d", m_pComboBox->currentIndex());
    return m_enumValues[m_pComboBox->currentIndex()].first;
}

void PropertyEntityIdComboBoxCtrl::onChildComboBoxValueChange(int comboBoxIndex)
{
    AZ_Assert(comboBoxIndex >= 0 &&
        comboBoxIndex < static_cast<int>(m_enumValues.size()), "Out of range combo box index %d", comboBoxIndex);

    emit valueChanged(m_enumValues[comboBoxIndex].first);
}

QWidget* PropertyEntityIdComboBoxCtrl::GetFirstInTabOrder()
{
    return m_pComboBox;
}
QWidget* PropertyEntityIdComboBoxCtrl::GetLastInTabOrder()
{
    return m_pComboBox;
}

void PropertyEntityIdComboBoxCtrl::UpdateTabOrder()
{
    // There's only one QT widget on this property.
}

#include <moc_PropertyHandlerEntityIdComboBox.cpp>
