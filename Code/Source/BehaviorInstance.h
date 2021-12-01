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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <TrajectoryHistory.h>
#include <TrajectoryQuery.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class ActorInstance;

    namespace MotionMatching
    {
        class Behavior;

        class EMFX_API BehaviorInstance
        {
        public:
            AZ_RTTI(BehaviorInstance, "{1ED03AD8-0FB2-431B-AF01-02F7E930EB73}")
            AZ_CLASS_ALLOCATOR_DECL

            struct EMFX_API InitSettings
            {
                ActorInstance* m_actorInstance = nullptr;
                Behavior* m_behavior = nullptr;
            };

            virtual ~BehaviorInstance();

            void Init(const InitSettings& settings);
            void DebugDraw();

            void Update(float timePassedInSeconds, const AZ::Vector3& targetPos, const AZ::Vector3& targetFacingDir, TrajectoryQuery::EMode mode, float pathRadius, float pathSpeed);
            void Output(Pose& outputPose);

            MotionInstance* GetMotionInstance() const { return m_motionInstance; }
            ActorInstance* GetActorInstance() const { return m_actorInstance; }
            Behavior* GetBehavior() const { return m_behavior; }

            size_t GetLowestCostFrameIndex() const;

            void SetTimeSinceLastFrameSwitch(float newTime) { m_timeSinceLastFrameSwitch = newTime; }
            float GetTimeSinceLastFrameSwitch() const { return m_timeSinceLastFrameSwitch; }

            void SetLowestCostSearchFrequency(float timeInSeconds) { m_lowestCostSearchFrequency = timeInSeconds; }
            float GetLowestCostSearchFrequency() const { return m_lowestCostSearchFrequency; }

            static void Reflect(AZ::ReflectContext* context);

            float GetNewMotionTime() const { return m_newMotionTime; }
            void SetNewMotionTime(float t) { m_newMotionTime = t; }

            const Pose& GetBlendSourcePose() const { return m_blendSourcePose; }

            // Stores the nearest matching frames / the result from the KD-tree
            const AZStd::vector<size_t>& GetNearestFrames() const { return m_nearestFrames; }
            AZStd::vector<size_t>& GetNearestFrames() { return m_nearestFrames; }

            // The input query features to be compared to every entry in the feature database in the motion matching search.
            const AZStd::vector<float>& GetQueryFeatureValues() const { return m_queryFeatureValues; }
            AZStd::vector<float>& GetQueryFeatureValues() { return m_queryFeatureValues; }

            const TrajectoryQuery& GetTrajectoryQuery() const { return m_trajectoryQuery; }
            const TrajectoryHistory& GetTrajectoryHistory() const { return m_trajectoryHistory; }
            Transform GetMotionExtractionDelta() const { return m_motionExtractionDelta; }

        private:
            MotionInstance* CreateMotionInstance() const;
            void SamplePose(MotionInstance* motionInstance, Pose& outputPose);

            Behavior* m_behavior = nullptr;
            ActorInstance* m_actorInstance = nullptr;
            Pose m_blendSourcePose;
            Pose m_blendTargetPose;
            MotionInstance* m_motionInstance = nullptr;
            MotionInstance* m_prevMotionInstance = nullptr;
            Transform m_motionExtractionDelta = Transform::CreateIdentity();

            AZStd::vector<float> m_queryFeatureValues;
            AZStd::vector<size_t> m_nearestFrames;
            TrajectoryQuery m_trajectoryQuery;
            TrajectoryHistory m_trajectoryHistory;
            static constexpr float m_trajectorySecsToTrack = 5.0f;

            float m_timeSinceLastFrameSwitch = 0.0f;
            float m_newMotionTime = 0.0f;
            size_t m_lowestCostFrameIndex = InvalidIndex;
            float m_lowestCostSearchFrequency = 0.1f; // Search lowest cost frame 10 times per second.

            bool m_blending = false;
            float m_blendWeight = 1.0f;
            float m_blendProgressTime = 0.0f; // How long are we already blending? In seconds.

            AZStd::vector<AzFramework::DebugDisplayRequests*> m_debugDisplays;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
