/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <CommonFiles/Preprocessor.h>
#include <AzslCompiler.h>
#include "AtomShaderConfig.h"
#include "ShaderBuilderUtility.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        static constexpr char AzslBuilderName[] = "AzslBuilder";

        //! This builder is the forerunner of the shader build pipeline.
        //! It will perform the first 3 transformations on shader files (.shader) and AZSL-containing files (.azsl/.azsli/.srgi)
        //! Which are: [prepend common header -> preprocess -> transpile AZSL to HLSL & reflect all shader program properties of interest into json files]
        //! The next builders in the chain: SRG/Shader/Variant, will consume the output products of this builder, without having to re-run Azslc nor Mcpp
        //! Note that the output products are not traditional product assets that will be used by the game project.
        //! They are artifacts that are produced once, cached, and used later by other AssetBuilders as a way to centralize build organization.
        class AzslBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:

            static constexpr const char* BuilderName = AzslBuilderName;
            static constexpr char JobKey[] = "AZSL Build";
            static constexpr char SrgIncludeExtension[] = "srgi";

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            //////////////////////////////////////////////////////////////////////////
            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override {};
            //////////////////////////////////////////////////////////////////////////

            static AZ::Uuid GetUUID();
        };

        //! Helper for dependent builders
        void AddAzslBuilderJobDependency(AssetBuilderSDK::JobDescriptor& jobDescriptor, const AZStd::string& platformInfoIdentifier, AZStd::string_view apiName, AZStd::string_view fullFilePath);
    } // ShaderBuilder
} // AZ
