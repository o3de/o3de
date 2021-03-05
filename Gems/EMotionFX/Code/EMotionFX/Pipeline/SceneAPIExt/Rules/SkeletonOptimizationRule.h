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

#include <AzSceneDef.h>
#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class SkeletonOptimizationRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(SkeletonOptimizationRule, "{D3C66FBD-AA6A-4ED3-80C5-A0B0B49AB408}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                SkeletonOptimizationRule() = default;
                ~SkeletonOptimizationRule() override = default;

                bool GetAutoSkeletonLOD() const;
                void SetAutoSkeletonLOD(bool autoSkeletonLOD);

                bool GetServerSkeletonOptimization() const;
                void SetServerSkeletonOptimization(bool serverSkeletonOptimization);

                SceneData::SceneNodeSelectionList& GetCriticalBonesList();

                static void Reflect(AZ::ReflectContext* context);

            protected:
                bool m_autoSkeletonLOD = true;                      // Client side skeleton LOD based on skinning information and critical bones list.
                bool m_serverSkeletonOptimization = false;          // Server side skeleton optimization based on hit detections and critical bones list.
                SceneData::SceneNodeSelectionList m_criticalBonesList;
            };
        } // Rule
    } // Pipeline
} // EMotionFX

