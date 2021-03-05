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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPIExt/Rules/SkeletonOptimizationRule.h>
#include <SceneAPIExt/Behaviors/SkeletonOptimizationRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkeletonOptimizationRule, AZ::SystemAllocator, 0)

            bool SkeletonOptimizationRule::GetAutoSkeletonLOD() const
            {
                return m_autoSkeletonLOD;
            }

            void SkeletonOptimizationRule::SetAutoSkeletonLOD(bool autoSkeletonLOD)
            {
                m_autoSkeletonLOD = autoSkeletonLOD;
            }

            bool SkeletonOptimizationRule::GetServerSkeletonOptimization() const
            {
                return m_serverSkeletonOptimization;
            }

            void SkeletonOptimizationRule::SetServerSkeletonOptimization(bool serverSkeletonOptimization)
            {
                m_serverSkeletonOptimization = serverSkeletonOptimization;
            }

            SceneData::SceneNodeSelectionList& SkeletonOptimizationRule::GetCriticalBonesList()
            {
                return m_criticalBonesList;
            }

            void SkeletonOptimizationRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkeletonOptimizationRule, IRule>()->Version(2)
                    ->Field("autoSkeletonLOD", &SkeletonOptimizationRule::m_autoSkeletonLOD)
                    ->Field("serverSkeletonOptimization", &SkeletonOptimizationRule::m_serverSkeletonOptimization)
                    ->Field("criticalBonesList", &SkeletonOptimizationRule::m_criticalBonesList);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkeletonOptimizationRule>("Skeleton Optimization", "Advanced skeleton optimization rule.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkeletonOptimizationRule::m_autoSkeletonLOD, "Auto Skeleton LOD", "(Client side) The actor will automatically build skeletal LOD based on skinning information.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkeletonOptimizationRule::m_serverSkeletonOptimization, "Server Skeleton Optimize", "(Server side) The actor will automatically build an optimized version of skeleton based on hit detection colliders")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SkeletonOptimizationRule::m_criticalBonesList, "Critical bones", "Bones in this list will not be optimized out.")
                            ->Attribute("FilterName", "bones")
                            ->Attribute("NarrowSelection", true)
                            ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid());
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX