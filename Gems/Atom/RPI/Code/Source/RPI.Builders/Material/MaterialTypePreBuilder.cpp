/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialTypePreBuilder.h"
//#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
//#include <Atom/RPI.Edit/Common/AssetUtils.h>
//#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
//#include <AzCore/Serialization/Json/JsonUtils.h>
//
//#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//
//#include <AzFramework/IO/LocalFileIO.h>
//#include <AzFramework/StringFunc/StringFunc.h>
//#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
//#include <AzToolsFramework/Debug/TraceContext.h>
//
//#include <AssetBuilderSDK/AssetBuilderSDK.h>
//#include <AssetBuilderSDK/SerializationDependencies.h>
//
//#include <AzCore/IO/IOUtils.h>
#include <AzCore/Utils/Utils.h>
//#include <AzCore/IO/Path/Path.h>
//#include <AzCore/Serialization/Json/JsonSerialization.h>
//#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialTypePreBuilderName[] = "MaterialTypePreBuilder";
        }

        const char* MaterialTypePreBuilder::JobKey = "Atom Material Type Pre-Builder";

        void MaterialTypePreBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = JobKey;
            builderDescriptor.m_version = 3; // First version
            builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtypeex", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            builderDescriptor.m_busId = azrtti_typeid<MaterialTypePreBuilder>();
            builderDescriptor.m_createJobFunction = AZStd::bind(&MaterialTypePreBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&MaterialTypePreBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);
        }

        MaterialTypePreBuilder::~MaterialTypePreBuilder()
        {
            BusDisconnect();
        }

        void MaterialTypePreBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& /*request*/, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = JobKey;
            outputJobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);
            response.m_createJobOutputs.push_back(outputJobDescriptor);
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void MaterialTypePreBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            if (m_isShuttingDown)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            AZ::IO::Path inputMaterialTypePath = request.m_fullPath;

            auto loadResult = MaterialUtils::LoadMaterialTypeSourceData(inputMaterialTypePath.c_str());

            if (!loadResult.IsSuccess())
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to load material type file.");
                return;
            }

            // Generate the required shaders

            auto azslFile = AZ::Utils::ReadFile<AZStd::string>("@gemroot:Atom@/TestData/TestData/Materials/Types/MinimalPBR_ForwardPass.azsl");

            auto shaderFile = AZ::Utils::ReadFile<AZStd::string>("@gemroot:Atom@/TestData/TestData/Materials/Types/MinimalPBR_ForwardPass.shader");

            if (!shaderFile.IsSuccess() || !azslFile.IsSuccess())
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to load shader files.");
                return;
            }

            AZ::u32 nextProductSubID = 0;

            // Intermediate azsl file

            AZ::IO::Path outputAzslFilePath = request.m_tempDirPath;
            outputAzslFilePath /= inputMaterialTypePath.Filename();
            outputAzslFilePath.ReplaceExtension("azsl");

            if (AZ::Utils::WriteFile(azslFile.GetValue(), outputAzslFilePath.c_str()).IsSuccess())
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_dependenciesHandled = true;
                product.m_productFileName = outputAzslFilePath.String();
                product.m_productSubID = nextProductSubID++;
                response.m_outputProducts.push_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate azsl file.");
                return;
            }

            // Intermediate shader file

            AzFramework::StringFunc::Replace(shaderFile.GetValue(), "./MinimalPBR_ForwardPass.azsl", AZ::IO::Path{outputAzslFilePath.Filename()}.c_str());

            AZ::IO::Path outputShaderFilePath = request.m_tempDirPath;
            outputShaderFilePath /= inputMaterialTypePath.Filename();
            outputShaderFilePath.ReplaceExtension("shader");

            if (AZ::Utils::WriteFile(shaderFile.GetValue(), outputShaderFilePath.c_str()).IsSuccess())
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_productFileName = outputShaderFilePath.String();
                product.m_productSubID = nextProductSubID++;
                response.m_outputProducts.push_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate shader file.");
                return;
            }

            // Intermediate material type

            MaterialTypeSourceData materialType = loadResult.TakeValue();

            materialType.m_shaderCollection.clear();

            materialType.m_shaderCollection.push_back({});
            materialType.m_shaderCollection.back().m_shaderFilePath = AZ::IO::Path{outputShaderFilePath.Filename()}.c_str();

            materialType.m_shaderCollection.push_back({});
            materialType.m_shaderCollection.back().m_shaderFilePath = "Shaders/Depth/DepthPass.shader";

            AZ::IO::Path outputMaterialTypeFilePath = request.m_tempDirPath;
            outputMaterialTypeFilePath /= inputMaterialTypePath.Filename();
            outputMaterialTypeFilePath.ReplaceExtension("materialtype");

            if (JsonUtils::SaveObjectToFile(outputMaterialTypeFilePath.String(), materialType))
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_productFileName = outputMaterialTypeFilePath.String();
                product.m_productSubID = nextProductSubID++;
                response.m_outputProducts.push_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate material type file.");
                return;
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialTypePreBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
