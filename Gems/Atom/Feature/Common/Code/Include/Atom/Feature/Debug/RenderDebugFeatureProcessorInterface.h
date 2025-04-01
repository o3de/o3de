/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>

namespace AZ::Render
{
    //! Interface for the RenderDebugFeatureProcessor, which handles debug render settings for the scene, such as
    //! displaying normals to screen or showing only diffuse/specular lighting, etc.
    class RenderDebugFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::RenderDebugFeatureProcessorInterface, "{9774C763-178C-4CE2-99CD-3ABDE12445A4}", AZ::RPI::FeatureProcessor);

        //! Retrieves existing settings. If none found, returns nullptr.
        virtual RenderDebugSettingsInterface* GetSettingsInterface() = 0;

        //! Called when a render debug level component is added
        virtual void OnRenderDebugComponentAdded() = 0;

        //! Called when a render debug level component is removed
        virtual void OnRenderDebugComponentRemoved() = 0;
    };

}
