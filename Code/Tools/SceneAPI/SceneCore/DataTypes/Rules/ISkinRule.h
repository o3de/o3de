/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            inline static constexpr AZStd::string_view DefaultMaxSkinInfluencesPerVertexKey{ "/O3DE/SceneAPI/SkinRule/DefaultMaxSkinInfluencesPerVertex" };
            inline static constexpr AZStd::string_view DefaultWeightThresholdKey{ "/O3DE/SceneAPI/SkinRule/DefaultWeightThreshold" };

            struct SkinRuleSettings
            {
                AZ::u32 m_maxInfluencesPerVertex = 8;
                float m_weightThreshold = 0.001;
            };

            inline SkinRuleSettings GetDefaultSkinRuleSettings()
            {
                SkinRuleSettings defaultSettings;
                // Get the project wide default settings for the skin rule
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    AZ::u64 defaultMaxSkinInfluences = 0;
                    if (settingsRegistry->Get(defaultMaxSkinInfluences, SceneAPI::DataTypes::DefaultMaxSkinInfluencesPerVertexKey))
                    {
                        defaultSettings.m_maxInfluencesPerVertex = aznumeric_caster(defaultMaxSkinInfluences);
                    }

                    double defaultWeightThreshold = 0.0f;
                    if (settingsRegistry->Get(defaultWeightThreshold, SceneAPI::DataTypes::DefaultWeightThresholdKey))
                    {
                        defaultSettings.m_weightThreshold = aznumeric_caster(defaultWeightThreshold);
                    }
                }
                return defaultSettings;
            }

            class ISkinRule
                : public IRule
            {
            public:
                AZ_RTTI(ISkinRule, "{5496ECAF-B096-4455-AE72-D55C5B675443}", IRule);
                
                ~ISkinRule() override = default;

                virtual AZ::u32 GetMaxWeightsPerVertex() const = 0;
                virtual float GetWeightThreshold() const = 0;
            };
        }
    } 
}
