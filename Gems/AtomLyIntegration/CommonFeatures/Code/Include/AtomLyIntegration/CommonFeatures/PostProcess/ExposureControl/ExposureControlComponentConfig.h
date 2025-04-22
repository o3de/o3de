/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class ExposureControlComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(ExposureControlComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::ExposureControlComponentConfig, "{3FBB712B-EA05-43DA-A2F5-9C8DA61AA787}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(ExposureControlSettingsInterface* settings);
            void CopySettingsTo(ExposureControlSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
            bool IsEyeAdaptation() const { return m_exposureControlType == ExposureControl::ExposureControlType::EyeAdaptation; }
        };
    }
}
