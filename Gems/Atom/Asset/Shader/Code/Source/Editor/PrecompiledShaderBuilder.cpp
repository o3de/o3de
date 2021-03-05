/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PrecompiledShaderBuilder.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AtomCore/Serialization/Json/JsonUtils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Shader/PrecompiledShaderAssetSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <ShaderPlatformInterfaceRequest.h>

namespace AZ
{
    namespace
    {
        static const char* PrecompiledShaderBuilderName = "PrecompiledShaderBuilder";
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

        // setup dependencies on the azsrg asset file names
        for (const auto& srgFileName : precompiledShaderAsset.m_srgAssetFileNames)
        {
            AZStd::string srgAssetPath = RPI::AssetUtils::ResolvePathReference(request.m_sourceFile.c_str(), srgFileName);
            AssetBuilderSDK::SourceFileDependency dependency;
            dependency.m_sourceFileDependencyPath = srgAssetPath;
            response.m_sourceFileDependencyList.push_back(dependency);
        }

        // setup dependencies on the root azshadervariant asset file names
        for (const auto& rootShaderVariantAsset : precompiledShaderAsset.m_rootShaderVariantAssets)
        {
            AZStd::string rootShaderVariantAssetPath = RPI::AssetUtils::ResolvePathReference(request.m_sourceFile.c_str(), rootShaderVariantAsset->m_rootShaderVariantAssetFileName);
            AssetBuilderSDK::SourceFileDependency dependency;
            dependency.m_sourceFileDependencyPath = rootShaderVariantAssetPath;
            response.m_sourceFileDependencyList.push_back(dependency);
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = PrecompiledShaderBuilderJobKey;
            job.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            job.m_critical = true;

            response.m_createJobOutputs.push_back(job);
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

        AssetBuilderSDK::JobProduct jobProduct;

        // load Srg product assets
        // these are the dependency Srg asset products that were processed prior to running this job
        RPI::ShaderAssetCreator::ShaderResourceGroupAssets srgProductAssets;
        for (const auto& srgAssetFileName : precompiledShaderAsset.m_srgAssetFileNames)
        {
            auto assetOutcome = RPI::AssetUtils::LoadAsset<RPI::ShaderResourceGroupAsset>(request.m_fullPath, srgAssetFileName, 0);
            if (!assetOutcome)
            {
                AZ_Error(PrecompiledShaderBuilderName, false, "Failed to retrieve Srg asset for file [%s]", srgAssetFileName.c_str());
                return;
            }
            srgProductAssets.push_back(assetOutcome.GetValue());

            AssetBuilderSDK::ProductDependency productDependency;
            productDependency.m_dependencyId = assetOutcome.GetValue().GetId();
            productDependency.m_flags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad);
            jobProduct.m_dependencies.push_back(productDependency);
        }

        // retrieve the list of shader platform interfaces for the target platform
        AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
        ShaderBuilder::ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderBuilder::ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, request.m_platformInfo);

        // load the variant product assets
        // these are the dependency root variant asset products that were processed prior to running this job
        RPI::ShaderAssetCreator::ShaderRootVariantAssets rootVariantProductAssets;
        for (AZStd::unique_ptr<RPI::RootShaderVariantAssetSourceData>& rootShaderVariantAsset : precompiledShaderAsset.m_rootShaderVariantAssets)
        {
            // find the ShaderPlatformInterface for this root shader variant
            // we need this to map the variant asset to the APIType
            RHI::ShaderPlatformInterface* foundShaderPlatformInterface = nullptr;
            for (auto shaderPlatformInterface : platformInterfaces)
            {
                if (shaderPlatformInterface->GetAPIName() == rootShaderVariantAsset->m_apiName)
                {
                    foundShaderPlatformInterface = shaderPlatformInterface;
                    break;
                }
            }

            if (!foundShaderPlatformInterface)
            {
                // skip this shader API since it's not available on the current platform
                continue;
            }

            RHI::APIType apiType = foundShaderPlatformInterface->GetAPIType();

            // retrieve the variant asset
            auto assetOutcome = RPI::AssetUtils::LoadAsset<RPI::ShaderVariantAsset>(request.m_fullPath, rootShaderVariantAsset->m_rootShaderVariantAssetFileName, 0);
            if (!assetOutcome)
            {
                AZ_Error(PrecompiledShaderBuilderName, false, "Failed to retrieve Variant asset for file [%s]", rootShaderVariantAsset->m_rootShaderVariantAssetFileName.c_str());
                return;
            }

            rootVariantProductAssets.push_back(AZStd::make_pair(apiType, assetOutcome.GetValue()));

            AssetBuilderSDK::ProductDependency productDependency;
            productDependency.m_dependencyId = assetOutcome.GetValue().GetId();
            productDependency.m_flags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad);
            jobProduct.m_dependencies.push_back(productDependency);
        }

        if (rootVariantProductAssets.empty())
        {
            // no shader variants for this platform
            AZ_Warning(PrecompiledShaderBuilderName, false, "No shader platforms detected for root variants");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return;
        }

        // use the ShaderAssetCreator to clone the shader asset, which  will update the embedded Srg and Variant asset UUIDs
        // Note that the Srg and Variant assets do not have embedded asset references and are processed with the RC Copy functionality
        RPI::ShaderAssetCreator shaderAssetCreator;
        shaderAssetCreator.Clone(Uuid::CreateRandom(),
            shaderAsset,
            srgProductAssets,
            rootVariantProductAssets);

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

        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        return AZ::Utils::LoadObjectFromBuffer<T>(buffer.data(), length, context, loadFilter);
    }
} // namespace AZ
