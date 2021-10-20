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
#include <AzCore/Math/Quaternion.h>
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

        class EMFX_API FeatureDirection
            : public Feature
        {
        public:
            AZ_RTTI(FeatureDirection, "{FDA7C5B5-0B18-45E5-893B-433FB38897F1}", Feature)
            AZ_CLASS_ALLOCATOR_DECL

            enum EAxis
            {
                AXIS_X = 0,
                AXIS_Y = 1,
                AXIS_Z = 2
            };

            FeatureDirection();
            ~FeatureDirection() override = default;

            bool Init(const InitSettings& settings) override;

            void ExtractFeatureValues(const ExtractFrameContext& context) override;
            size_t GetNumDimensions() const override;
            void FillFrameFloats(const FeatureMatrix& featureMatrix, size_t frameIndex, size_t startIndex, AZStd::vector<float>& frameFloats) const override;

            struct EMFX_API FrameCostContext
            {
                const Pose* m_pose;
            };
            float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            void SetNodeIndex(size_t nodeIndex);
            AZ_INLINE const AZ::Vector3& GetDirection(size_t frameIndex) const { return m_directions[frameIndex]; }

            void DebugDrawDirection(EMotionFX::DebugDraw::ActorInstanceData& draw, size_t frameIndex, const AZ::Vector3 startPoint, const AZ::Transform& transform, const AZ::Color& color);

            AZ::Vector3 ExtractDirection(const AZ::Quaternion& quaternion) const;

            static void Reflect(AZ::ReflectContext* context);

        private:          
            AZStd::vector<AZ::Vector3> m_directions; /**< A position for every frame. */
            size_t m_nodeIndex; /**< The node to grab the data from. */
            EAxis m_axis; /**< The rotation axis to use as direction vector. */
            bool m_flipAxis; /**< Flip the axis? */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
