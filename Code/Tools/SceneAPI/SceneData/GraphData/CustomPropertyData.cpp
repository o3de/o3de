/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::SceneData::GraphData
{
    CustomPropertyData::CustomPropertyData(PropertyMap propertyMap)
        : m_propertyMap(AZStd::move(propertyMap))
    {
    }

    void CustomPropertyData::SetPropertyMap(const PropertyMap& propertyMap)
    {
        m_propertyMap = propertyMap;
    }

    CustomPropertyData::PropertyMap& CustomPropertyData::GetPropertyMap()
    {
        return m_propertyMap;
    }

    const CustomPropertyData::PropertyMap& CustomPropertyData::GetPropertyMap() const
    {
        return m_propertyMap;
    }

    void CustomPropertyData::GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const
    {
        for (const auto& kvp : m_propertyMap)
        {
            if (kvp.second.is<AZStd::string>())
            {
                const auto* value = AZStd::any_cast<AZStd::string>(&kvp.second);
                output.Write(kvp.first.c_str(), value->c_str());
            }
            else if (kvp.second.is<bool>())
            {
                const auto* value = AZStd::any_cast<bool>(&kvp.second);
                output.Write(kvp.first.c_str(), *value);
            }
            else if (kvp.second.is<int32_t>())
            {
                const auto* value = AZStd::any_cast<int32_t>(&kvp.second);
                output.Write(kvp.first.c_str(), aznumeric_cast<int64_t>(*value));
            }
            else if (kvp.second.is<uint64_t>())
            {
                const auto* value = AZStd::any_cast<uint64_t>(&kvp.second);
                output.Write(kvp.first.c_str(), *value);
            }
            else if (kvp.second.is<float>())
            {
                const auto* value = AZStd::any_cast<float>(&kvp.second);
                output.Write(kvp.first.c_str(), *value);
            }
            else if (kvp.second.is<double>())
            {
                const auto* value = AZStd::any_cast<double>(&kvp.second);
                output.Write(kvp.first.c_str(), *value);
            }
        }
    }

    void CustomPropertyData::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CustomPropertyData>()->Version(1)
                ->Field("propertyMap", &CustomPropertyData::m_propertyMap);
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<SceneAPI::DataTypes::ICustomPropertyData>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "scene");

            behaviorContext->Class<CustomPropertyData>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "scene")
                ->Method("GetPropertyMap", [](CustomPropertyData& self) -> const CustomPropertyData::PropertyMap&
                {
                    return self.GetPropertyMap();
                });
        }
    }
}
