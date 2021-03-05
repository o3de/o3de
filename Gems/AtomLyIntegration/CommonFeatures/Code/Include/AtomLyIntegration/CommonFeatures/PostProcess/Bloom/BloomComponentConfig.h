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
#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class BloomComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::BloomComponentConfig, "{23545754-0FAE-4220-99AF-0AA0045F4D8D}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(BloomSettingsInterface* settings);
            void CopySettingsTo(BloomSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
        };
    }
}
