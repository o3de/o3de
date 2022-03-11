/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class VignetteComponentConfig final : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::VignetteComponentConfig, "{2CB8446B-1410-4885-A3DE-11BB0590D91A}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/Vignette/VignetteParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/Vignette/VignetteParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(VignetteSettingsInterface* settings);
            void CopySettingsTo(VignetteSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
