/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/EventHandler.h>
#include <gmock/gmock.h>

namespace EMotionFX
{
    class MockEventHandler
        : public EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(EventHandler, EventHandlerAllocator)
        MOCK_METHOD1(OnEvent, void(const EventInfo& eventInfo));
        MOCK_CONST_METHOD0(GetHandledEventTypes, const AZStd::vector<EventTypes>());
        MOCK_METHOD2(OnPlayMotion, void(Motion* motion, PlayBackInfo* info));
        MOCK_METHOD2(OnStartMotionInstance, void(MotionInstance* motionInstance, PlayBackInfo* info));
        MOCK_METHOD1(OnDeleteMotionInstance, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnDeleteMotion, void(Motion* motion));
        MOCK_METHOD1(OnStop, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnHasLooped, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnHasReachedMaxNumLoops, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnHasReachedMaxPlayTime, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnIsFrozenAtLastFrame, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnChangedPauseState, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnChangedActiveState, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnStartBlending, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnStopBlending, void(MotionInstance* motionInstance));
        MOCK_METHOD2(OnQueueMotionInstance, void(MotionInstance* motionInstance, PlayBackInfo* info));
        MOCK_METHOD1(OnDeleteActor, void(Actor* actor));
        MOCK_METHOD1(OnDeleteActorInstance, void(ActorInstance* actorInstance));
        MOCK_METHOD1(OnSimulatePhysics, void(float timeDelta));
        MOCK_METHOD2(OnCustomEvent, void(uint32 eventType, void* data));
        MOCK_METHOD7(OnDrawTriangle, void(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color));
        MOCK_METHOD0(OnDrawTriangles, void());
        MOCK_METHOD1(OnCreateAnimGraph, void(AnimGraph* animGraph));
        MOCK_METHOD1(OnCreateAnimGraphInstance, void(AnimGraphInstance* animGraphInstance));
        MOCK_METHOD1(OnCreateMotion, void(Motion* motion));
        MOCK_METHOD1(OnCreateMotionSet, void(MotionSet* motionSet));
        MOCK_METHOD1(OnCreateMotionInstance, void(MotionInstance* motionInstance));
        MOCK_METHOD1(OnCreateMotionSystem, void(MotionSystem* motionSystem));
        MOCK_METHOD1(OnCreateActor, void(Actor* actor));
        MOCK_METHOD1(OnCreateActorInstance, void(ActorInstance* actorInstance));
        MOCK_METHOD1(OnPostCreateActor, void(Actor* actor));
        MOCK_METHOD1(OnDeleteAnimGraph, void(AnimGraph* animGraph));
        MOCK_METHOD1(OnDeleteAnimGraphInstance, void(AnimGraphInstance* animGraphInstance));
        MOCK_METHOD1(OnDeleteMotionSet, void(MotionSet* motionSet));
        MOCK_METHOD1(OnDeleteMotionSystem, void(MotionSystem* motionSystem));
        MOCK_METHOD3(OnRayIntersectionTest, bool(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo));
        MOCK_METHOD2(OnStateEnter, void(AnimGraphInstance* animGraphInstance, AnimGraphNode* state));
        MOCK_METHOD2(OnStateEntering, void(AnimGraphInstance* animGraphInstance, AnimGraphNode* state));
        MOCK_METHOD2(OnStateExit, void(AnimGraphInstance* animGraphInstance, AnimGraphNode* state));
        MOCK_METHOD2(OnStateEnd, void(AnimGraphInstance* animGraphInstance, AnimGraphNode* state));
        MOCK_METHOD2(OnStartTransition, void(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition));
        MOCK_METHOD2(OnEndTransition, void(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition));

        MOCK_METHOD3(OnSetVisualManipulatorOffset, void(AnimGraphInstance* animGraphInstance, size_t paramIndex, const AZ::Vector3& offset));
        MOCK_METHOD4(OnInputPortsChanged, void(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue));
        MOCK_METHOD4(OnOutputPortsChanged, void(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue));
        MOCK_METHOD3(OnRenamedNode, void(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName));
        MOCK_METHOD2(OnCreatedNode, void(AnimGraph* animGraph, AnimGraphNode* node));
        MOCK_METHOD2(OnRemoveNode, void(AnimGraph* animGraph, AnimGraphNode* nodeToRemove));
        MOCK_METHOD2(OnRemovedChildNode, void(AnimGraph* animGraph, AnimGraphNode* parentNode));
        MOCK_METHOD0(OnProgressStart, void());
        MOCK_METHOD0(OnProgressEnd, void());
        MOCK_METHOD1(OnProgressText, void(const char* text));
        MOCK_METHOD1(OnProgressValue, void(float percentage));
        MOCK_METHOD1(OnSubProgressText, void(const char* text));
        MOCK_METHOD1(OnSubProgressValue, void(float percentage));
        MOCK_METHOD2(OnScaleActorData, void(Actor* actor, float scaleFactor));
        MOCK_METHOD2(OnScaleMotionData, void(Motion* motion, float scaleFactor));
    };
} // namespace EMotionFX
