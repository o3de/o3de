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
    //! Interface for the RayTracingDebugFeatureProcessor, which handles debug ray tracing settings for the scene, such as displaying only
    //! InstanceID, PrimitiveIndex, barycentric coordinates, normals, etc.
    class RayTracingDebugFeatureProcessorInterface : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(RayTracingDebugFeatureProcessorInterface, "{873F5D8A-5A0D-41F3-BADF-57394F8542D8}", RPI::FeatureProcessor);

        //! Retrieves existing settings. If none found, returns nullptr.
        virtual RayTracingDebugSettingsInterface* GetSettingsInterface() = 0;

        //! Called when a ray tracing debug level component is added.
        virtual void OnRayTracingDebugComponentAdded() = 0;

        //! Called when a ray tracing debug level component is removed.
        virtual void OnRayTracingDebugComponentRemoved() = 0;
    };
} // namespace AZ::Render
