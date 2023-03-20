/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            High,

            Count
        };

        //! This class provides general features and configuration for the diffuse global illumination environment,
        //! which consists of DiffuseProbeGrids and the diffuse Global IBL cubemap.
        class DiffuseGlobalIlluminationFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseGlobalIlluminationFeatureProcessorInterface, "{BD8CA35A-47C3-4FD8-932B-18495EF07527}", AZ::RPI::FeatureProcessor);

            virtual void SetQualityLevel(DiffuseGlobalIlluminationQualityLevel qualityLevel) = 0;
        };
    } // namespace Render
} // namespace AZ
