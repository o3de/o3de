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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>
#include <FeatureDatabase.h>
#include <FrameDatabase.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class ActorInstance;

    namespace MotionMatching
    {
        class BehaviorInstance;

        class EMFX_API Behavior
        {
        public:
            AZ_RTTI(Behavior, "{7BC3DFF5-8864-4518-B6F0-0553ADFAB5C1}")
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API InitSettings
            {
                ActorInstance* m_actorInstance = nullptr;
                AZStd::vector<Motion*> m_motionList;
                FrameDatabase::FrameImportSettings m_frameImportSettings;
                size_t m_maxKdTreeDepth = 20;
                size_t m_minFramesPerKdTreeNode = 1000;
                bool m_importMirrored = false;

            };

            Behavior() = default;
            virtual ~Behavior() = default;

            virtual bool RegisterParameters(const InitSettings& settings) = 0;
            virtual bool RegisterFrameDatas(const InitSettings& settings) = 0;
            virtual bool Init(const InitSettings& settings);
            virtual void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance);

            virtual size_t FindLowestCostFrameIndex(BehaviorInstance* behaviorInstance, const Pose& inputPose, const Pose& previousPose, size_t currentFrameIndex, float timeDelta) = 0;

            static void Reflect(AZ::ReflectContext* context);
            static Behavior* CreateBehaviorByType(const AZ::TypeId& typeId);

            AZ_INLINE const FrameDatabase& GetData() const { return m_data; }
            AZ_INLINE FrameDatabase& GetData() { return m_data; }

            const FeatureDatabase& GetFeatures() const { return m_features; }
            FeatureDatabase& GetFeatures() { return m_features; }

        protected:
            FrameDatabase m_data; /**< The frames and their data. */
            FeatureDatabase m_features;
            float m_newMotionTime = 0.0f; /**< New motion instance time before sync. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
