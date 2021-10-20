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

            FeaturePosition();
            ~FeaturePosition() override = default;

            bool Init(const InitSettings& settings) override;

            void ExtractFeatureValues(const ExtractFrameContext& context) override;
            void DebugDraw(AZ::RPI::AuxGeomDrawPtr& drawQueue,
                EMotionFX::DebugDraw::ActorInstanceData& draw,
                BehaviorInstance* behaviorInstance,
                size_t frameIndex) override;

            struct EMFX_API FrameCostContext
            {
                FrameCostContext(const Pose& pose, const FeatureMatrix& featureMatrix)
                    : m_pose(pose)
                    , m_featureMatrix(featureMatrix)
                {
                }

                const Pose& m_pose;
                const FeatureMatrix& m_featureMatrix;
            };
            float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

            void FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context);

            void SetNodeIndex(size_t nodeIndex);

            static void Reflect(AZ::ReflectContext* context);

            size_t GetNumDimensions() const override;
            AZStd::string GetDimensionName(size_t index, Skeleton* skeleton) const override;
            AZ::Vector3 GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const;
            void SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const AZ::Vector3& position);

        private:
            size_t m_nodeIndex; /**< The node to grab the data from. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
