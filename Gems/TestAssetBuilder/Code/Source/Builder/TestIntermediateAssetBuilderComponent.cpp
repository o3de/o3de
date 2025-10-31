/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Builder/TestIntermediateAssetBuilderComponent.h>

namespace TestAssetBuilder
{
    void TestIntermediateAssetBuilderComponent::Init()
    {
    }

    void TestIntermediateAssetBuilderComponent::Activate()
    {
        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = "Test Intermediate Asset Builder Stage 1";
            builderDescriptor.m_version = 1;
            builderDescriptor.m_patterns.emplace_back("*.intersource", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
            builderDescriptor.m_busId = AZ::Uuid("{6A27C79F-28F0-44EA-B1CC-4A52DADB887D}");
            builderDescriptor.m_createJobFunction = AZStd::bind(
                &TestIntermediateAssetBuilderComponent::CreateJobsStage1, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(
                &TestIntermediateAssetBuilderComponent::ProcessJobStage1, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
        }

        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = "Test Intermediate Asset Builder Stage 2";
            builderDescriptor.m_version = 1;
            builderDescriptor.m_patterns.emplace_back("*.stage1output", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
            builderDescriptor.m_busId = AZ::Uuid("{1A1FB5D4-2F4A-434A-9D2C-9D51235C2C27}");
            builderDescriptor.m_createJobFunction =
                AZStd::bind(&TestIntermediateAssetBuilderComponent::CreateJobsStage2, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction =
                AZStd::bind(&TestIntermediateAssetBuilderComponent::ProcessJobStage2, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
        }

        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = "Test Intermediate Asset Builder Stage 3";
            builderDescriptor.m_version = 1;
            builderDescriptor.m_patterns.emplace_back("*.stage2output", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
            builderDescriptor.m_busId = AZ::Uuid("{BB935CEF-63EE-44D1-A8C5-DEF3DD799D49}");
            builderDescriptor.m_createJobFunction = AZStd::bind(
                &TestIntermediateAssetBuilderComponent::CreateJobsStage3, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(
                &TestIntermediateAssetBuilderComponent::ProcessJobStage3, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(
                &AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
        }
    }

    void TestIntermediateAssetBuilderComponent::Deactivate()
    {
        BusDisconnect();
    }

    void TestIntermediateAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestIntermediateAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void TestIntermediateAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TestIntermediateAssetBuilderPluginService"));
    }

    void TestIntermediateAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TestIntermediateAssetBuilderPluginService"));
    }

    void TestIntermediateAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestIntermediateAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TestIntermediateAssetBuilderComponent::CreateJobsStage1(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        auto commonPlatform = AZStd::find_if(request.m_enabledPlatforms.begin(), request.m_enabledPlatforms.end(), [](auto input) -> bool
        {
            return input.m_identifier == AssetBuilderSDK::CommonPlatformName;
        });

        if(commonPlatform != request.m_enabledPlatforms.end())
        {
            AZ_Error("TestIntermediateAssetBuilder", false, "Common platform was found in the list of enabled platforms."
                "  This is not expected as it will cause all builders to output files for the common platform.");
            return;
        }

        AssetBuilderSDK::JobDescriptor desc{"", "Test Product Stage 1", AssetBuilderSDK::CommonPlatformName};
        response.m_createJobOutputs.push_back(desc);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    bool CopyWithExtension(const AssetBuilderSDK::ProcessJobRequest& request, AZStd::string_view extension, AZ::IO::Path& outDestinationPath)
    {
        outDestinationPath = request.m_tempDirPath;
        outDestinationPath /= AZ::IO::PathView(request.m_fullPath).Filename();
        outDestinationPath.ReplaceExtension(extension);

        if (auto result = AZ::IO::FileIOBase::GetInstance()->Copy(request.m_fullPath.c_str(), outDestinationPath.c_str()); !result)
        {
            AZ_Error(
                "TestIntermediateAssetBuilder", false, "Failed to copy input file `%s` to temp output `%s", request.m_fullPath.c_str(),
                outDestinationPath.c_str());
            return false;
        }

        return true;
    }

    void TestIntermediateAssetBuilderComponent::ProcessJobStage1(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        // Check if we are cancelled or shutting down before doing intensive processing on this source file
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::IO::Path destinationPath;
        if(!CopyWithExtension(request, ".stage1output", destinationPath))
        {
            return;
        }

        const AZ::Uuid assetType("{978D26F9-D9F4-40E5-888B-3A53E2363BEA}");

        AssetBuilderSDK::JobProduct jobProduct(destinationPath.c_str(), assetType, 1);
        jobProduct.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void TestIntermediateAssetBuilderComponent::CreateJobsStage2(
        const AssetBuilderSDK::CreateJobsRequest&, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AssetBuilderSDK::JobDescriptor desc{ "", "Test Product Stage 2", AssetBuilderSDK::CommonPlatformName };
        response.m_createJobOutputs.push_back(desc);

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void TestIntermediateAssetBuilderComponent::ProcessJobStage2(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        const AZ::Uuid assetType("{CE426CC8-86AE-48EB-8D03-5E09DBBEAC94}");

        AZ::IO::Path destinationPath;
        if (!CopyWithExtension(request, ".stage2output", destinationPath))
        {
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct(destinationPath.AsPosix().c_str(), assetType, 1);
        jobProduct.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void TestIntermediateAssetBuilderComponent::CreateJobsStage3(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const auto& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor desc{ "", "Test Product Stage 3", platform.m_identifier.c_str() };
            response.m_createJobOutputs.push_back(desc);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void TestIntermediateAssetBuilderComponent::ProcessJobStage3(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        const AZ::Uuid assetType("{1CC1DD34-5675-4071-8732-3E3406664ADB}");

        AZ::IO::Path destinationPath;
        if (!CopyWithExtension(request, ".stage3output", destinationPath))
        {
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct(destinationPath.AsPosix().c_str(), assetType, 1);
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void TestIntermediateAssetBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

} // namespace TestAssetBuilder

