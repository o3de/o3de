/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>

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
                ->Field("supportedDataTypes", &DynamicNodeSlotConfig::m_supportedDataTypes)
                ->Field("defaultValue", &DynamicNodeSlotConfig::m_defaultValue)
                ->Field("supportsEditingOnNode", &DynamicNodeSlotConfig::m_supportsEditingOnNode)
                ->Field("settings", &DynamicNodeSlotConfig::m_settings)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNodeSlotConfig>("DynamicNodeSlotConfig", "Configuration settings for individual slots on a dynamic node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->SetDynamicEditDataProvider(&DynamicNodeSlotConfig::GetDynamicEditData)
                    ->DataElement(AZ_CRC_CE("MultiLineString"), &DynamicNodeSlotConfig::m_name, "Name", "Unique name used to identify individual slots on a node.")
                    ->DataElement(AZ_CRC_CE("MultiLineString"), &DynamicNodeSlotConfig::m_displayName, "Display Name", "User friendly title of the slot that will appear on the node UI.")
                    ->DataElement(AZ_CRC_CE("MultiLineString"), &DynamicNodeSlotConfig::m_description, "Description", "Detailed description of the node, its purpose, and behavior that will appear in tooltips and other UI.")
                    ->DataElement(AZ_CRC_CE("MultiSelectStringVector"), &DynamicNodeSlotConfig::m_supportedDataTypes, "Data Types", "Container of names of data types that can be assigned to this slot. Output and property slots will be created using the first recognized data type in the container.")
                        ->Attribute(AZ_CRC_CE("MultiSelectOptions"), &GetRegisteredDataTypeNames)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::ClearDefaultValueIfInvalid)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, &DynamicNodeSlotConfig::ClearDefaultValueIfInvalid)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, &DynamicNodeSlotConfig::ClearDefaultValueIfInvalid)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, &DynamicNodeSlotConfig::ClearDefaultValueIfInvalid)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_defaultValue, "Default Value", "The initial value of an input or property slot that has no incoming connection.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Default Value")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_supportsEditingOnNode, "Display On Node", "Enable this to allow editing the the slot value directly in the node UI.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_settings, "Settings", "Table of strings that can be used for any context specific or user defined data for each slot.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->ElementAttribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Select Default Value")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Select Default Value")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::SelectDefaultValue)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Clear Default Value")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Clear Default Value")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicNodeSlotConfig::ClearDefaultValue)
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
                ->Property("defaultValue", BehaviorValueProperty(&DynamicNodeSlotConfig::m_defaultValue))
                ->Property("supportedDataTypes", BehaviorValueProperty(&DynamicNodeSlotConfig::m_supportedDataTypes))
                ->Property("supportsEditingOnNode", BehaviorValueProperty(&DynamicNodeSlotConfig::m_supportsEditingOnNode))
                ->Property("settings", BehaviorValueProperty(&DynamicNodeSlotConfig::m_settings))
                ;
        }
    }

    DynamicNodeSlotConfig::DynamicNodeSlotConfig(
        const AZStd::string& name,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZStd::any& defaultValue,
        const AZStd::vector<AZStd::string>& supportedDataTypes,
        const DynamicNodeSettingsMap& settings)
        : m_name(name)
        , m_displayName(displayName)
        , m_description(description)
        , m_defaultValue(defaultValue)
        , m_supportedDataTypes(supportedDataTypes)
        , m_settings(settings)
    {
    }

    AZ::Crc32 DynamicNodeSlotConfig::SelectDefaultValue()
    {
        AZStd::vector<AZStd::string> selections;
        if (GetStringListFromDialog(selections, m_supportedDataTypes, "Select Default Value", false))
        {
            m_defaultValue.clear();

            GraphModel::DataTypeList registeredDataTypes;
            DynamicNodeManagerRequestBus::BroadcastResult(
                registeredDataTypes, &DynamicNodeManagerRequestBus::Events::GetRegisteredDataTypes);

            for (const auto& dataType : registeredDataTypes)
            {
                for (const auto& selection : selections)
                {
                    if (dataType->GetDisplayName() == selection)
                    {
                        m_defaultValue = dataType->GetDefaultValue();
                        return AZ::Edit::PropertyRefreshLevels::EntireTree;
                    }
                }
            }

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::Crc32 DynamicNodeSlotConfig::ClearDefaultValue()
    {
        m_defaultValue.clear();
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 DynamicNodeSlotConfig::ClearDefaultValueIfInvalid()
    {
        GraphModel::DataTypeList registeredDataTypes;
        DynamicNodeManagerRequestBus::BroadcastResult(registeredDataTypes, &DynamicNodeManagerRequestBus::Events::GetRegisteredDataTypes);

        for (const auto& dataType : registeredDataTypes)
        {
            for (const auto& selection : m_supportedDataTypes)
            {
                if (dataType->GetDisplayName() == selection && dataType->GetTypeUuid() == m_defaultValue.type())
                {
                    return AZ::Edit::PropertyRefreshLevels::EntireTree;
                }
            }
        }

        ClearDefaultValue();
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZStd::vector<AZStd::string> DynamicNodeSlotConfig::GetSelectedDataTypesVec() const
    {
        return m_supportedDataTypes;
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
