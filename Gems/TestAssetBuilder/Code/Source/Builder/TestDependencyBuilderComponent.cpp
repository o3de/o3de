/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Builder/TestDependencyBuilderComponent.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace TestAssetBuilder
{
    namespace Details
    {
        AzFramework::GenericAssetHandler<TestAsset>* s_testAssetHandler = nullptr;

        void RegisterAssethandlers()
        {
            s_testAssetHandler =
                aznew AzFramework::GenericAssetHandler<TestAsset>("Automated Test Asset", "Other", "auto_test_asset");
            s_testAssetHandler->Register();
        }

        void UnregisterAssethandlers()
        {
            if (s_testAssetHandler)
            {
                s_testAssetHandler->Unregister();
                delete s_testAssetHandler;
                s_testAssetHandler = nullptr;
            }
        }
    } // namespace Details

    void TestAsset::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<TestAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("ReferencedAssets", &TestAsset::m_referencedAssets);
        }
    }

    void TestDependencyBuilderComponent::Init()
    {
    }

    void TestDependencyBuilderComponent::Activate()
    {
        Details::RegisterAssethandlers();

        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Test Dependency Builder";
        builderDescriptor.m_version = 1;
        builderDescriptor.m_patterns.emplace_back("*.auto_test_input", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDescriptor.m_busId = AZ::Uuid("{13D338AD-745F-442C-B0AA-48EFA6F3F044}");
        builderDescriptor.m_createJobFunction = AZStd::bind(
            &TestDependencyBuilderComponent::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(
            &TestDependencyBuilderComponent::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void TestDependencyBuilderComponent::Deactivate()
    {
        BusDisconnect();

        Details::UnregisterAssethandlers();
    }

    void TestDependencyBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        TestAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestDependencyBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void TestDependencyBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TestDependencyBuilderComponentPluginService"));
    }

    void TestDependencyBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TestDependencyBuilderComponentPluginService"));
    }

    void TestDependencyBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestDependencyBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    AZStd::string ReadFile(const char* path)
    {
        AZStd::string buffer;

        auto fileSize = AZ::IO::SystemFile::Length(path);

        if (fileSize > 0)
        {
            buffer.resize_no_construct(AZ::IO::SystemFile::Length(path));
            AZ::IO::SystemFile::Read(path, buffer.data(), buffer.size());
        }

        return buffer;
    }

    void TestDependencyBuilderComponent::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const auto& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor desc{ "", "Auto Test Builder", platform.m_identifier.c_str() };
            response.m_createJobOutputs.push_back(desc);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void TestDependencyBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        TestAsset outputAsset;
        AZStd::string buffer = ReadFile(request.m_fullPath.c_str());

        if (!buffer.empty())
        {
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(buffer, tokens, "|");

            for (const AZStd::string& path : tokens)
            {
                bool result = false;
                AZ::Data::AssetInfo assetInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    result,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                    path.c_str(),
                    assetInfo,
                    watchFolder);

                if (!result || !assetInfo.m_assetId.IsValid())
                {
                    AZ_Error("TestDependencyBuilderComponent", false, "GetSourceInfoBySourcePath failed for %s", path.c_str());
                    return;
                }

                // It's not technically correct to use the source AssetId as a product asset reference, however we know the output will have
                // a subId of 0 (the default) so we don't actually need that bit of data, we just need the UUID
                auto assetRef = AZ::Data::Asset<TestAsset>(assetInfo.m_assetId, azrtti_typeid<TestAsset>(), path);
                assetRef.SetAutoLoadBehavior(AZ::Data::PreLoad);
                outputAsset.m_referencedAssets.push_back(AZStd::move(assetRef));
            }
        }

        auto outputPath = AZ::IO::Path(request.m_tempDirPath) / request.m_sourceFile;
        outputPath.ReplaceExtension("auto_test_asset");

        AZ::JsonSerializationUtils::SaveObjectToFile(&outputAsset, outputPath.StringAsPosix());

        AssetBuilderSDK::JobProduct jobProduct;
        AssetBuilderSDK::OutputObject(&outputAsset, outputPath.FixedMaxPathStringAsPosix(), azrtti_typeid<TestAsset>(), 0, jobProduct);

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void TestDependencyBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

} // namespace TestAssetBuilder

