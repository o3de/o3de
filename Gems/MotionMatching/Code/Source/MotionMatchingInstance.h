/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <Feature.h>
#include <TrajectoryHistory.h>
#include <TrajectoryQuery.h>

#include <MotionMatching/MotionMatchingBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class ActorInstance;
    class Motion;
}

namespace EMotionFX::MotionMatching
{
    class MotionMatchingData;

    //! The instance is where everything comes together. It stores the trajectory history, the trajectory query along with the query vector, knows about the
    //! last lowest cost frame frame index and stores the time of the animation that the instance is currently playing. It is responsible for motion extraction,
    //! blending towards a new frame in the motion capture database in case the algorithm found a better matching frame and executes the actual search.
    class EMFX_API MotionMatchingInstance
        : public DebugDrawRequestBus::Handler
    {
    public:
        AZ_RTTI(MotionMatchingInstance, "{1ED03AD8-0FB2-431B-AF01-02F7E930EB73}")
        AZ_CLASS_ALLOCATOR_DECL

        MotionMatchingInstance() = default;
        virtual ~MotionMatchingInstance();

        struct EMFX_API InitSettings
        {
            ActorInstance* m_actorInstance = nullptr;
            MotionMatchingData* m_data = nullptr;
        };
        void Init(const InitSettings& settings);

        // DebugDrawRequestBus::Handler overrides
        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay) override;

        void Update(float timePassedInSeconds, const AZ::Vector3& targetPos, const AZ::Vector3& targetFacingDir, TrajectoryQuery::EMode mode, float pathRadius, float pathSpeed);
        void PostUpdate(float timeDelta);
        void Output(Pose& outputPose);

        MotionInstance* GetMotionInstance() const { return m_motionInstance; }
        ActorInstance* GetActorInstance() const { return m_actorInstance; }
        MotionMatchingData* GetData() const { return m_data; }

        size_t GetLowestCostFrameIndex() const { return m_lowestCostFrameIndex; }
        void SetLowestCostSearchFrequency(float frequency) { m_lowestCostSearchFrequency = frequency; }
        float GetNewMotionTime() const { return m_newMotionTime; }

        //! Get the cached trajectory feature.
        //! The trajectory feature is searched in the feature schema used in the current instance at init time.
        FeatureTrajectory* GetTrajectoryFeature() const { return m_cachedTrajectoryFeature; }
        const TrajectoryQuery& GetTrajectoryQuery() const { return m_trajectoryQuery; }
        const TrajectoryHistory& GetTrajectoryHistory() const { return m_trajectoryHistory; }
        const Transform& GetMotionExtractionDelta() const { return m_motionExtractionDelta; }

    private:
        MotionInstance* CreateMotionInstance() const;
        void SamplePose(MotionInstance* motionInstance, Pose& outputPose);
        void SamplePose(Motion* motion, Pose& outputPose, float sampleTime) const;

        size_t FindLowestCostFrameIndex(const Feature::FrameCostContext& context);

        MotionMatchingData* m_data = nullptr;
        ActorInstance* m_actorInstance = nullptr;
        Pose m_blendSourcePose;
        Pose m_blendTargetPose;
        Pose m_queryPose; //! Input query pose for the motion matching search.
        MotionInstance* m_motionInstance = nullptr;
        MotionInstance* m_prevMotionInstance = nullptr;
        Transform m_motionExtractionDelta = Transform::CreateIdentity();

        /// Buffers used for the broad-phase KD-tree search.
        AZStd::vector<float> m_queryFeatureValues; //< The input query features to be compared to every entry/row in the feature matrix with the motion matching search.
        AZStd::vector<size_t> m_nearestFrames; //< Stores the nearest matching frames / search result from the KD-tree.

        FeatureTrajectory* m_cachedTrajectoryFeature = nullptr; //< Cached pointer to the trajectory feature in the feature schema.
        TrajectoryQuery m_trajectoryQuery;
        TrajectoryHistory m_trajectoryHistory;
        static constexpr float m_trajectorySecsToTrack = 5.0f;

        float m_timeSinceLastFrameSwitch = 0.0f;
        float m_newMotionTime = 0.0f;
        size_t m_lowestCostFrameIndex = InvalidIndex;
        float m_lowestCostSearchFrequency = 5.0f; //< How often the lowest cost frame shall be searched per second.

        bool m_blending = false;
        float m_blendWeight = 1.0f;
        float m_blendProgressTime = 0.0f; //< How long are we already blending? In seconds.

        /// Buffers used for FindLowestCostFrameIndex().
        AZStd::vector<float> m_tempCosts;
        AZStd::vector<float> m_minCosts;

        AZStd::vector<AzFramework::DebugDisplayRequests*> m_debugDisplays;
    };
} // namespace EMotionFX::MotionMatching
