/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>

namespace AZ::Render
{
    class RenderDebugFeatureProcessor;

    // The post process sub-settings class for SSAO (Screen-Space Ambient Occlusion)
    class RenderDebugSettings final
        : public RenderDebugSettingsInterface
    {
        friend class RenderDebugFeatureProcessor;

    public:
        AZ_RTTI(AZ::Render::RenderDebugSettings, "{942CB951-C5D0-4E90-9F55-633DAA561A03}", AZ::Render::RenderDebugSettingsInterface);
        AZ_CLASS_ALLOCATOR(RenderDebugSettings, SystemAllocator);

        RenderDebugSettings(RenderDebugFeatureProcessor* featureProcessor);
        ~RenderDebugSettings() = default;

        u32 GetRenderDebugOptions() { return m_optionsMask; }

        // Generate getters and setters.
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

    private:

        // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        void Simulate();
        void UpdateOptionsMask();

        u32 m_optionsMask = 0;
    };

}
