/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
