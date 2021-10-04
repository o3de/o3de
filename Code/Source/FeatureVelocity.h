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

            FeatureVelocity();
            ~FeatureVelocity() override = default;

            bool Init(const InitSettings& settings) override;
            void ExtractFrameData(const ExtractFrameContext& context) override;
            void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance, size_t frameIndex) override;

            struct EMFX_API FrameCostContext
            {
                FrameCostContext(const FeatureMatrix& featureMatrix)
                    : m_featureMatrix(featureMatrix)
                {
                }

                const FeatureMatrix& m_featureMatrix;
                AZ::Vector3 m_direction;
                float m_speed;
            };
            float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            void FillFrameFloats(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const override;
            void FillFrameFloats(size_t startIndex, AZStd::vector<float>& frameFloats, const FrameCostContext& context);

            void SetNodeIndex(size_t nodeIndex);

            static void Reflect(AZ::ReflectContext* context);

            size_t GetNumDimensions() const override;
            Velocity GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const;
            void SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const Velocity& velocity);

        private:
            size_t m_nodeIndex = InvalidIndex; /**< The node to grab the data from. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
