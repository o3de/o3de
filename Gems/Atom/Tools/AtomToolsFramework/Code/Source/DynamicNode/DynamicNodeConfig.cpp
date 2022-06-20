/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Utils.h>

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
        return AZ::JsonSerializationUtils::SaveObjectToFile(this, ConvertAliasToPath(path)).IsSuccess();
    }

    bool DynamicNodeConfig::Load(const AZStd::string& path)
    {
        auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(ConvertAliasToPath(path));
        if (loadResult && loadResult.GetValue().is<DynamicNodeConfig>())
        {
            *this = AZStd::any_cast<DynamicNodeConfig>(loadResult.GetValue());
            return true;
        }
        return false;
    }
} // namespace AtomToolsFramework
