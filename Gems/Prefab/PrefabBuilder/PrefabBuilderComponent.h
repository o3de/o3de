/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Component/Component.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>
#include <Fingerprinting/TypeFingerprinter.h>
#include <Prefab/PrefabDomUtils.h>

namespace AZ::Prefab
{
    class PrefabBuilderComponent
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        , public AZ::Component
    {
    public:
        static constexpr const char* BuilderId = "{A2E0791C-4607-4363-A7FD-73D01ED49660}";
        static constexpr const char* PrefabJobKey = "Prefabs";

        AZ_COMPONENT(PrefabBuilderComponent, BuilderId);
        static void Reflect(AZ::ReflectContext* context);
        
        PrefabBuilderComponent()
            : m_builderId(BuilderId)
        {
            
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        void ShutDown() override;
        AzToolsFramework::Fingerprinting::TypeFingerprint CalculateBuilderFingerprint() const;
        AzToolsFramework::Fingerprinting::TypeFingerprint CalculatePrefabFingerprint(
            const AzToolsFramework::Prefab::PrefabDom& genericDocument) const;
        static AZStd::vector<AssetBuilderSDK::SourceFileDependency> GetSourceDependencies(
            const AzToolsFramework::Prefab::PrefabDom& genericDocument);
        bool ProcessPrefab(
            const AZ::PlatformTagSet& platformTags, const char* filePath, AZ::IO::PathView tempDirPath, const AZ::Uuid& sourceFileUuid,
            AzToolsFramework::Prefab::PrefabDom& mutableRootDom,
            AZStd::vector<AssetBuilderSDK::JobProduct>& jobProducts);

    protected:
        virtual AZStd::unique_ptr<AZ::IO::GenericStream> GetOutputStream(const AZ::IO::Path& path) const;

    private:
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;

        bool StoreProducts(
            AZ::IO::PathView tempDirPath,
            const AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext::ProcessedObjectStoreContainer& store,
            const AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext::ProductAssetDependencyContainer& registeredDependencies,
            AZStd::vector<AssetBuilderSDK::JobProduct>& outputProducts) const;
        
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabConversionPipeline m_pipeline;
        AZ::Uuid m_builderId;
        AZStd::atomic_bool m_isShuttingDown{ false };
        AZStd::unique_ptr<AzToolsFramework::Fingerprinting::TypeFingerprinter> m_typeFingerprinter;
    };
    
} // namespace AZ::Prefab
