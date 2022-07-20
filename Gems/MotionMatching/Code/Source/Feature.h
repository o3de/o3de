/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <FeatureMatrix.h>
#include <QueryVector.h>

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
    class FeatureMatrixTransformer;

    //! A feature is a property extracted from the animation data and is used by the motion matching algorithm to find the next best matching frame.
    //! Examples of features are the position of the feet joints, the linear or angular velocity of the knee joints or the trajectory history and future
    //! trajectory of the root joint. We can also encode environment sensations like obstacle positions and height, the location of the sword of an enemy
    //! character or a football's position and velocity. Their purpose is to describe a frame of the animation by their key characteristics and sometimes
    //! enhance the actual keyframe data (pos/rot/scale per joint) by e.g. taking the time domain into account and calculate the velocity or acceleration,
    //! or a whole trajectory to describe where the given joint came from to reach the frame and the path it moves along in the near future.
    //! @Note: Features are extracted and stored relative to a given joint, in most cases the motion extraction or root joint, and thus are in model-space.
    //! This makes the search algorithm invariant to the character location and orientation and the extracted features, like e.g. a joint position or velocity,
    //! translate and rotate along with the character.
    class EMFX_API Feature
    {
    public:
        AZ_RTTI(Feature, "{DE9CBC48-9176-4DF1-8306-4B1E621F0E76}")
        AZ_CLASS_ALLOCATOR_DECL

        Feature() = default;
        virtual ~Feature() = default;

        ////////////////////////////////////////////////////////////////////////
        // Initialization
        struct EMFX_API InitSettings
        {
            ActorInstance* m_actorInstance = nullptr;
            FeatureMatrix::Index m_featureColumnStartOffset = 0;
        };
        virtual bool Init(const InitSettings& settings);

        ////////////////////////////////////////////////////////////////////////
        // Feature extraction
        struct EMFX_API ExtractFeatureContext
        {
            ExtractFeatureContext(FeatureMatrix& featureMatrix, AnimGraphPosePool& posePool);

            FrameDatabase* m_frameDatabase = nullptr;
            FeatureMatrix& m_featureMatrix;

            size_t m_frameIndex = InvalidIndex;
            const Pose* m_framePose = nullptr; //!< Pre-sampled pose for the given frame.
            AnimGraphPosePool& m_posePool;

            ActorInstance* m_actorInstance = nullptr;
        };
        virtual void ExtractFeatureValues(const ExtractFeatureContext& context) = 0;

        ////////////////////////////////////////////////////////////////////////
        // Fill query vector
        struct EMFX_API QueryVectorContext
        {
            QueryVectorContext(const Pose& currentPose, const TrajectoryQuery& trajectoryQuery);

            const Pose& m_currentPose; //!< Current actor instance pose.
            const TrajectoryQuery& m_trajectoryQuery;
            FeatureMatrixTransformer* m_featureTransformer = nullptr;
        };
        virtual void FillQueryVector([[maybe_unused]] QueryVector& queryVector,
            [[maybe_unused]] const QueryVectorContext& context) = 0;

        ////////////////////////////////////////////////////////////////////////
        // Feature cost
        struct EMFX_API FrameCostContext
        {
            FrameCostContext(const QueryVector& queryVector, const FeatureMatrix& featureMatrix);

            const QueryVector& m_queryVector; //!< Input query feature values.
            const FeatureMatrix& m_featureMatrix;
        };
        virtual float CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const;

        //! Specifies how the feature value differences (residuals), between the input query values
        //! and the frames in the motion database that sum up the feature cost, are calculated.
        enum ResidualType
        {
            Absolute,
            Squared
        };

        void SetCostFactor(float costFactor) { m_costFactor = costFactor; }
        float GetCostFactor() const { return m_costFactor; }
        virtual void DebugDraw([[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
            [[maybe_unused]] const Pose& currentPose,
            [[maybe_unused]] const FeatureMatrix& featureMatrix,
            [[maybe_unused]] const FeatureMatrixTransformer* featureTransformer,
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

    protected:
        //! Calculate a normalized direction vector difference between the two given vectors.
        //! A dot product of the two vectors is taken and the result in range [-1, 1] is scaled to [0, 1].
        //! @result Normalized, absolute difference between the vectors. 
        //! Angle difference     dot result      cost
        //! 0.0 degrees          1.0             0.0
        //! 90.0 degrees         0.0             0.5
        //! 180.0 degrees        -1.0            1.0
        //! 270.0 degrees        0.0             0.5
        float GetNormalizedDirectionDifference(const AZ::Vector2& directionA, const AZ::Vector2& directionB) const;
        float GetNormalizedDirectionDifference(const AZ::Vector3& directionA, const AZ::Vector3& directionB) const;

        float CalcResidual(float value) const;
        float CalcResidual(const AZ::Vector3& a, const AZ::Vector3& b) const;

        virtual AZ::Crc32 GetCostFactorVisibility() const;

        // Shared and reflected data.
        AZ::TypeId m_id = AZ::TypeId::CreateRandom(); //< The feature identification number. Use this instead of the RTTI class ID so that we can have multiple of the same type.
        AZStd::string m_name; //< Display name used for feature identification and debug visualizations.
        AZStd::string m_jointName; //< Joint name to extract the data from.
        AZStd::string m_relativeToJointName; //< When extracting feature data, convert it to relative-space to the given joint.
        AZ::Color m_debugColor = AZ::Colors::Green; //< Color used for debug visualizations to identify the feature.
        bool m_debugDrawEnabled = false; //< Are debug visualizations enabled for this feature?
        float m_costFactor = 1.0f; //< The cost factor for the feature is multiplied with the actual and can be used to change a feature's influence in the motion matching search.
        ResidualType m_residualType = ResidualType::Absolute; //< How do we calculate the differences (residuals) between the input query values and the frames in the motion database that sum up the feature cost.

        // Instance data (depends on the feature schema or actor instance).
        FeatureMatrix::Index m_featureColumnOffset; //< Float/Value offset, starting column for where the feature should be places at.
        size_t m_relativeToNodeIndex = InvalidIndex;
        size_t m_jointIndex = InvalidIndex;
    };
} // namespace EMotionFX::MotionMatching
