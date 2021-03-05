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

#pragma once

#include "ResourceCompilerLegacy_precompiled.h"
#include <IConvertor.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AssetBuilderSDK
{
    struct ProcessJobRequest;
    struct ProcessJobResponse;
}
namespace AZ
{
    namespace RC
    {
        class LegacyCompiler
            : public ICompiler
        {
        public:
            LegacyCompiler();

            void Release() override;

            void BeginProcessing(const IConfig* config) override;
            bool Process() override;
            void EndProcessing() override;

            IConvertContext* GetConvertContext() override;

        protected:

            AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> ReadJobRequest(const char* folder) const;
            bool WriteResponse(const char* folder, AssetBuilderSDK::ProcessJobResponse& response, bool success = true) const;

            ConvertContext m_context;
        };
    }
}
