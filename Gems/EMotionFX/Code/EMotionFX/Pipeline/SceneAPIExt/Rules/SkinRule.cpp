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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPIExt/Rules/SkinRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkinRule, AZ::SystemAllocator, 0)

            SkinRule::SkinRule()
                : m_maxWeightsPerVertex(4)
                , m_weightThreshold(0.001f)
            {
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkinRule::m_maxWeightsPerVertex, "Max weights per vertex", "The maximum number of bones that can influence a single vertex.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 4)
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
