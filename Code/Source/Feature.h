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
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

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
        class MotionMatchingInstance;
        class TrajectoryQuery;

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

            struct EMFX_API ExtractFeatureContext
            {
                ExtractFeatureContext(FeatureMatrix& featureMatrix)
                    : m_featureMatrix(featureMatrix)
                {
                }

                FrameDatabase* m_frameDatabase = nullptr;
                FeatureMatrix& m_featureMatrix;

                size_t m_frameIndex = InvalidIndex;
                const Pose* m_framePose = nullptr; //! Pre-sampled pose for the given frame.

                ActorInstance* m_actorInstance = nullptr;
            };
            virtual void ExtractFeatureValues(const ExtractFeatureContext& context) = 0;

            struct EMFX_API FrameCostContext
            {
                FrameCostContext(const FeatureMatrix& featureMatrix, const Pose& currentPose)
                    : m_featureMatrix(featureMatrix)
                    , m_currentPose(currentPose)
                {
                }

                const FeatureMatrix& m_featureMatrix;
                const ActorInstance* m_actorInstance = nullptr;
                const Pose& m_currentPose; //! Current actor instance pose.
                const TrajectoryQuery* m_trajectoryQuery;
            };

            virtual void DebugDraw([[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
                [[maybe_unused]] MotionMatchingInstance* instance,
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
            void SetFrameDatabase(FrameDatabase* frameDatabase);
            FrameDatabase* GetFrameDatabase() const;

            static void Reflect(AZ::ReflectContext* context);
            static void CalculateVelocity(size_t jointIndex, const Pose* curPose, const Pose* nextPose, float timeDelta, AZ::Vector3& outVelocity);
            static void CalculateVelocity(size_t jointIndex, size_t relativeToJointIndex, MotionInstance* motionInstance, AZ::Vector3& outVelocity);
            static void CalculateVelocity(const ActorInstance* actorInstance, size_t jointIndex, size_t relativeToJointIndex, const Frame& frame, AZ::Vector3& outVelocity);

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
            float GetNormalizedDirectionDifference(const AZ::Vector2& directionA, const AZ::Vector2& directionB) const;
            float GetNormalizedDirectionDifference(const AZ::Vector3& directionA, const AZ::Vector3& directionB) const;

            AZ::TypeId m_id = AZ::TypeId::CreateRandom(); /**< The frame data id. Use this instead of the RTTI class Id. This is because we can have multiple of the same types. */
            FrameDatabase* m_frameDatabase = nullptr; /**< The frame database from which the feature got calculated from and belongs to. */
            size_t m_relativeToNodeIndex = 0; /**< Make the data relative to this node (default=0). */
            AZ::Color m_debugColor = AZ::Colors::Green; /**< The debug drawing color. */
            bool m_debugDrawEnabled = false; /**< Is debug drawing enabled for this data? */

            FeatureMatrix::Index m_featureColumnOffset; // Float/Value offset, starting column for where the feature should be places at
        };
    } // namespace MotionMatching
} // namespace EMotionFX
