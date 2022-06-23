/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <DynamicNode/DynamicNodeSlotConfigSerializer.h>

namespace AtomToolsFramework
{ 
    void DynamicNodeSlotConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonDynamicNodeSlotConfigSerializer>()->HandlesType<DynamicNodeSlotConfig>();
        }

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNodeSlotConfig>()
                ->Version(0)
                ->Field("name", &DynamicNodeSlotConfig::m_name)
                ->Field("displayName", &DynamicNodeSlotConfig::m_displayName)
                ->Field("description", &DynamicNodeSlotConfig::m_description)
                ->Field("defaultValue", &DynamicNodeSlotConfig::m_defaultValue)
                ->Field("supportedDataTypes", &DynamicNodeSlotConfig::m_supportedDataTypes)
                ->Field("settings", &DynamicNodeSlotConfig::m_settings)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNodeSlotConfig>("DynamicNodeSlotConfig", "Configuration settings for individual slots on a dynamic node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_name, "Name", "Unique name used to identify individual slots on a node.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_displayName, "Display Name", "User friendly title of the slot that will appear on the node UI.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_description, "Description", "Detailed description of the node, its purpose, and behavior that will appear in tooltips and other UI.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_defaultValue, "Default Value", "The initial value of an input or property slot that has no incoming connection.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_supportedDataTypes, "Supported Data Types", "Container of names of data types that can be assigned to this slot. Output and property slots will be created using the first recognized data type in the container.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeSlotConfig::m_settings, "Settings", "Table of strings that can be used for any context specific or user defined data for each slot.")
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
} // namespace AtomToolsFramework
