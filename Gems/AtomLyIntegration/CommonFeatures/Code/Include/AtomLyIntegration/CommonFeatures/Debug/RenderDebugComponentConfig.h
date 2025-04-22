/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>

namespace AZ::Render
{
    class RenderDebugComponentConfig final
        : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderDebugComponentConfig, SystemAllocator)
        AZ_RTTI(AZ::Render::RenderDebugComponentConfig, "{07362463-95C9-40CE-89E7-76FD25978E96}", AZ::ComponentConfig);

        static void Reflect(ReflectContext* context);

        // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        void CopySettingsFrom(RenderDebugSettingsInterface* settings);
        void CopySettingsTo(RenderDebugSettingsInterface* settings);

        bool IsBaseColorReadOnly() { return !GetOverrideBaseColor(); }
        bool IsRoughnessReadOnly() { return !GetOverrideRoughness(); }
        bool IsMetallicReadOnly() { return !GetOverrideMetallic(); }
        bool IsDebugLightReadOnly() { return GetRenderDebugLightingSource() != RenderDebugLightingSource::DebugLight; }

    };

}
