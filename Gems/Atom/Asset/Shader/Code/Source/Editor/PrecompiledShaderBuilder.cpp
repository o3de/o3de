/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrecompiledShaderBuilder.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Shader/PrecompiledShaderAssetSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <ShaderPlatformInterfaceRequest.h>
#include "ShaderBuilderUtility.h"

namespace AZ
{
    namespace
    {
        [[maybe_unused]] static const char* PrecompiledShaderBuilderName = "PrecompiledShaderBuilder";
        static const char* PrecompiledShaderBuilderJobKey = "PrecompiledShader Asset Builder";
        static const char* ShaderAssetExtension = "azshader";
    }

    void PrecompiledShaderBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void PrecompiledShaderBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, true);

        // load precompiled shader information file
        RPI::PrecompiledShaderAssetSourceData precompiledShaderAsset;
        auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<RPI::PrecompiledShaderAssetSourceData>(precompiledShaderAsset, fullPath);
        if (!loadResult.IsSuccess())
        {
            AZ_Error(PrecompiledShaderBuilderName, false, "Failed to load precompiled shader assets file [%s] error [%s]", fullPath.c_str(), loadResult.GetError().data());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AZStd::vector<AZStd::string>::iterator itPlatformIdentifier = AZStd::find(
                precompiledShaderAsset.m_platformIdentifiers.begin(),
                precompiledShaderAsset.m_platformIdentifiers.end(),
                platformInfo.m_identifier);

            if (itPlatformIdentifier != precompiledShaderAsset.m_platformIdentifiers.end())
            {
                // retrieve the shader APIs for this platform
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilder::ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(platformInfo);
                if (platformInterfaces.empty())
                {
                    continue;
                }

                AZStd::vector<AssetBuilderSDK::JobDependency> jobDependencyList;

                // setup dependencies on the root azshadervariant asset file names, for each supervariant
                for (const auto& supervariant : precompiledShaderAsset.m_supervariants)
                {
                    for (const auto& rootShaderVariantAsset : supervariant->m_rootShaderVariantAssets)
                    {
                        // find the API in the list of supported APIs on this platform
                        AZStd::vector<RHI::ShaderPlatformInterface*>::const_iterator itFoundAPI = AZStd::find_if(
                            platformInterfaces.begin(),
                            platformInterfaces.end(),
                            [&rootShaderVariantAsset](const RHI::ShaderPlatformInterface* shaderPlatformInterface)
                            {
                                return rootShaderVariantAsset->m_apiName == shaderPlatformInterface->GetAPIName();
                            });

                        if (itFoundAPI == platformInterfaces.end())
                        {
                            // the API is not supported on this platform, skip this entry
                            continue;
                        }

                        AZStd::string rootShaderVariantAssetPath = RPI::AssetUtils::ResolvePathReference(request.m_sourceFile.c_str(), rootShaderVariantAsset->m_rootShaderVariantAssetFileName);
                        AssetBuilderSDK::SourceFileDependency sourceDependency;
                        sourceDependency.m_sourceFileDependencyPath = rootShaderVariantAssetPath;
                        response.m_sourceFileDependencyList.push_back(sourceDependency);

                        AssetBuilderSDK::JobDependency jobDependency;
                        jobDependency.m_jobKey = "azshadervariant";
                        jobDependency.m_platformIdentifier = platformInfo.m_identifier;
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        jobDependency.m_sourceFile = sourceDependency;
                        jobDependencyList.push_back(jobDependency);
                    }
                }

                AssetBuilderSDK::JobDescriptor job;
                job.m_jobKey = PrecompiledShaderBuilderJobKey;
                job.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                job.m_jobDependencyList = jobDependencyList;
                job.m_critical = true;

                response.m_createJobOutputs.push_back(job);
            }
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void PrecompiledShaderBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        if (jobCancelListener.IsCancelled() || m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        SerializeContext* context = nullptr;
        ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Assert(false, "No serialize context");
            return;
        }

        // load precompiled shader information file
        RPI::PrecompiledShaderAssetSourceData precompiledShaderAsset;
        auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<RPI::PrecompiledShaderAssetSourceData>(precompiledShaderAsset, request.m_fullPath);
        if (!loadResult.IsSuccess())
        {
            AZ_Error(PrecompiledShaderBuilderName, false, "Failed to load precompiled shader assets file [%s] error [%s]", request.m_fullPath.c_str(), loadResult.GetError().c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // load shader source asset
        // this is the precompiled shader asset that we will be cloning and recreating from this builder
        AZStd::string fullShaderAssetPath = RPI::AssetUtils::ResolvePathReference(request.m_fullPath, precompiledShaderAsset.m_shaderAssetFileName);
        RPI::ShaderAsset* shaderAsset = LoadSourceAsset<RPI::ShaderAsset>(context, fullShaderAssetPath);
        if (!shaderAsset)
        {
            AZ_Error(PrecompiledShaderBuilderName, false, "Failed to retrieve shader asset for file [%s]", fullShaderAssetPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // retrieve the shader APIs for this platform
        AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilder::ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(request.m_platformInfo);
        if (platformInterfaces.empty())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct;

        // load the variant product assets, for each supervariant
        // these are the dependency root variant asset products that were processed prior to running this job
        RPI::ShaderAssetCreator::ShaderSupervariants supervariants;
        for (const auto& supervariant : precompiledShaderAsset.m_supervariants)
        {
            RPI::ShaderAssetCreator::ShaderRootVariantAssets rootVariantProductAssets;
            for (const auto& rootShaderVariantAsset : supervariant->m_rootShaderVariantAssets)
            {
                // find the API in the list of supported APIs on this platform
                AZStd::vector<RHI::ShaderPlatformInterface*>::const_iterator itFoundAPI = AZStd::find_if(
                    platformInterfaces.begin(),
                    platformInterfaces.end(),
                    [&rootShaderVariantAsset](const RHI::ShaderPlatformInterface* shaderPlatformInterface)
                    {
                        return rootShaderVariantAsset->m_apiName == shaderPlatformInterface->GetAPIName();
                    });

                if (itFoundAPI == platformInterfaces.end())
                {
                    // the API is not supported on this platform, skip this entry
                    continue;
                }

                // retrieve the variant asset
                auto assetOutcome = RPI::AssetUtils::LoadAsset<RPI::ShaderVariantAsset>(request.m_fullPath, rootShaderVariantAsset->m_rootShaderVariantAssetFileName, 0);
                if (!assetOutcome)
                {
                    AZ_Error(PrecompiledShaderBuilderName, false, "Failed to retrieve Variant asset for file [%s]", rootShaderVariantAsset->m_rootShaderVariantAssetFileName.c_str());
                    return;
                }

                rootVariantProductAssets.push_back(AZStd::make_pair(RHI::APIType{ rootShaderVariantAsset->m_apiName.GetCStr() }, assetOutcome.GetValue()));

                AssetBuilderSDK::ProductDependency productDependency;
                productDependency.m_dependencyId = assetOutcome.GetValue().GetId();
                productDependency.m_flags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad);
                jobProduct.m_dependencies.push_back(productDependency);
            }

            if (!rootVariantProductAssets.empty())
            {
                supervariants.push_back({ supervariant->m_name, rootVariantProductAssets });
            }
        }

        if (supervariants.empty())
        {
            // no applicable shader variants for this platform
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return;
        }

        // use the ShaderAssetCreator to clone the shader asset, which  will update the embedded Srg and Variant asset UUIDs
        // Note that the Srg and Variant assets do not have embedded asset references and are processed with the RC Copy functionality
        RPI::ShaderAssetCreator shaderAssetCreator;
        shaderAssetCreator.Clone(Uuid::CreateRandom(), *shaderAsset, supervariants, platformInterfaces);

        Data::Asset<RPI::ShaderAsset> outputShaderAsset;
        if (!shaderAssetCreator.End(outputShaderAsset))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // build the output product path
        AZStd::string destFileName;
        AZStd::string destPath;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), ShaderAssetExtension, destPath, true);

        // save the cloned shader file
        if (!Utils::SaveObjectToFile(destPath, DataStream::ST_BINARY, outputShaderAsset.Get()))
        {
            AZ_Error(PrecompiledShaderBuilderName, false, "Failed to output Shader Asset");
            return;
        }

        // setup the job product
        jobProduct.m_productFileName = destPath;
        jobProduct.m_productSubID = static_cast<uint32_t>(RPI::ShaderAssetSubId::ShaderAsset);
        jobProduct.m_productAssetType = azrtti_typeid<RPI::ShaderAsset>();
        jobProduct.m_dependenciesHandled = true;
        response.m_outputProducts.push_back(AZStd::move(jobProduct));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    template<typename T>
    T* PrecompiledShaderBuilder::LoadSourceAsset(SerializeContext* context, const AZStd::string& shaderAssetPath) const
    {
        AZStd::vector<char> buffer;

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(shaderAssetPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return nullptr;
        }

        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return nullptr;
        }

        buffer.resize_no_construct(length + 1);
        fileStream.Read(length, buffer.data());
        buffer.back() = 0;

        AZ::ObjectStream::FilterDescriptor loadFilter(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        return AZ::Utils::LoadObjectFromBuffer<T>(buffer.data(), length, context, loadFilter);
    }
} // namespace AZ
