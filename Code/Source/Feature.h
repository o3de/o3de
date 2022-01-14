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
};

namespace EMotionFX::MotionMatching
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
        virtual bool Init(const InitSettings& settings);

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
        virtual float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

        void SetCostFactor(float costFactor) { m_costFactor = costFactor; }
        float GetCostFactor() const { return m_costFactor; }

        virtual void FillQueryFeatureValues([[maybe_unused]] size_t startIndex,
            [[maybe_unused]] AZStd::vector<float>& queryFeatureValues,
            [[maybe_unused]] const FrameCostContext& context) {}

        virtual void DebugDraw([[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
            [[maybe_unused]] MotionMatchingInstance* instance,
            [[maybe_unused]] size_t frameIndex) {}

        void SetDebugDrawColor(const AZ::Color& color);
        const AZ::Color& GetDebugDrawColor() const;

        void SetDebugDrawEnabled(bool enabled);
        bool GetDebugDrawEnabled() const;

        void SetJointName(const AZStd::string& jointName) { m_jointName = jointName; }
        const AZStd::string& GetJointName() const { return m_jointName; }

        void SetRelativeToJointName(const AZStd::string& jointName) { m_relativeToJointName = jointName; }
        const AZStd::string& GetRelativeToJointName() const { return m_relativeToJointName; }

        void SetName(const AZStd::string& name) { m_name = name; }
        const AZStd::string& GetName() const { return m_name; }

        // Column offset for the first value for the given feature inside the feature matrix.
        virtual size_t GetNumDimensions() const = 0;
        virtual AZStd::string GetDimensionName([[maybe_unused]] size_t index) const { return "Unknown"; }
        FeatureMatrix::Index GetColumnOffset() const { return m_featureColumnOffset; }
        void SetColumnOffset(FeatureMatrix::Index offset) { m_featureColumnOffset = offset; }

        const AZ::TypeId& GetId() const { return m_id; }
        size_t GetRelativeToNodeIndex() const { return m_relativeToNodeIndex; }
        void SetRelativeToNodeIndex(size_t nodeIndex);

        static void Reflect(AZ::ReflectContext* context);
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

        // Shared and reflected data.
        AZ::TypeId m_id = AZ::TypeId::CreateRandom(); /**< The frame data id. Use this instead of the RTTI class Id. This is because we can have multiple of the same types. */
        AZStd::string m_name; /**< Display name used for feature identification and debug visualizations. */
        AZStd::string m_jointName; /**< Joint name to extract the data from. */
        AZStd::string m_relativeToJointName; /**< Make the data relative to this node (default=0). */
        AZ::Color m_debugColor = AZ::Colors::Green; /**< The debug drawing color. */
        bool m_debugDrawEnabled = false; /**< Is debug drawing enabled for this data? */
        float m_costFactor = 1.0f; /** The cost factor for the feature is multiplied with the actual and can be used to change a feature's influence in the motion matching search. */

        // Instance data (depends on the feature schema or actor instance).
        FeatureMatrix::Index m_featureColumnOffset; // Float/Value offset, starting column for where the feature should be places at
        size_t m_relativeToNodeIndex = InvalidIndex;
        size_t m_jointIndex = InvalidIndex;
    };
} // namespace EMotionFX::MotionMatching
