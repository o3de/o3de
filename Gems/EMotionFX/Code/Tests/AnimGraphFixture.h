/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SystemComponentFixture.h"
#include <EMotionFX/Source/MotionSet.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Attribute.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphInstance;
    class AnimGraphTimeCondition;
    class Transform;

    class AnimGraphFixture
        : public SystemComponentFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;

        // Derived classes should override and construct the rest of the graph at this point.
        // They should call this base ConstructGraph to get the anim graph and root state machine created.

        virtual void ConstructGraph();

        virtual void ConstructActor();

        AZStd::string SerializeAnimGraph() const;

        // Evaluates the graph
        void Evaluate();

        const Transform& GetOutputTransform(uint32 nodeIndex = 0);

        void AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name);

        template <class ParamType, class InputType>
        void ParamSetValue(const AZStd::string& paramName, const InputType& value)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
            const AZ::u32 paramIndex = static_cast<AZ::u32>(parameterIndex.GetValue());
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(paramIndex);
            ParamType* typeParam = static_cast<ParamType*>(param);
            typeParam->SetValue(value);
        }

        // Helper functions for state machine construction (Works on m_rootStateMachine).
        AnimGraphStateTransition* AddTransition(AnimGraphNode* source, AnimGraphNode* target, float time);
        AnimGraphTimeCondition* AddTimeCondition(AnimGraphStateTransition* transition, float countDownTime);
        AnimGraphStateTransition* AddTransitionWithTimeCondition(AnimGraphNode* source, AnimGraphNode* target, float blendTime, float countDownTime);

        // Helper function for motion set construction (Works on m_motionSet).
        MotionSet::MotionEntry* AddMotionEntry(const AZStd::string& motionId, float motionMaxTime);

        // TraceMessageBus - Intercepting to prevent dialog popup in AnimGraphReferenceNodeWithNoContentsTest.
        virtual bool OnError(const char* /*window*/, const char* /*message*/) override { return true; }


        using SimulateFrameCallback = std::function<void(AnimGraphInstance*, /*time*/ float, /*timeDelta*/ float, /*frame*/ int)>;
        using SimulateCallback = std::function<void(AnimGraphInstance*)>;

        /**
         * Simulation helper with callbacks before and after starting the simulation as well as
         * callbakcs before and after the anim graph update.
         * Example: expectedFps = 60, fpsVariance = 10 -> actual framerate = [55, 65]
         * @param[in] simulationTime Simulation time in seconds.
         * @param[in] expectedFps is the targeted frame rate
         * @param[in] fpsVariance is the range in which the instabilities happen.
         */
        void Simulate(float simulationTime, float expectedFps, float fpsVariance,
            SimulateCallback preCallback,
            SimulateCallback postCallback,
            SimulateFrameCallback preUpdateCallback,
            SimulateFrameCallback postUpdateCallback);

    protected:
        AZStd::unique_ptr<Actor> m_actor;
        ActorInstance* m_actorInstance = nullptr;
        AZStd::unique_ptr<AnimGraph> m_animGraph;
        AnimGraphStateMachine* m_rootStateMachine = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        MotionSet* m_motionSet = nullptr;

        AZStd::unique_ptr<OneBlendTreeNodeAnimGraph> m_blendTreeAnimGraph;
    };
}
