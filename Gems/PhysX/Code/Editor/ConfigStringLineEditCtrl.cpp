/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/Material.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <AzQtComponents/Components/StyledLineEdit.h>

#include <QtWidgets/QHBoxLayout>
#include <QFocusEvent>

#include "ConfigStringLineEditCtrl.h"

namespace PhysX
{
    ConfigStringLineEditCtrl::ConfigStringLineEditCtrl(QWidget* parent
        , ConfigStringLineEditValidator* validator)
        : AzToolsFramework::PropertyStringLineEditCtrl(parent)
        , m_pValidator(validator)
    {
        ConnectWidgets(); // Connections need to be done on top of parent class's connections.
    };

    AZStd::string ConfigStringLineEditCtrl::Value() const
    {
        return AZStd::string(m_pLineEdit->text().toUtf8().data());
    }

    void ConfigStringLineEditCtrl::setValue(AZStd::string& value)
    {
        QString text = m_pLineEdit->text();
        if (text.compare(value.data()) != 0)
        {
            m_pLineEdit->blockSignals(true);
            QString stringIn(value.c_str());
            m_pLineEdit->setText(stringIn);
            if (m_pValidator)
            {
                // Manually use validator when value is set (by reflection). Correct value if necessary.
                OnEditStart(false);
                int unused = 0;
                if (m_pValidator->validate(stringIn, unused) != QValidator::Acceptable)
                {
                    m_pValidator->fixup(stringIn);
                    m_pLineEdit->setText(stringIn);
                    AZStd::string newValue(stringIn.toUtf8().data());
                    emit AzToolsFramework::PropertyStringLineEditCtrl::valueChanged(newValue);
                }
                OnEditEnd();
            }
            m_pLineEdit->blockSignals(false);
        }
    }

    void ConfigStringLineEditCtrl::OnEditStart(bool removeEditedString)
    {
        if (!m_pValidator)
        {
            return;
        }
        m_pValidator->OnEditStart(m_uniqueGroup
            , Value()
            , m_forbiddenStrings
            , m_pLineEdit->maxLength()
            , removeEditedString);
    }

    void ConfigStringLineEditCtrl::OnEditEnd()
    {
        if (!m_pValidator)
        {
            return;
        }
        AZStd::string newValue = Value();
        m_pValidator->OnEditEnd(m_uniqueGroup, newValue);
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
    }

    ConfigStringLineEditCtrl::~ConfigStringLineEditCtrl()
    {
        if (m_pValidator)
        {
            m_pValidator->RemoveUniqueString(m_uniqueGroup, Value());
        }
    }

    void ConfigStringLineEditCtrl::SetForbiddenStrings(const UniqueStringContainer::StringSet& forbiddenStrings)
    {
        m_forbiddenStrings = forbiddenStrings;
    }

    void ConfigStringLineEditCtrl::SetUniqueGroup(AZ::Crc32 uniqueGroup)
    {
        m_uniqueGroup = uniqueGroup;
    }

    void ConfigStringLineEditCtrl::ConnectWidgets()
    {
        if (m_pValidator)
        {
            m_pLineEdit->setValidator(m_pValidator);
            connect(m_pLineEdit, SIGNAL(onFocus()), this, SLOT(OnEditStart()));
            connect(m_pLineEdit, SIGNAL(onFocusOut()), this, SLOT(OnEditEnd()));
        }
    }

    const AZ::Crc32 ConfigStringLineEditValidator::s_groupStringNotUnique = AZ_CRC("GroupStringNotUnique", 0xad22cd3d);

    ConfigStringLineEditValidator::ConfigStringLineEditValidator(QObject* parent)
        : QValidator(parent)
    {
    }

    void ConfigStringLineEditValidator::OnEditStart(AZ::Crc32 stringGroupId
        , const AZStd::string& stringToEdit
        , const UniqueStringContainer::StringSet& forbiddenStrings
        , int stringMaxLength
        , bool removeEditedString)
    {
        if (removeEditedString)
        {
            m_uniqueStringContainer.RemoveString(stringGroupId, stringToEdit);
        }
        m_currStringGroup = stringGroupId;
        m_currStringMaxLen = stringMaxLength;
        m_forbiddenStrings = forbiddenStrings;
    }

    void ConfigStringLineEditValidator::OnEditEnd(AZ::Crc32 stringGroupId
        , const AZStd::string& stringEditFinished)
    {
        // If string does not belong in group where string values are not kept unique.
        // i.e. If string value must be kept unique within group.
        if (m_currStringGroup != s_groupStringNotUnique)
        {
            m_uniqueStringContainer.AddString(stringGroupId, stringEditFinished);
        }

        // Reset current string group to default and clearing set of forbidden strings after editing is finished.
        m_currStringGroup = s_groupStringNotUnique;
        m_forbiddenStrings.clear();
    }

    void ConfigStringLineEditValidator::RemoveUniqueString(AZ::Crc32 stringGroupId
        , const AZStd::string& stringIn)
    {
        m_uniqueStringContainer.RemoveString(stringGroupId, stringIn);
    }

    void ConfigStringLineEditValidator::fixup(QString& input) const
    {
        AZStd::string stringIn(input.toUtf8().data());
        AZStd::string stringFixed = m_uniqueStringContainer.GetUniqueString(m_currStringGroup
            , stringIn
            , m_currStringMaxLen
            , m_forbiddenStrings);
        input = QString(stringFixed.c_str());
    }

    QValidator::State ConfigStringLineEditValidator::validate(QString& input, int&) const
    {
        AZStd::string stringIn(input.toUtf8().data());
        if (m_forbiddenStrings.find(stringIn) != m_forbiddenStrings.end())
        {
            return QValidator::Intermediate;
        }

        if (stringIn.empty())
        {
            return QValidator::Intermediate;
        }

        // if current group does not require string value to be unique within group
        if (m_currStringGroup == s_groupStringNotUnique)
        {
            return QValidator::Acceptable;
        }

        if (m_uniqueStringContainer.IsStringUnique(m_currStringGroup, stringIn))
        {
            return QValidator::Acceptable;
        }
        else
        {
            return QValidator::Intermediate;
        }
    }

    QWidget* ConfigStringLineEditHandler::CreateGUI(QWidget* pParent)
    {
        ConfigStringLineEditCtrl* newCtrl = aznew ConfigStringLineEditCtrl(pParent
            , &m_validator);
        connect(newCtrl, &ConfigStringLineEditCtrl::valueChanged, this, [newCtrl]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        return newCtrl;
    }

    void ConfigStringLineEditHandler::ConsumeAttribute(ConfigStringLineEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(debugName);

        GUI->blockSignals(true);
        if (attrib == AZ::Edit::Attributes::MaxLength)
        {
            int maxLen = -1;
            if (attrValue->Read<int>(maxLen))
            {
                GUI->setMaxLen(maxLen);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool isReadOnly = false;
            if (attrValue->Read<AZ::Crc32>(isReadOnly))
            {
                GUI->setEnabled(!isReadOnly);
            }
        }
        else if (attrib == Physics::MaterialConfiguration::s_stringGroup)
        {
            AZ::Crc32 uniqueGroup;
            if (attrValue->Read<AZ::Crc32>(uniqueGroup))
            {
                GUI->SetUniqueGroup(uniqueGroup);
            }
        }
        else if (attrib == Physics::MaterialConfiguration::s_forbiddenStringSet)
        {
            UniqueStringContainer::StringSet forbiddenStringsUnorderedSet;
            AZStd::set<AZStd::string> forbiddenStringsSet;
            AZStd::vector<AZStd::string> forbiddenStringsVector;

            if (attrValue->Read<UniqueStringContainer::StringSet>(forbiddenStringsUnorderedSet))
            {
                GUI->SetForbiddenStrings(forbiddenStringsUnorderedSet);
            }
            else if (attrValue->Read<AZStd::set<AZStd::string>>(forbiddenStringsSet))
            {
                forbiddenStringsUnorderedSet = UniqueStringContainer::StringSet(forbiddenStringsSet.begin()
                    , forbiddenStringsSet.end());
                GUI->SetForbiddenStrings(forbiddenStringsUnorderedSet);
            }
            else if (attrValue->Read<AZStd::vector<AZStd::string>>(forbiddenStringsVector))
            {
                forbiddenStringsUnorderedSet = UniqueStringContainer::StringSet(forbiddenStringsVector.begin()
                    , forbiddenStringsVector.end());
                GUI->SetForbiddenStrings(forbiddenStringsUnorderedSet);
            }
        }
        GUI->blockSignals(false);
    }

    void ConfigStringLineEditHandler::WriteGUIValuesIntoProperty(size_t index, ConfigStringLineEditCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZStd::string val = GUI->Value();
        instance = static_cast<property_t>(val);
    }

    bool ConfigStringLineEditHandler::ReadValuesIntoGUI(size_t index, ConfigStringLineEditCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZStd::string val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterConfigStringLineEditHandler()
    {
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew ConfigStringLineEditHandler());
    }
}

#include <Editor/moc_ConfigStringLineEditCtrl.cpp>
