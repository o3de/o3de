/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <CommonFiles/CommonTypes.h>
#include <AzslData.h>

#include "AzslBuilder.h"

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }

    namespace ShaderBuilder
    {
        struct SrgData;
        struct TypeIdPair;

        class SrgLayoutBuilder
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            SrgLayoutBuilder();
            ~SrgLayoutBuilder();

            // An *.srgi file is nothing more than a regular azsl file that simply includes a set of srg/azsli
            // files, and in turn each one of those included files define "partial ShaderResourceGroup"s, which are
            // merged into a single ShaderResourceGroup by the shader compiler.
            // So, *.srgi are supposed to include files that only define "partial" SRGs.
            // And any file that defines a ShaderResourceGroup it should not be "partial" unless it is supposed
            // to be included by a *.srgi file.
            static constexpr const char* MergedPartialSrgsExtension = AzslBuilder::SrgIncludeExtension;
            static constexpr const char* SrgLayoutBuilderName = "SrgLayoutBuilder";
            static constexpr const char* SrgLayoutBuilderJobKey = "Shader Resource Group Layout";

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            //////////////////////////////////////////////////////////////////////////
            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;
            //////////////////////////////////////////////////////////////////////////

            void Activate();
            void Deactivate();

            static AZ::Uuid GetUUID();

            static void Reflect(AZ::ReflectContext* context);
        private:
            AZ_DISABLE_COPY_MOVE(SrgLayoutBuilder);

            static void CreateSRGAsset(
                AZStd::string fullSourcePath,
                const AssetBuilderSDK::ProcessJobRequest& request,
                AssetBuilderSDK::ProcessJobResponse& response,
                AssetBuilderSDK::JobCancelListener& jobCancelListener);

        };

    } // ShaderBuilder
} // AZ
