/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/SkinRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkinRule, AZ::SystemAllocator)

            SkinRule::SkinRule()
            {
                DataTypes::SkinRuleSettings defaultSettings = DataTypes::GetDefaultSkinRuleSettings();
                m_maxWeightsPerVertex = defaultSettings.m_maxInfluencesPerVertex;
                m_weightThreshold = defaultSettings.m_weightThreshold;
            }

            AZ::u32 SkinRule::GetMaxWeightsPerVertex() const
            {
                return m_maxWeightsPerVertex;
            }

            float SkinRule::GetWeightThreshold() const
            {
                return m_weightThreshold;
            }

            void SkinRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<ISkinRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

                serializeContext->Class<SkinRule, ISkinRule>()->Version(2)
                    ->Field("maxWeightsPerVertex", &SkinRule::m_maxWeightsPerVertex)
                    ->Field("weightThreshold", &SkinRule::m_weightThreshold);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkinRule>("Skin", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkinRule::m_maxWeightsPerVertex, "Max weights per vertex", "The maximum number of joints that can influence a single vertex.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 32)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkinRule::m_weightThreshold, "Weight threshold", "Weight value less than this will be ignored during import.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 6);
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
