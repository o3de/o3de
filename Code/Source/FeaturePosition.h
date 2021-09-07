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
    class Pose;

    namespace MotionMatching
    {
        class FrameDatabase;

        class EMFX_API FeaturePosition
            : public Feature
        {
        public:
            AZ_RTTI(FeaturePosition, "{3EAA6459-DB59-4EA1-B8B3-C933A83AA77D}", Feature)
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API FrameCostContext
            {
                const Pose* m_pose;
            };

            FeaturePosition();
            ~FeaturePosition() override = default;

            bool Init(const InitSettings& settings) override;
            void ExtractFrameData(const ExtractFrameContext& context) override;
            void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance, size_t frameIndex) override;
            size_t GetNumDimensionsForKdTree() const override;
            void FillFrameFloats(size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const override;
            void FillFrameFloats(size_t startIndex, AZStd::vector<float>& frameFloats, const FrameCostContext& context);
            void CalcMedians(AZStd::vector<float>& medians, size_t startIndex) const override;

            float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            void SetNodeIndex(size_t nodeIndex);
            size_t CalcMemoryUsageInBytes() const override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::vector<AZ::Vector3> m_positions; /**< A position for every frame. */
            size_t m_nodeIndex; /**< The node to grab the data from. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
