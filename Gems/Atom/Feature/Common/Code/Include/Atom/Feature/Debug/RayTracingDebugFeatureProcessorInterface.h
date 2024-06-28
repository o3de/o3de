/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugSettingsInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class RayTracingDebugFeatureProcessorInterface : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(RayTracingDebugFeatureProcessorInterface, "{873F5D8A-5A0D-41F3-BADF-57394F8542D8}", RPI::FeatureProcessor);

        virtual RayTracingDebugSettingsInterface* GetSettingsInterface() = 0;
        virtual void OnRayTracingDebugComponentAdded() = 0;
        virtual void OnRayTracingDebugComponentRemoved() = 0;
    };
} // namespace AZ::Render
