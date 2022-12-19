/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class WhiteBalanceComponentConfig final : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::WhiteBalanceComponentConfig, "{FA4DD42D-2BEA-4719-9F11-87E1D4CC165E}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(WhiteBalanceSettingsInterface* settings);
            void CopySettingsTo(WhiteBalanceSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
