/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <LmbrCentral_precompiled.h>
#include "LevelBuilderComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/API/ApplicationAPI.h>

namespace LevelBuilder
{
    void LevelBuilderComponent::Activate()
    {
        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        // No need to build level.pak files when using the prefab system.
        if (usePrefabSystemForLevels)
        {
            return;
        }

        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "LevelBuilderWorker";
        // This builder only works with the level.pak exported from levels. Other pak files are handled by the copy job.
        builderDescriptor.m_patterns.emplace_back(
            AssetBuilderSDK::AssetBuilderPattern(".*\\/level\\.pak$", AssetBuilderSDK::AssetBuilderPattern::PatternType::Regex));
        builderDescriptor.m_busId = azrtti_typeid<LevelBuilderWorker>();
        builderDescriptor.m_version = 8;
        builderDescriptor.m_createJobFunction =
            AZStd::bind(&LevelBuilderWorker::CreateJobs, &m_levelBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction =
            AZStd::bind(&LevelBuilderWorker::ProcessJob, &m_levelBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        
        m_levelBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void LevelBuilderComponent::Deactivate()
    {
        m_levelBuilder.BusDisconnect();
    }

    void LevelBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LevelBuilderComponent, AZ::Component>()
                ->Version(2)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }
}
