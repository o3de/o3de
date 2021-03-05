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


#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>


#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/Ssao/SsaoSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {

        SsaoSettings::SsaoSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void SsaoSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void SsaoSettings::ApplySettingsTo(SsaoSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "SsaoSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <Atom/Feature/ParamMacros/StartOverrideBlend.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void SsaoSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
        }

    } // namespace Render
} // namespace AZ
