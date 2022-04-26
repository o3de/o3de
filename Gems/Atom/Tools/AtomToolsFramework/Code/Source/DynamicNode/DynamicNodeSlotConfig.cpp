/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AtomToolsFramework
{ 
    void DynamicNodeSlotConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNodeSlotConfig>()
                ->Version(0)
                ->Field("type", &DynamicNodeSlotConfig::m_type)
                ->Field("name", &DynamicNodeSlotConfig::m_name)
                ->Field("displayName", &DynamicNodeSlotConfig::m_displayName)
                ->Field("description", &DynamicNodeSlotConfig::m_description)
                ->Field("supportedDataTypes", &DynamicNodeSlotConfig::m_supportedDataTypes)
                ->Field("defaultValue", &DynamicNodeSlotConfig::m_defaultValue);
        }
    }

    DynamicNodeSlotConfig::DynamicNodeSlotConfig(
        const AZStd::string& type,
        const AZStd::string& name,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const AZStd::vector<AZStd::string>& supportedDataTypes,
        const AZStd::string& defaultValue)
        : m_type(type)
        , m_name(name)
        , m_displayName(displayName)
        , m_description(description)
        , m_supportedDataTypes(supportedDataTypes)
        , m_defaultValue(defaultValue)
    {
    }
} // namespace AtomToolsFramework
