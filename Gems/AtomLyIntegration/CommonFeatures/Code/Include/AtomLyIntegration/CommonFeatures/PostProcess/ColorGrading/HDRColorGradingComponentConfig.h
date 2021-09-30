/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class HDRColorGradingComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::HDRColorGradingComponentConfig, "{8613EA4A-6E1C-49AD-87F3-9FECCB7EA36D}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsTo(HDRColorGradingSettingsInterface* settings);
        };
    } // namespace Render
} // namespace AZ
