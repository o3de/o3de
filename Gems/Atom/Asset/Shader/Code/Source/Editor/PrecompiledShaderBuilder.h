/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AZ
{
    class SerializeContext;

    //! AssetBuilder that takes precompiled azshader product file and produces an output
    //! products with the correct dependent asset GUIDs.
    class PrecompiledShaderBuilder final
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(PrecompiledShaderBuilder, "{50D3185B-489C-4C8E-84DC-F99A75FDB72F}");

        static constexpr const char* Extension = "precompiledshader";

        PrecompiledShaderBuilder() = default;
        ~PrecompiledShaderBuilder() = default;

        // Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        // AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        /// Register to builder and listen to builder command
        void RegisterBuilder();

    private:
        template<typename T>
        T* LoadSourceAsset(SerializeContext* context, const AZStd::string& shaderAssetPath) const;

        bool m_isShuttingDown = false;
    };
} // namespace AZ
