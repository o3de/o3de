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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <ACES/Aces.h>

#include <PostProcess/LookModification/LookModificationSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        HashValue64 LutBlendItem::GetHash(HashValue64 seed) const
        {
            seed = TypeHash64(m_intensity, seed);
            seed = TypeHash64(m_overrideStrength, seed);
            seed = TypeHash64(m_assetId.GetId(), seed);
            seed = TypeHash64(m_shaperPreset, seed);
            return seed;
        }

        LookModificationSettings::LookModificationSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void LookModificationSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void LookModificationSettings::ApplySettingsTo(LookModificationSettings* target, float alpha) const
        {
            AZ_UNUSED(alpha);
            AZ_Assert(target != nullptr, "LookModificationSettings::ApplySettingsTo called with nullptr as argument.");

            auto lutAssetId = GetColorGradingLut();
            if (GetEnabled() && lutAssetId.GetId().IsValid())
            {
                Render::LutBlendItem lutBlend;
                lutBlend.m_intensity = GetColorGradingLutIntensity();
                lutBlend.m_overrideStrength = GetColorGradingLutOverride();
                lutBlend.m_assetId = lutAssetId;
                target->AddLutBlend(lutBlend);
            }

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha

#include <Atom/Feature/ParamMacros/StartOverrideBlend.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void LookModificationSettings::Simulate(float deltaTime)
        {
            AZ_UNUSED(deltaTime);
        }

        void LookModificationSettings::AddLutBlend(LutBlendItem lutBlendItem)
        {
            if (m_lutBlendStack.size() < MaxBlendLuts)
            {
                m_lutBlendStack.push_back(lutBlendItem);
            }
            else
            {
                AZ_Warning("LookModificationSettings", false, "Attempted to add more than the maxiumum number of LUTs of %u for blending.", MaxBlendLuts);
            }
        }

        void LookModificationSettings::PrepareLutBlending()
        {
            if (m_preparedForBlending == true)
            {
                return;
            }
            // If color grading LUT enabled for this setting, push the LUT entry onto the head of the blend stack
            if (GetEnabled())
            {
                auto lutAsset = GetColorGradingLut();
                if (lutAsset.GetId().IsValid())
                {
                    LutBlendItem blendItem;
                    blendItem.m_intensity = GetColorGradingLutIntensity();
                    blendItem.m_overrideStrength = GetColorGradingLutOverride();
                    blendItem.m_assetId = GetColorGradingLut();
                    m_lutBlendStack.insert(m_lutBlendStack.begin(), blendItem);
                }
            }
            // The override strength of the lowest priority LUT should not be considered, but setting to 1.0 here because it makes calculating the weights cleaner with less special casing
            if (m_lutBlendStack.size() > 0)
            {
                m_lutBlendStack[0].m_overrideStrength = 1.0;
            }
            m_preparedForBlending = true;
        }

        size_t LookModificationSettings::GetLutBlendStackSize()
        {
            return m_lutBlendStack.size();
        }

        LutBlendItem& LookModificationSettings::GetLutBlendItem(size_t lutIndex)
        {
            return m_lutBlendStack[lutIndex];
        }

        HashValue64 LookModificationSettings::GetHash() const
        {
            HashValue64 seed = TypeHash64(m_lutBlendStack.size());
            for (int index = 0; index < m_lutBlendStack.size(); index++)
            {
                seed = m_lutBlendStack[index].GetHash(seed);
            }
            return seed;
        }
    } // namespace Render
} // namespace AZ
