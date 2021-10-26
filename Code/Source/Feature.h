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

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>

#include <FeatureMatrix.h>

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

        class EMFX_API Feature
        {
        public:
            AZ_RTTI(Feature, "{DE9CBC48-9176-4DF1-8306-4B1E621F0E76}")
            AZ_CLASS_ALLOCATOR_DECL

            Feature() = default;
            virtual ~Feature() = default;

            struct EMFX_API InitSettings
            {
                ActorInstance* m_actorInstance = nullptr;
                FeatureMatrix::Index m_featureColumnStartOffset = 0;
            };
            virtual bool Init(const InitSettings& settings) = 0;

            struct EMFX_API ExtractFrameContext
            {
                ExtractFrameContext(FeatureMatrix& featureMatrix)
                    : m_featureMatrix(featureMatrix)
                {
                }

                size_t m_frameIndex = InvalidIndex;
                size_t m_nextFrameIndex = InvalidIndex;
                size_t m_prevFrameIndex = InvalidIndex;
                FrameDatabase* m_data = nullptr;
                MotionInstance* m_motionInstance = nullptr;
                const Pose* m_pose = nullptr;
                const Pose* m_previousPose = nullptr;
                const Pose* m_nextPose = nullptr;
                float m_timeDelta = 0.0f;

                FeatureMatrix& m_featureMatrix;
            };
            virtual void ExtractFeatureValues(const ExtractFrameContext& context) = 0;

            virtual void DebugDraw([[maybe_unused]] AZ::RPI::AuxGeomDrawPtr& drawQueue,
                [[maybe_unused]] EMotionFX::DebugDraw::ActorInstanceData& draw,
                [[maybe_unused]] BehaviorInstance* behaviorInstance,
                [[maybe_unused]] size_t frameIndex) {}

            void SetDebugDrawColor(const AZ::Color& color);
            const AZ::Color& GetDebugDrawColor() const;

            void SetDebugDrawEnabled(bool enabled);
            bool GetDebugDrawEnabled() const;

            // Column offset for the first value for the given feature
            virtual size_t GetNumDimensions() const = 0;
            virtual AZStd::string GetDimensionName([[maybe_unused]] size_t index, [[maybe_unused]] Skeleton* skeleton) const { return "Unknown"; }
            FeatureMatrix::Index GetColumnOffset() const { return m_featureColumnOffset; }
            void SetColumnOffset(FeatureMatrix::Index offset) { m_featureColumnOffset = offset; }

            const AZ::TypeId& GetId() const { return m_id; }
            size_t GetRelativeToNodeIndex() const { return m_relativeToNodeIndex; }
            void SetId(const AZ::TypeId& id);
            void SetRelativeToNodeIndex(size_t nodeIndex);
            void SetData(FrameDatabase* data);
            FrameDatabase* GetData() const;

            static void Reflect(AZ::ReflectContext* context);
            static void SamplePose(float sampleTime, const Pose* bindPose, Motion* sourceMotion, MotionInstance* motionInstance, Pose* samplePose);
            static void CalculateVelocity(size_t jointIndex, const Pose* curPose, const Pose* nextPose, float timeDelta, AZ::Vector3& outDirection, float& outSpeed);
            static void CalculateVelocity(size_t jointIndex, size_t relativeToJointIndex, MotionInstance* motionInstance, AZ::Vector3& outDirection, float& outSpeed);

        protected:
            /**
             * Calculate a normalized direction vector difference between the two given vectors.
             * A dot product of the two vectors is taken and the result in range [-1, 1] is scaled to [0, 1].
             * @result Normalized, absolute difference between the vectors. 
             * Angle difference     dot result      cost
             * 0.0 degrees          1.0             0.0
             * 90.0 degrees         0.0             0.5
             * 180.0 degrees        -1.0            1.0
             * 270.0 degrees        0.0             0.5
             **/
            float GetNormalizedDirectionDifference(const AZ::Vector3& directionA, const AZ::Vector3& directionB) const;

            AZ::TypeId m_id = AZ::TypeId::CreateRandom(); /**< The frame data id. Use this instead of the RTTI class Id. This is because we can have multiple of the same types. */
            FrameDatabase* m_data = nullptr; /**< The data we point into. This class does not own the data. */
            size_t m_relativeToNodeIndex = 0; /**< Make the data relative to this node (default=0). */
            AZ::Color m_debugColor = AZ::Colors::Green; /**< The debug drawing color. */
            bool m_debugDrawEnabled = false; /**< Is debug drawing enabled for this data? */

            FeatureMatrix::Index m_featureColumnOffset; // Float/Value offset, starting column for where the feature should be places at
        };
    } // namespace MotionMatching
} // namespace EMotionFX
