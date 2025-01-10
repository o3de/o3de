/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeSlotConfig.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>

namespace AtomToolsFramework
{ 
    void DynamicNodeSlotConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNodeSlotConfig>()
                ->Version(0)
                ->Field("name", &DynamicNodeSlotConfig::m_name)
                ->Field("displayName", &DynamicNodeSlotConfig::m_displayName)
                ->Field("description", &DynamicNodeSlotConfig::m_description)
                ->Field("supportedDataTypeRegex", &DynamicNodeSlotConfig::m_supportedDataTypeRegex)
                ->Field("defaultDataType", &DynamicNodeSlotConfig::m_defaultDataType)
                ->Field("defaultValue", &DynamicNodeSlotConfig::m_defaultValue)
                ->Field("enumValues", &DynamicNodeSlotConfig::m_enumValues)
                ->Field("visibleOnNode", &DynamicNodeSlotConfig::m_visibleOnNode)
                ->Field("editableOnNode", &DynamicNodeSlotConfig::m_editableOnNode)
                ->Field("allowNameSubstitution", &DynamicNodeSlotConfig::m_allowNameSubstitution)
                ->Field("settings", &DynamicNodeSlotConfig::m_settings)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNodeSlotConfig>("DynamicNodeSlotConfig", "Configuration settings for individual slots on a dynamic node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &DynamicNodeSlotConfig::GetDisplayNameForEditor)
                    ->SetDynamicEditDataProvider(&DynamicNodeSlotConfig::GetDynamicEditData)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Add new settings groups from settings registered with this tool.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::AddRegisteredSettingGroups)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Add Setting Groups")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeSlotConfig::m_name, "Name", "Unique name used to identify individual slots on a node.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeSlotConfig::m_displayName, "Display Name", "User friendly title of the slot that will appear on the node UI.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeSlotConfig::m_description, "Description", "Detailed description of the node, its purpose, and behavior that will appear in tooltips and other UI.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeSlotConfig::m_supportedDataTypeRegex, "Supported Data Type Regex", "Regular expression to search for data types compatible with this slot.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::ValidateDataTypes)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, &DynamicNodeSlotConfig::ValidateDataTypes)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DynamicNodeSlotConfig::m_defaultDataType, "Default Data Type", "Name of the default data type for this slot. If this is not specified the default data type will fall back to the first supported data type.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &DynamicNodeSlotConfig::GetSupportedDataTypeNames)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::ValidateDataTypes)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, &DynamicNodeSlotConfig::ValidateDataTypes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_defaultValue, "Default Value", "The initial value of an input or property slot that has no incoming connection.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Default Value")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_enumValues, "Enum Values", "List of potential values if the data type is a string.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_visibleOnNode, "Visible On Node", "Enable this for the slot to appear on the node UI in the graph view.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_editableOnNode, "Editable On Node", "Enable this for the slot value to be editable on the node UI in the graph view.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_allowNameSubstitution, "Allow Name Substitution", "Hint on whether or not the slot name can be substituted or mangled in applicable systems.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_settings, "Settings", "Table of strings that can be used for any context specific or user defined data for each slot.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->ElementAttribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DynamicNodeSlotConfig>("DynamicNodeSlotConfig")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Constructor()
                ->Constructor<const DynamicNodeSlotConfig&>()
                ->Property("name", BehaviorValueProperty(&DynamicNodeSlotConfig::m_name))
                ->Property("displayName", BehaviorValueProperty(&DynamicNodeSlotConfig::m_displayName))
                ->Property("defaultValue", BehaviorValueProperty(&DynamicNodeSlotConfig::m_defaultValue))
                ->Property("enumValues", BehaviorValueProperty(&DynamicNodeSlotConfig::m_enumValues))
                ->Property("supportedDataTypeRegex", BehaviorValueProperty(&DynamicNodeSlotConfig::m_supportedDataTypeRegex))
                ->Property("defaultDataType", BehaviorValueProperty(&DynamicNodeSlotConfig::m_defaultDataType))
                ->Property("visibleOnNode", BehaviorValueProperty(&DynamicNodeSlotConfig::m_visibleOnNode))
                ->Property("editableOnNode", BehaviorValueProperty(&DynamicNodeSlotConfig::m_editableOnNode))
                ->Property("allowNameSubstitution", BehaviorValueProperty(&DynamicNodeSlotConfig::m_allowNameSubstitution))
                ->Property("settings", BehaviorValueProperty(&DynamicNodeSlotConfig::m_settings))
                ;
        }
    }

    DynamicNodeSlotConfig::DynamicNodeSlotConfig(
        const AZStd::string& name,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZStd::any& defaultValue,
        const AZStd::string& supportedDataTypeRegex,
        const DynamicNodeSettingsMap& settings)
        : m_name(name)
        , m_displayName(displayName)
        , m_description(description)
        , m_defaultValue(defaultValue)
        , m_supportedDataTypeRegex(supportedDataTypeRegex)
        , m_settings(settings)
    {
    }

    AZ::Crc32 DynamicNodeSlotConfig::ValidateDataTypes()
    {
        const auto& supportedDataTypes = GetSupportedDataTypes();
        if (supportedDataTypes.empty())
        {
            const bool hadDefaultValue = !m_defaultValue.empty();
            m_defaultDataType.clear();
            m_defaultValue.clear();
            return hadDefaultValue ? AZ::Edit::PropertyRefreshLevels::EntireTree : AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        // Find the registered data types corresponding to the selected default value and default data type.
        auto defaultValueItr = AZStd::find_if(supportedDataTypes.begin(), supportedDataTypes.end(), [&](const auto& dataType) {
            return dataType->IsSupportedValue(m_defaultValue);
        });
        auto defaultTypeItr = AZStd::find_if(supportedDataTypes.begin(), supportedDataTypes.end(), [&](const auto& dataType) {
            return dataType->GetDisplayName() == m_defaultDataType;
        });

        // If the default data type is not set or does not match any of the supported types then assign it to the same type as the default
        // value or the first registered data type.
        if (defaultTypeItr == supportedDataTypes.end())
        {
            if (defaultValueItr != supportedDataTypes.end())
            {
                m_defaultDataType = (*defaultValueItr)->GetDisplayName();
                defaultTypeItr = defaultValueItr;
            }
            else
            {
                m_defaultDataType = supportedDataTypes.front()->GetDisplayName();
                defaultTypeItr = supportedDataTypes.begin();
            }
        }

        // Finally, if the default value does not match the default data type then reset it.
        if (defaultTypeItr != defaultValueItr)
        {
            m_defaultValue = (*defaultTypeItr)->GetDefaultValue();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::any DynamicNodeSlotConfig::GetDefaultValue() const
    {
        const auto& supportedDataTypes = GetSupportedDataTypes();
        for (const auto& dataType : supportedDataTypes)
        {
            if (dataType->IsSupportedValue(m_defaultValue))
            {
                return m_defaultValue;
            }
        }
        return !supportedDataTypes.empty() ? supportedDataTypes.front()->GetDefaultValue() : AZStd::any();
    }

    AZStd::string DynamicNodeSlotConfig::GetDefaultDataTypeName() const
    {
        const auto& dataType = GetDefaultDataType();
        return dataType ? dataType->GetDisplayName() : AZStd::string();
    }

    GraphModel::DataTypePtr DynamicNodeSlotConfig::GetDefaultDataType() const
    {
        const auto& supportedDataTypes = GetSupportedDataTypes();
        for (const auto& dataType : supportedDataTypes)
        {
            if (dataType->GetDisplayName() == m_defaultDataType)
            {
                return dataType;
            }
        }
        return !supportedDataTypes.empty() ? supportedDataTypes.front() : GraphModel::DataTypePtr();
    }

    AZStd::vector<AZStd::string> DynamicNodeSlotConfig::GetSupportedDataTypeNames() const
    {
        AZStd::vector<AZStd::string> dataTypeNames;
        for (const auto& dataType : GetSupportedDataTypes())
        {
            dataTypeNames.push_back(dataType->GetDisplayName());
        }
        return dataTypeNames;
    }

    GraphModel::DataTypeList DynamicNodeSlotConfig::GetSupportedDataTypes() const
    {
        GraphModel::DataTypeList supportedDataTypes;
        if (!m_supportedDataTypeRegex.empty())
        {
            DynamicNodeManagerRequestBus::BroadcastResult(supportedDataTypes, &DynamicNodeManagerRequestBus::Events::GetRegisteredDataTypes);
            AZStd::regex supportedDataTypeRegex(m_supportedDataTypeRegex, AZStd::regex::flag_type::icase);
            AZStd::erase_if(supportedDataTypes, [&](const auto& dataType) { return !AZStd::regex_match(dataType->GetDisplayName(), supportedDataTypeRegex); });
        }
        return supportedDataTypes;
    }

    AZStd::string DynamicNodeSlotConfig::GetDisplayNameForEditor() const
    {
        return AZStd::string::format("Slot (%s)", !m_name.empty() ? m_name.c_str() : "unnamed");
    }

    void DynamicNodeSlotConfig::AutoFillMissingData()
    {
        if (m_displayName.empty())
        {
            m_displayName = GetDisplayNameFromText(m_name);
        }

        if (m_description.empty())
        {
            m_description = m_displayName;
        }
    }

    AZ::Crc32 DynamicNodeSlotConfig::AddRegisteredSettingGroups()
    {
        if (AddRegisteredSettingGroupsToMap(m_settings))
        {
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    const AZ::Edit::ElementData* DynamicNodeSlotConfig::GetDynamicEditData(
        const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType)
    {
        const DynamicNodeSlotConfig* owner = reinterpret_cast<const DynamicNodeSlotConfig*>(handlerPtr);
        if (elementType == azrtti_typeid<AZStd::string>())
        {
            return FindDynamicEditDataForSetting(owner->m_settings, elementPtr);
        }
        return nullptr;
    }
} // namespace AtomToolsFramework
