/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/Ssao/SsaoSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class SsaoComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(SsaoComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::SsaoComponentConfig, "{C1ACBBE7-C3BD-4245-B571-3B5FDCAC0B0B}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(SsaoSettingsInterface* settings);
            void CopySettingsTo(SsaoSettingsInterface* settings);
        };
    }
}
