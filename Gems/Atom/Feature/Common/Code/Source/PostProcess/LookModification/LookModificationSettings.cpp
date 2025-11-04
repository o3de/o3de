/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
            seed = TypeHash64(m_asset.GetId(), seed);
            seed = TypeHash64(m_shaperPreset, seed);
            seed = TypeHash64(m_customMinExposure, seed);
            seed = TypeHash64(m_customMaxExposure, seed);
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
            AZ_Assert(target != nullptr, "LookModificationSettings::ApplySettingsTo called with nullptr as argument.");

            auto lutAsset = GetColorGradingLut();
            if (GetEnabled() && lutAsset.GetId().IsValid())
            {
                Render::LutBlendItem lutBlend;
                lutBlend.m_intensity = GetColorGradingLutIntensity();
                lutBlend.m_overrideStrength = GetColorGradingLutOverride() * alpha;
                lutBlend.m_asset = lutAsset;
                lutBlend.m_shaperPreset = GetShaperPresetType();
                lutBlend.m_customMinExposure = GetCustomMinExposure();
                lutBlend.m_customMaxExposure = GetCustomMaxExposure();
                target->AddLutBlend(lutBlend);
            }
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
                    blendItem.m_asset = GetColorGradingLut();
                    blendItem.m_shaperPreset = GetShaperPresetType();
                    blendItem.m_customMinExposure = GetCustomMinExposure();
                    blendItem.m_customMaxExposure = GetCustomMaxExposure();
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
