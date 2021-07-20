/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "LuaBuilderComponent.h"

#include <AzCore/Script/ScriptAsset.h>
#include "AzCore/Serialization/EditContextConstants.inl"

void LuaBuilder::BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<BuilderPluginComponent, AZ::Component>()
            ->Version(2)
            ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));;
    }
}

void LuaBuilder::BuilderPluginComponent::Activate()
{
    AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
    builderDescriptor.m_name = "Lua Worker Builder";
    builderDescriptor.m_version = 6;
            builderDescriptor.m_analysisFingerprint = AZStd::string::format("%d", static_cast<AZ::u8>(AZ::ScriptAsset::AssetVersion));
    builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.lua", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
    builderDescriptor.m_busId = azrtti_typeid<LuaBuilderWorker>();
    builderDescriptor.m_createJobFunction = AZStd::bind(&LuaBuilderWorker::CreateJobs, &m_luaBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
    builderDescriptor.m_processJobFunction = AZStd::bind(&LuaBuilderWorker::ProcessJob, &m_luaBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
    m_luaBuilder.BusConnect(builderDescriptor.m_busId);

    // (optimization) this builder does not emit source dependencies:
    builderDescriptor.m_flags |= AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;

    AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
}

void LuaBuilder::BuilderPluginComponent::Deactivate()
{
    m_luaBuilder.BusDisconnect();
}
