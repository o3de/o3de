/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>
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
                ->Field("id", &DynamicNodeConfig::m_id)
                ->Field("category", &DynamicNodeConfig::m_category)
                ->Field("title", &DynamicNodeConfig::m_title)
                ->Field("subTitle", &DynamicNodeConfig::m_subTitle)
                ->Field("titlePaletteName", &DynamicNodeConfig::m_titlePaletteName)
                ->Field("slotDataTypeGroups", &DynamicNodeConfig::m_slotDataTypeGroups)
                ->Field("settings", &DynamicNodeConfig::m_settings)
                ->Field("propertySlots", &DynamicNodeConfig::m_propertySlots)
                ->Field("inputSlots", &DynamicNodeConfig::m_inputSlots)
                ->Field("outputSlots", &DynamicNodeConfig::m_outputSlots)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNodeConfig>("DynamicNodeConfig", "Configuration settings defining the slots and UI of a dynamic node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->SetDynamicEditDataProvider(&DynamicNodeConfig::GetDynamicEditData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_id, "Id", "UUID for identifying this node configuration regardless of file location.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeConfig::m_category, "Category", "Name of the category where this node will appear in the node palette.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeConfig::m_title, "Title", "Title that will appear at the top of the node UI in a graph.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeConfig::m_subTitle, "Sub Title", "Secondary title that will appear below the main title on the node UI in a graph.")
                    ->DataElement(AZ_CRC_CE("MultilineStringDialog"), &DynamicNodeConfig::m_titlePaletteName, "Title Palette Name", "Name of the node title bar UI palette style sheet entry.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_slotDataTypeGroups, "Slot Data Type Groups", "Groups of slots that should have the same types.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("MultiStringSelectDelimited"))
                        ->ElementAttribute(AZ_CRC_CE("Options"), &DynamicNodeConfig::GetSlotNames)
                        ->ElementAttribute(AZ_CRC_CE("DelimitersForJoin"), "|")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_settings, "Settings", "Table of strings that can be used for any context specific or user defined data for each node.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->ElementAttribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ElementAttribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_inputSlots, "Input Slots", "Container of dynamic node input slot configurations.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_outputSlots, "Output Slots", "Container of dynamic node output slot configurations.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicNodeConfig::m_propertySlots, "Property Slots", "Container hub dynamic node property slot configurations.")
                         ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
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
                ->Property("id", BehaviorValueProperty(&DynamicNodeConfig::m_id))
                ->Property("category", BehaviorValueProperty(&DynamicNodeConfig::m_category))
                ->Property("title", BehaviorValueProperty(&DynamicNodeConfig::m_title))
                ->Property("subTitle", BehaviorValueProperty(&DynamicNodeConfig::m_subTitle))
                ->Property("titlePaletteName", BehaviorValueProperty(&DynamicNodeConfig::m_titlePaletteName))
                ->Property("slotDataTypeGroups", BehaviorValueProperty(&DynamicNodeConfig::m_slotDataTypeGroups))
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
        auto saveResult = AZ::JsonSerializationUtils::SaveObjectToFile(this, GetPathWithoutAlias(path));
        if (!saveResult)
        {
            AZ_Error("DynamicNodeConfig", false, "Failed to save (%s). %s", path.c_str(), saveResult.GetError().c_str());
            return false;
        }
        return true;
    }

    bool DynamicNodeConfig::Load(const AZStd::string& path)
    {
        auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(GetPathWithoutAlias(path));
        if (!loadResult)
        {
            AZ_Error("DynamicNodeConfig", false, "Failed to load (%s). %s", path.c_str(), loadResult.GetError().c_str());
            return false;
        }

        if (!loadResult.GetValue().is<DynamicNodeConfig>())
        {
            AZ_Error("DynamicNodeConfig", false, "Failed to load (%s). Doesn't contain correct type", path.c_str());
            return false;
        }

        *this = AZStd::any_cast<DynamicNodeConfig>(loadResult.GetValue());

        ValidateSlots();
        return true;
    }

    void DynamicNodeConfig::ValidateSlots()
    {
        VisitDynamicNodeSlotConfigs(
            *this,
            [](DynamicNodeSlotConfig& slotConfig)
            {
                slotConfig.ValidateDataTypes();
            });
    }

    AZStd::vector<AZStd::string> DynamicNodeConfig::GetSlotNames() const
    {
        AZStd::vector<AZStd::string> slotNames;
        VisitDynamicNodeSlotConfigs(
            *this,
            [&slotNames](const DynamicNodeSlotConfig& slotConfig)
            {
                slotNames.push_back(slotConfig.m_name);
            });
        return slotNames;
    }

    const AZ::Edit::ElementData* DynamicNodeConfig::GetDynamicEditData(
        const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType)
    {
        const DynamicNodeConfig* owner = reinterpret_cast<const DynamicNodeConfig*>(handlerPtr);
        if (elementType == azrtti_typeid<AZStd::string>())
        {
            return FindDynamicEditDataForSetting(owner->m_settings, elementPtr);
        }
        return nullptr;
    }
} // namespace AtomToolsFramework
