/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <OpenXRVk/OpenXRVkInteractionProfilesAsset.h>
#include <OpenXRVk/OpenXRVkActionSetsAsset.h>

#include "OpenXRVkAssetsBuilderSystemComponent.h"

namespace OpenXRVkBuilders
{
    void OpenXRAssetsBuilderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OpenXRAssetsBuilderSystemComponent, Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
        }
    }
    
    void OpenXRAssetsBuilderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("OpenXRAssetsBuilderService"));
    }
    
    void OpenXRAssetsBuilderSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("OpenXRAssetsBuilderService"));
    }
    
    void OpenXRAssetsBuilderSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }
    
    void OpenXRAssetsBuilderSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AssetCatalogService"));
    }
    
    void OpenXRAssetsBuilderSystemComponent::Init()
    {
    }
    
    void OpenXRAssetsBuilderSystemComponent::Activate()
    {    
        // Register Shader Asset Builder
        AssetBuilderSDK::AssetBuilderDesc assetBuilderDescriptor;
        assetBuilderDescriptor.m_name = "OpenXR Assets Builder";
        assetBuilderDescriptor.m_version = 1; // First versuib
        assetBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(AZStd::string::format("*.%s", OpenXRVk::OpenXRInteractionProfilesAsset::s_assetExtension), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        assetBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(AZStd::string::format("*.%s", OpenXRVk::OpenXRActionSetsAsset::s_assetExtension), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        assetBuilderDescriptor.m_busId = azrtti_typeid<OpenXRAssetsBuilder>();
        assetBuilderDescriptor.m_createJobFunction = AZStd::bind(&OpenXRAssetsBuilder::CreateJobs, &m_actionSetsAssetBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        assetBuilderDescriptor.m_processJobFunction = AZStd::bind(&OpenXRAssetsBuilder::ProcessJob, &m_actionSetsAssetBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
    
        m_actionSetsAssetBuilder.BusConnect(assetBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, assetBuilderDescriptor);
    }
    
    void OpenXRAssetsBuilderSystemComponent::Deactivate()
    {
        m_actionSetsAssetBuilder.BusDisconnect();
    }

} // namespace AZ
