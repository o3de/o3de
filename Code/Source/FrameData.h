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

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/DebugDraw.h>

namespace EMotionFX
{
    class ActorInstance;
    class MotionInstance;
    class Pose;
    class Motion;

    namespace MotionMatching
    {
        class Frame;
        class FrameDatabase;
        class BehaviorInstance;

        class EMFX_API FrameData
        {
        public:
            AZ_RTTI(FrameData, "{DE9CBC48-9176-4DF1-8306-4B1E621F0E76}")
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API InitSettings
            {
                ActorInstance* m_actorInstance = nullptr;
            };

            struct EMFX_API ExtractFrameContext
            {
                size_t m_frameIndex = ~0;
                size_t m_nextFrameIndex = ~0;
                size_t m_prevFrameIndex = ~0;
                FrameDatabase* m_data = nullptr;
                MotionInstance* m_motionInstance = nullptr;
                const Pose* m_pose = nullptr;
                const Pose* m_previousPose = nullptr;
                const Pose* m_nextPose = nullptr;
                float m_timeDelta = 0.0f;
            };

            FrameData();
            virtual ~FrameData() = default;

            virtual bool Init(const InitSettings& settings) = 0;
            virtual void ExtractFrameData(const ExtractFrameContext& context) = 0;
            virtual void DebugDraw([[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw, [[maybe_unused]] BehaviorInstance* behaviorInstance) {}
            virtual void PrepareForFrameSampling([[maybe_unused]] const Pose& currentPose) {}
            virtual size_t GetNumDimensionsForKdTree() const = 0;
            virtual void FillFrameFloats([[maybe_unused]] size_t frameIndex, [[maybe_unused]] size_t startIndex, [[maybe_unused]] AZStd::vector<float>& frameFloats) const {}
            virtual void CalcMedians([[maybe_unused]] AZStd::vector<float>& medians, [[maybe_unused]] size_t startIndex) const {};

            void SetDebugDrawColor(const AZ::Color& color);
            const AZ::Color& GetDebugDrawColor() const;

            void SetDebugDrawEnabled(bool enabled);
            bool GetDebugDrawEnabled() const;

            void SetIncludeInKdTree(bool include);
            bool GetIncludeInKdTree() const;

            const AZ::TypeId& GetId() const { return m_id; }
            size_t GetRelativeToNodeIndex() const { return m_relativeToNodeIndex; }
            void SetId(const AZ::TypeId& id);
            void SetRelativeToNodeIndex(size_t nodeIndex);
            void SetData(FrameDatabase* data);
            FrameDatabase* GetData() const;

            virtual size_t CalcMemoryUsageInBytes() const = 0;

            static void Reflect(AZ::ReflectContext* context);
            static void SamplePose(float sampleTime, const Pose* bindPose, Motion* sourceMotion, MotionInstance* motionInstance, Pose* samplePose);
            static void CalculateVelocity(size_t jointIndex, const Pose* curPose, const Pose* nextPose, float timeDelta, AZ::Vector3& outDirection, float& outSpeed);
            static void CalculateVelocity(size_t jointIndex, size_t relativeToJointIndex, MotionInstance* motionInstance, AZ::Vector3& outDirection, float& outSpeed);

        protected:
            AZ::TypeId m_id = AZ::TypeId::CreateNull(); /**< The frame data id. Use this instead of the RTTI class Id. This is because we can have multiple of the same types. */
            FrameDatabase* m_data = nullptr; /**< The data we point into. This class does not own the data. */
            size_t m_relativeToNodeIndex = 0; /**< Make the data relative to this node (default=0). */
            AZ::Color m_debugColor = AZ::Colors::Green; /**< The debug drawing color. */
            bool m_debugDrawEnabled = false; /**< Is debug drawing enabled for this data? */
            bool m_includeInKdTree = true; /**< Include in kdTree acceleration structure? */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
