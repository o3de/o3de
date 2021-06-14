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

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        enum class DiffuseGlobalIlluminationQualityLevel : uint8_t
        {
            Low,
            Medium,
            High
        };

        //! This class provides general features and configuration for the diffuse global illumination environment,
        //! which consists of DiffuseProbeGrids and the diffuse Global IBL cubemap.
        class DiffuseGlobalIlluminationFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessorInterface, "{BD8CA35A-47C3-4FD8-932B-18495EF07527}");

            virtual void SetQualityLevel(DiffuseGlobalIlluminationQualityLevel qualityLevel) = 0;
        };
    } // namespace Render
} // namespace AZ
