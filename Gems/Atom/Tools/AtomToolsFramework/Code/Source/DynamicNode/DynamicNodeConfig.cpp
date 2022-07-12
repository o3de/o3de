/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AtomToolsFramework
{
    void DynamicNodeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNodeConfig>()
                ->Version(0)
                ->Field("category", &DynamicNodeConfig::m_category)
                ->Field("title", &DynamicNodeConfig::m_title)
                ->Field("subTitle", &DynamicNodeConfig::m_subTitle)
                ->Field("settings", &DynamicNodeConfig::m_settings)
                ->Field("inputSlots", &DynamicNodeConfig::m_inputSlots)
                ->Field("outputSlots", &DynamicNodeConfig::m_outputSlots)
                ->Field("propertySlots", &DynamicNodeConfig::m_propertySlots)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNodeConfig>("DynamicNodeConfig", "Configuration settings defining the slots and UI of a dynamic node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_category, "Category", "Name of the category where this node will appear in the node palette.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_title, "Title", "Title that will appear at the top of the node UI in a graph.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_subTitle, "Sub Title", "Secondary title that will appear below the main title on the node UI in a graph.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_settings, "Settings", "Table of strings that can be used for any context specific or user defined data for each node.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_inputSlots, "Input Slots", "Container of dynamic node input slot configurations.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_outputSlots, "Output Slots", "Container of dynamic node output slot configurations.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_propertySlots, "Property Slots", "Container hub dynamic node property slot configurations.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DynamicNodeConfig>("DynamicNodeConfig")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Constructor()
                ->Constructor<const DynamicNodeConfig&>()
                ->Property("category", BehaviorValueProperty(&DynamicNodeConfig::m_category))
                ->Property("title", BehaviorValueProperty(&DynamicNodeConfig::m_title))
                ->Property("subTitle", BehaviorValueProperty(&DynamicNodeConfig::m_subTitle))
                ->Property("settings", BehaviorValueProperty(&DynamicNodeConfig::m_settings))
                ->Property("inputSlots", BehaviorValueProperty(&DynamicNodeConfig::m_inputSlots))
                ->Property("outputSlots", BehaviorValueProperty(&DynamicNodeConfig::m_outputSlots))
                ->Property("propertySlots", BehaviorValueProperty(&DynamicNodeConfig::m_propertySlots))
                ;
        }
    }

    DynamicNodeConfig::DynamicNodeConfig(
        const AZStd::string& category,
        const AZStd::string& title,
        const AZStd::string& subTitle,
        const DynamicNodeSettingsMap& settings,
        const AZStd::vector<DynamicNodeSlotConfig>& inputSlots,
        const AZStd::vector<DynamicNodeSlotConfig>& outputSlots,
        const AZStd::vector<DynamicNodeSlotConfig>& propertySlots)
        : m_category(category)
        , m_title(title)
        , m_subTitle(subTitle)
        , m_settings(settings)
        , m_inputSlots(inputSlots)
        , m_outputSlots(outputSlots)
        , m_propertySlots(propertySlots)
    {
    }

    bool DynamicNodeConfig::Save(const AZStd::string& path) const
    {
        return AZ::JsonSerializationUtils::SaveObjectToFile(this, GetPathWithoutAlias(path)).IsSuccess();
    }

    bool DynamicNodeConfig::Load(const AZStd::string& path)
    {
        auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(GetPathWithoutAlias(path));
        if (loadResult && loadResult.GetValue().is<DynamicNodeConfig>())
        {
            *this = AZStd::any_cast<DynamicNodeConfig>(loadResult.GetValue());
            return true;
        }
        return false;
    }
} // namespace AtomToolsFramework
