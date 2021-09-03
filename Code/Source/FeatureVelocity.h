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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <Feature.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace MotionMatching
    {
        class FrameDatabase;

        class EMFX_API FeatureVelocity
            : public Feature
        {
        public:
            AZ_RTTI(FeatureVelocity, "{DEEA4F0F-CE70-4F16-9136-C2BFDDA29336}", Feature)
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API Velocity
            {
                AZ::Vector3 m_direction; /**< Direction we are moving to. */
                float m_speed; /**< The speed at which we move into this direction (m/s). */
            };

            struct EMFX_API FrameCostContext
            {
                AZ::Vector3 m_direction;
                float m_speed;
            };

            FeatureVelocity();
            ~FeatureVelocity() override = default;

            bool Init(const InitSettings& settings) override;
            void ExtractFrameData(const ExtractFrameContext& context) override;
            void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance) override;
            size_t GetNumDimensionsForKdTree() const override;
            void FillFrameFloats(size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const override;
            void FillFrameFloats(size_t startIndex, AZStd::vector<float>& frameFloats, const FrameCostContext& context);
            void CalcMedians(AZStd::vector<float>& medians, size_t startIndex) const override;

            float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            AZ_INLINE const Velocity& GetVelocity(size_t frameIndex) const { return m_velocities[frameIndex]; }

            void SetNodeIndex(size_t nodeIndex);
            size_t CalcMemoryUsageInBytes() const override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::vector<Velocity> m_velocities; /**< The velocities for each frame. */
            size_t m_nodeIndex; /**< The node to grab the data from. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
