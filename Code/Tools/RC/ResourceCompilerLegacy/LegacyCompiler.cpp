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

#include "ResourceCompilerLegacy_precompiled.h"

#include "LegacyCompiler.h"
#include "LegacyAssetParser/ProductInfoCreator.h"
#include "Utilities.h"
#include "CryPath.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

namespace AZ
{
    namespace RC
    {
        LegacyCompiler::LegacyCompiler()
        {
        }

        void LegacyCompiler::Release()
        {
            delete this;
        }

        void LegacyCompiler::BeginProcessing(const IConfig* /*config*/)
        {
        }

        bool LegacyCompiler::Process()
        {
            // Register the AssetBuilderSDK structures needed later on.
            AzToolsFramework::ToolsApplication application;
            AZ::ComponentApplication::Descriptor descriptor;
            application.Start(descriptor);
            AssetBuilderSDK::InitializeSerializationContext();

            AssetBuilderSDK::ProcessJobResponse response;

            // Read ProcessJobRequest.xml from the output folder
            AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> request = ReadJobRequest(m_context.GetOutputFolder().c_str());
            if (!request)
            {
                return WriteResponse(m_context.GetOutputFolder().c_str(), response, false);
            }
            
            // Parse the source file and get product dependencies
            ProductInfoCreator productInfoCreator;
            response.m_outputProducts.push_back(productInfoCreator.GenerateProductInfo(request->m_sourceFile, request->m_fullPath));
            
            // Write ProcessJobResponse.xml to the output folder
            return WriteResponse(m_context.GetOutputFolder().c_str(), response, true);
        }

        void LegacyCompiler::EndProcessing()
        {
        }

        IConvertContext* LegacyCompiler::GetConvertContext()
        {
            return &m_context;
        }

        AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> LegacyCompiler::ReadJobRequest(const char* folder) const
        {
            AZStd::string requestFilePath;
            AzFramework::StringFunc::Path::ConstructFull(folder, AssetBuilderSDK::s_processJobRequestFileName, requestFilePath);
            AssetBuilderSDK::ProcessJobRequest* result = AZ::Utils::LoadObjectFromFile<AssetBuilderSDK::ProcessJobRequest>(requestFilePath);

            if (!result)
            {
                AZ_Error(LegacyCompilerUtils::TracePrint, false,"Unable to load ProcessJobRequest. Not enough information to process this file %s.\n", requestFilePath.c_str());
            }

            return AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest>(result);
        }

        bool LegacyCompiler::WriteResponse(const char* cacheFolder, AssetBuilderSDK::ProcessJobResponse& response, bool success) const
        {
            AZStd::string responseFilePath;
            AzFramework::StringFunc::Path::ConstructFull(cacheFolder, AssetBuilderSDK::s_processJobResponseFileName, responseFilePath);

            response.m_requiresSubIdGeneration = false;
            response.m_resultCode = success ? AssetBuilderSDK::ProcessJobResult_Success : AssetBuilderSDK::ProcessJobResult_Failed;

            bool result = AZ::Utils::SaveObjectToFile(responseFilePath, AZ::DataStream::StreamType::ST_XML, &response);

            if (!result)
            {
                AZ_Error(LegacyCompilerUtils::TracePrint, false, "Unable to save ProcessJobResponse file to %s.\n", responseFilePath.c_str());
            }

            return result && success;
        }
    } // namespace RC

} // namespace AZ
