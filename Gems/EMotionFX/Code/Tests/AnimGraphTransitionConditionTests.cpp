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

#include "AnimGraphTransitionConditionFixture.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/AnimGraphVector2Condition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/TwoStringEventData.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>

#include <functional>
#include <unordered_map>

namespace EMotionFX
{
    // This function controls the way that the Google Test code formats objects
    // of type AnimGraphNode*. This causes test failure messages involving
    // AnimGraphNode pointers to contain the node name instead of just the
    // pointer address.
    void PrintTo(AnimGraphNode* const object, ::std::ostream* os)
    {
        *os << object->GetName();
    }

    // Google Test does not consider inheritance when calling PrintTo(), so
    // multiple functions must be defined for subclasses
    void PrintTo(AnimGraphMotionNode* const object, ::std::ostream* os)
    {
        PrintTo(static_cast<AnimGraphNode*>(object), os);
    }

    // This is used to map from frame numbers to a list of AnimGraphNodes. Test
    // data defines the expected list of active nodes per frame using this
    // type.
    using ActiveNodesMap = std::unordered_map<int, std::vector<const char*>>;

    // This function is defined by test data to allow for modifications to the
    // anim graph at each frame. The second parameter is the current frame
    // number. It is called with a frame number of -1 as the first thing in the
    // test before the first Update() call, to allow for initial values to be
    // set.
    using FrameCallback = std::function<void(AnimGraphInstance*, int)>;

    // The TransitionConditionFixtureP fixture is parameterized
    // on this type
    template<class ConditionType>
    struct ConditionFixtureParams
    {
        using ConditionSetUpFunc = void(*)(ConditionType*);

        ConditionFixtureParams(
            const ConditionSetUpFunc& func,
            const ActiveNodesMap& activeNodesMap,
            const FrameCallback& frameCallback = [] (AnimGraphInstance*, int) {}
        ) : m_setUpFunction(func), activeNodes(activeNodesMap), callback(frameCallback)
        {
        }

        // Function to set up the condition's parameters
        const ConditionSetUpFunc m_setUpFunction;

        // List of nodes that are active on each frame
        const ActiveNodesMap activeNodes;

        const FrameCallback callback;
    };

    template<class ConditionType>
    class TransitionConditionFixtureP : public AnimGraphTransitionConditionFixture,
                                        public ::testing::WithParamInterface<ConditionFixtureParams<ConditionType>>
    {
    public:
        const float fps;
        const float updateInterval;
        const int numUpdates;

        TransitionConditionFixtureP()
            : fps(60.0f)
            , updateInterval(1.0f / fps)
            , numUpdates(static_cast<int>(3.0f * fps))
        {
        }

        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void AddNodesToAnimGraph() override
        {
            AddParameter<FloatSliderParameter>("FloatParam", 0.1f);
            AddParameter<Vector2Parameter>("Vector2Param", AZ::Vector2(0.1f, 0.1f));
            AddParameter<TagParameter>("TagParam1", false);
            AddParameter<TagParameter>("TagParam2", false);

            // Create the appropriate condition type from this test's parameters
            ConditionType* condition = aznew ConditionType();
            m_transition->AddCondition(condition);
            condition->SetAnimGraph(m_animGraph.get());

            this->GetParam().m_setUpFunction(condition);

            m_transition->SetBlendTime(0.5f);
        }

    protected:
        void RunEMotionFXUpdateLoop()
        {
            const ActiveNodesMap& activeNodes = this->GetParam().activeNodes;
            const FrameCallback& callback = this->GetParam().callback;

            // Allow tests to set starting values for parameters
            callback(m_animGraphInstance, -1);

            // Run the EMotionFX update loop for 3 seconds at 60 fps
            for (int frameNum = 0; frameNum < numUpdates; ++frameNum)
            {
                // Allow for test-data defined updates to the graph state
                callback(m_animGraphInstance, frameNum);

                if (frameNum == 0)
                {
                    // Make sure the first frame is initialized correctly
                    // The EMotion FX update is needed before we can extract
                    // the currently active nodes from it
                    // We use an update delta time of zero on the first frame,
                    // to make sure we have a valid internal state on the first
                    // frame
                    GetEMotionFX().Update(0.0f);
                }
                else
                {
                    GetEMotionFX().Update(updateInterval);
                }

                // Check the state for the current frame
                if (activeNodes.count(frameNum))
                {
                    AZStd::vector<AnimGraphNode*> expectedActiveNodes;
                    for (const char* name : activeNodes.at(frameNum))
                    {
                        expectedActiveNodes.push_back(m_animGraph->RecursiveFindNodeByName(name));
                    }

                    const AZStd::vector<AnimGraphNode*>& gotActiveNodes = m_stateMachine->GetActiveStates(m_animGraphInstance);

                    EXPECT_EQ(gotActiveNodes, expectedActiveNodes) << "on frame " << frameNum << ", time " << frameNum * updateInterval;
                }
            }
            {
                // Ensure that we reached the target state after 3 seconds
                const AZStd::vector<AnimGraphNode*>& activeStates = m_stateMachine->GetActiveStates(m_animGraphInstance);
                const size_t numActiveStates = activeStates.size();

                EXPECT_EQ(numActiveStates, 1) << numActiveStates << " active state detected. Only one state should be active.";

                if (numActiveStates > 0)
                {
                    EXPECT_EQ(activeStates[0], m_motionNodeB) << "MotionNode1 is not the single active node";
                }
            }
        }
    };

    class StateConditionFixture : public TransitionConditionFixtureP<AnimGraphStateCondition>
    {
    public:
        // The base class TransitionConditionFixtureP just sets up a simple
        // anim graph with two motion nodes and a transition between them. This
        // graph is not sufficient to test the state condition, as there are no
        // states available to test against. This fixture creates a slightly
        // more complex graph.
        void AddNodesToAnimGraph() override
        {
            //                       +-------------------+
            //                       | childStateMachine |
            //                       +-------------------+
            //        0.5s time     ^                     \     state condition defined
            //        condition--->o                       o<---by test data
            //  0.5s blend time-->/                         v<--0.5s blend time
            //+-------------------+                         +-------------------+
            //|testSkeletalMotion0|------------------------>|testSkeletalMotion1|
            //+-------------------+           ^             +-------------------+
            //                          transition with
            //                            no condition
            //
            // -------------------+----------------------------------------------
            // Child State Machine|          1.0 sec time
            // -------------------+            condition
            //               +---------------+    v     +----------+
            // entry state-->|ChildMotionNode|----o---->|Exit state|
            //               +---------------+ ^        +----------+
            //                                 transitions to exit states have
            //                                 a blend time of 0

            AddParameter<FloatSliderParameter>("FloatParam", 0.1f);
            AddParameter<Vector2Parameter>("Vector2Param", AZ::Vector2(0.1f, 0.1f));

            // Create another state machine inside the top-level one
            AnimGraphMotionNode* childMotionNode = aznew AnimGraphMotionNode();
            childMotionNode->SetName("ChildMotionNode");
            childMotionNode->AddMotionId("testSkeletalMotion0");

            AnimGraphExitNode* childExitNode = aznew AnimGraphExitNode();
            childExitNode->SetName("ChildExitNode");

            AnimGraphTimeCondition* motionToExitCondition = aznew AnimGraphTimeCondition();
            motionToExitCondition->SetCountDownTime(1.0f);

            AnimGraphStateTransition* motionToExitTransition = aznew AnimGraphStateTransition();
            motionToExitTransition->SetSourceNode(childMotionNode);
            motionToExitTransition->SetTargetNode(childExitNode);
            motionToExitTransition->SetBlendTime(0.0f);
            motionToExitTransition->AddCondition(motionToExitCondition);

            mChildState = aznew AnimGraphStateMachine();
            mChildState->SetName("ChildStateMachine");
            mChildState->AddChildNode(childMotionNode);
            mChildState->AddChildNode(childExitNode);
            mChildState->SetEntryState(childMotionNode);
            mChildState->AddTransition(motionToExitTransition);

            AnimGraphTimeCondition* motion0ToChildStateCondition = aznew AnimGraphTimeCondition();
            motion0ToChildStateCondition->SetCountDownTime(0.5f);

            AnimGraphStateTransition* motion0ToChildStateTransition = aznew AnimGraphStateTransition();
            motion0ToChildStateTransition->SetSourceNode(m_motionNodeA);
            motion0ToChildStateTransition->SetTargetNode(mChildState);
            motion0ToChildStateTransition->SetBlendTime(0.5f);
            motion0ToChildStateTransition->AddCondition(motion0ToChildStateCondition);

            AnimGraphStateTransition* childStateToMotion1Transition = aznew AnimGraphStateTransition();
            childStateToMotion1Transition->SetSourceNode(mChildState);
            childStateToMotion1Transition->SetTargetNode(m_motionNodeB);
            childStateToMotion1Transition->SetBlendTime(0.5f);

            m_stateMachine->AddChildNode(mChildState);
            m_stateMachine->AddTransition(motion0ToChildStateTransition);
            m_stateMachine->AddTransition(childStateToMotion1Transition);

            // Create the appropriate condition type from this test's
            // parameters
            AnimGraphStateCondition* condition = aznew AnimGraphStateCondition();
            condition->SetAnimGraph(m_animGraph.get());
            childStateToMotion1Transition->AddCondition(condition);

            this->GetParam().m_setUpFunction(condition);
        }

    protected:
        AnimGraphStateMachine* mChildState;
    };

    class RangedMotionEventConditionFixture
        : public TransitionConditionFixtureP<AnimGraphMotionCondition>
    {
    public:
        void AddNodesToAnimGraph() override
        {
            this->TransitionConditionFixtureP<AnimGraphMotionCondition>::AddNodesToAnimGraph();

            AnimGraphMotionCondition* rangeMotionCondition = aznew AnimGraphMotionCondition();
            rangeMotionCondition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_EVENT);
            rangeMotionCondition->SetMotionNodeId(m_motionNodeA->GetId());
            const AZStd::shared_ptr<const EventData> eventData = GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestRangeEvent", "TestParameter");
            rangeMotionCondition->SetEventDatas({eventData});

            this->m_transition->AddCondition(rangeMotionCondition);
            rangeMotionCondition->SetAnimGraph(m_animGraph.get());
        }
    };

    // The test data changes various parameters of the conditions being tested,
    // but they frequently result in the anim graph changing in an identical
    // manner (such as moving from testSkeletalMotionNode0 to
    // testSkeletalMotionNode1 at the same point in time). These following
    // functions centralize some of the expected behavior.
    template<class AttributeType>
    void ChangeParamTo(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, const char* paramName, void (*changeFunc)(AttributeType* parameter))
    {
        if (currentFrame == testFrame)
        {
            AttributeType* parameter = static_cast<AttributeType*>(animGraphInstance->FindParameter(paramName));
            changeFunc(parameter);
        }
    }


    void ChangeVector2ParamSpecial(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, const AZ::Vector2& frame30Value, const AZ::Vector2& otherValue)
    {
        if (currentFrame != testFrame)
        {
            MCore::AttributeVector2* parameter = static_cast<MCore::AttributeVector2*>(animGraphInstance->FindParameter("Vector2Param"));
            parameter->SetValue(otherValue);
        }
        else
        {
            MCore::AttributeVector2* parameter = static_cast<MCore::AttributeVector2*>(animGraphInstance->FindParameter("Vector2Param"));
            parameter->SetValue(frame30Value);
        }
    }

    void ChangeNodeAttribute(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, void(*changeFunc)(AnimGraphMotionNode*))
    {
        if (currentFrame == testFrame)
        {
            const AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            AnimGraphMotionNode* node = static_cast<AnimGraphMotionNode*>(animGraph->RecursiveFindNodeByName("testSkeletalMotion0"));
            AZ_Assert(node, "There is no node named 'testSkeletalMotion0'");

            changeFunc(node);
            node->InvalidateUniqueData(animGraphInstance);
        }
    }

    static const auto ChangeNodeToLooping = std::bind(
        ChangeNodeAttribute,
        std::placeholders::_1,
        std::placeholders::_2,
        -1,
        [](AnimGraphMotionNode* node) { node->SetLoop(true); }
    );

    static const auto ChangeNodeToNonLooping = std::bind(
        ChangeNodeAttribute,
        std::placeholders::_1,
        std::placeholders::_2,
        -1,
        [](AnimGraphMotionNode* node) { node->SetLoop(false); }
    );

    static const auto ChangeFloatParamToPointFiveOnFrameThirty = std::bind(
        ChangeParamTo<MCore::AttributeFloat>,
        std::placeholders::_1,
        std::placeholders::_2,
        30,
        "FloatParam",
        [](MCore::AttributeFloat* parameter) { parameter->SetValue(0.5f); }
    );

    static const auto ChangeFloatParamToNegativePointFiveOnFrameThirty = std::bind(
        ChangeParamTo<MCore::AttributeFloat>,
        std::placeholders::_1,
        std::placeholders::_2,
        30,
        "FloatParam",
        [](MCore::AttributeFloat* parameter) { parameter->SetValue(-0.5f); }
    );

    static const auto ChangeVector2Param = std::bind(
        ChangeParamTo<MCore::AttributeVector2>,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        "Vector2Param",
        std::placeholders::_4
    );

    static const ActiveNodesMap moveToMotion1AtFrameThirty
    {
        {0, {"testSkeletalMotion0"}},
        {29, {"testSkeletalMotion0"}},
        {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
        {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
        {60, {"testSkeletalMotion1"}}
    };

    // Remember that the test runs the update loop at 60 fps. All the frame
    // numbers in the ActiveNodesPerFrameMaps are based on this value.
    // testSkeletalMotion0 is exactly 1 second long.
    static const std::vector<ConditionFixtureParams<AnimGraphMotionCondition>> motionTransitionConditionData
    {
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_EVENT);
                condition->SetEventDatas({GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", "TestParameter")});
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {44, {"testSkeletalMotion0"}},
                {45, {"testSkeletalMotion0", "testSkeletalMotion1"}},   // The event gets triggered on frame 44, but the condition only will only be reevaluated the next frame, so we have one frame delay.
                {46, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {74, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {75, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_HASENDED);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_HASREACHEDMAXNUMLOOPS);
                condition->SetNumLoops(1);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0"}},  // Motion will not have reached 1.0 as playtime yet, because it lags a frame behind. The actual time value gets updated in PostUpdate which is after the evaluation of the condition.
                {61, {"testSkeletalMotion0"}},  // Motion will be at 1.0 play time exactly, the loop is not detected yet.
                {62, {"testSkeletalMotion0", "testSkeletalMotion1"}},   // The loop has been detected
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {91, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {92, {"testSkeletalMotion1"}}
            },
            ChangeNodeToLooping
        },
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_PLAYTIME);
                condition->SetPlayTime(0.2f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {11, {"testSkeletalMotion0"}},
                {12, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {41, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {42, {"testSkeletalMotion1"}}
            },
            ChangeNodeToNonLooping
        },
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_PLAYTIMELEFT);
                condition->SetPlayTime(0.2f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {47, {"testSkeletalMotion0"}},
                {48, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {77, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {78, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_ISMOTIONASSIGNED);
                condition->SetPlayTime(0.2f);
            },
            ActiveNodesMap {
                // This condition will always evaluate to true. The transition
                // will start immediately.
                {0, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {29, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {30, {"testSkeletalMotion1"}}
            }
        }
        // TODO AnimGraphMotionCondition::FUNCTION_ISMOTIONNOTASSIGNED
    };

    static const std::vector<ConditionFixtureParams<AnimGraphMotionCondition>> rangedMotionTransitionConditionData
    {
        {
            [](AnimGraphMotionCondition* condition) {
                condition->SetMotionNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphMotionCondition::FUNCTION_EVENT);
                condition->SetEventDatas({GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", "TestParameter")});
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {44, {"testSkeletalMotion0"}},
                {45, {"testSkeletalMotion0", "testSkeletalMotion1"}},   // The event gets triggered on frame 44, but the condition only will only be reevaluated the next frame, so we have one frame delay.
                {46, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {74, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {75, {"testSkeletalMotion1"}}
            }
        },
    };

    static const std::vector<ConditionFixtureParams<AnimGraphParameterCondition>> parameterTransitionConditionData
    {
        // FUNCTION_EQUAL tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_EQUAL);
                condition->SetTestValue(0.1f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {29, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {30, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_EQUAL);
                condition->SetTestValue(0.5f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_NOTEQUAL tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTEQUAL);
                condition->SetTestValue(0.1f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_INRANGE tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_INRANGE);
                condition->SetTestValue(0.4f);
                condition->SetRangeValue(0.6f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_NOTINRANGE tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTINRANGE);
                condition->SetTestValue(-0.2f);
                condition->SetRangeValue(0.2f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_LESS tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESS);
                condition->SetTestValue(0.0f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToNegativePointFiveOnFrameThirty
        },

        // FUNCTION_GREATER tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATER);
                condition->SetTestValue(0.1f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_GREATEREQUAL tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.5f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.49f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        },

        // FUNCTION_LESSEQUAL tests
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(-0.1f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToNegativePointFiveOnFrameThirty
        },

        // Time requirement test
        {
            [](AnimGraphParameterCondition* condition) {
                condition->SetParameterName("FloatParam");
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATER);
                condition->SetTimeRequirement(0.5f);
                condition->SetTestValue(0.1f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {88, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {89, {"testSkeletalMotion1"}}
            },
            ChangeFloatParamToPointFiveOnFrameThirty
        }
    };

    static const std::vector<ConditionFixtureParams<AnimGraphPlayTimeCondition>> playTimeTransitionConditionData
    {
        {
            [](AnimGraphPlayTimeCondition* condition) {
                condition->SetNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetMode(AnimGraphPlayTimeCondition::MODE_REACHEDEND);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphPlayTimeCondition* condition) {
                condition->SetNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetMode(AnimGraphPlayTimeCondition::MODE_REACHEDTIME);
                condition->SetPlayTime(0.3f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {17, {"testSkeletalMotion0"}},
                {18, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {47, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {48, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphPlayTimeCondition* condition) {
                condition->SetNodeId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetMode(AnimGraphPlayTimeCondition::MODE_HASLESSTHAN);
                condition->SetPlayTime(0.3f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {41, {"testSkeletalMotion0"}},
                {42, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {71, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {72, {"testSkeletalMotion1"}}
            }
        }
    };

    static const std::vector<ConditionFixtureParams<AnimGraphStateCondition>> stateTransitionConditionData
    {
        {
            [](AnimGraphStateCondition* condition) {
                condition->SetStateId(condition->GetAnimGraph()->RecursiveFindNodeByName("ChildStateMachine")->GetId());
                condition->SetTestFunction(AnimGraphStateCondition::FUNCTION_EXITSTATES);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                {60, {"ChildStateMachine"}},
                {89, {"ChildStateMachine"}},
                {90, {"ChildStateMachine", "testSkeletalMotion1"}},
                {119, {"ChildStateMachine", "testSkeletalMotion1"}},
                {120, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphStateCondition* condition) {
                condition->SetStateId(condition->GetAnimGraph()->RecursiveFindNodeByName("ChildMotionNode")->GetId());
                condition->SetTestFunction(AnimGraphStateCondition::FUNCTION_END);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Before the state machine defer update changes.
                //{60, {"ChildStateMachine"}},
                //{89, {"ChildStateMachine"}},
                //{90, {"ChildStateMachine", "testSkeletalMotion1"}},
                //{119, {"ChildStateMachine", "testSkeletalMotion1"}},
                // After the state machine defer update changes.
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
#else
                {61, {"ChildStateMachine", "testSkeletalMotion1"}},
#endif
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {90, {"testSkeletalMotion1"}},
#else
                {91, {"testSkeletalMotion1"}},
#endif
                {119, {"testSkeletalMotion1"}},
                {120, {"testSkeletalMotion1"}}
            }
        },
        {
            [](AnimGraphStateCondition* condition) {
                condition->SetStateId(condition->GetAnimGraph()->RecursiveFindNodeByName("ChildStateMachine")->GetId());
                condition->SetTestFunction(AnimGraphStateCondition::FUNCTION_ENTERING);
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
#else
                {61, {"ChildStateMachine", "testSkeletalMotion1"}},
#endif
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {90, {"testSkeletalMotion1"}}
#else
                {91, {"testSkeletalMotion1"}}
#endif
            }
        },
        {
            [](AnimGraphStateCondition* condition) {
                condition->SetStateId(condition->GetAnimGraph()->RecursiveFindNodeByName("ChildStateMachine")->GetId());
                condition->SetTestFunction(AnimGraphStateCondition::FUNCTION_ENTER);
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
#else
                {61, {"ChildStateMachine", "testSkeletalMotion1"}},
#endif
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {90, {"testSkeletalMotion1"}}
#else
                {91, {"testSkeletalMotion1"}}
#endif
            }
        },
        {
            [](AnimGraphStateCondition* condition) {
                condition->SetStateId(condition->GetAnimGraph()->RecursiveFindNodeByName("testSkeletalMotion0")->GetId());
                condition->SetTestFunction(AnimGraphStateCondition::FUNCTION_END);
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
#else
                {61, {"ChildStateMachine", "testSkeletalMotion1"}},
#endif
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
                {90, {"testSkeletalMotion1"}}
#else
                {91, {"testSkeletalMotion1"}}
#endif
            }
        }
    };

    static const std::vector<ConditionFixtureParams<AnimGraphTagCondition>> tagTransitionConditionData
    {
        {
            [](AnimGraphTagCondition* condition) {
                condition->SetFunction(AnimGraphTagCondition::FUNCTION_ALL);
                condition->SetTags({"TagParam1", "TagParam2"});
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(true); });
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 15, "TagParam2", [](MCore::AttributeBool* param) { param->SetValue(true); });
            }
        },
        {
            [](AnimGraphTagCondition* condition) {
                condition->SetFunction(AnimGraphTagCondition::FUNCTION_NOTALL);
                condition->SetTags({"TagParam1", "TagParam2"});
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                // initialize tags to on
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(true); });
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, "TagParam2", [](MCore::AttributeBool* param) { param->SetValue(true); });

                // turn TagParam1 off on frame 30
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(false); });
            }
        },
        {
            [](AnimGraphTagCondition* condition) {
                condition->SetFunction(AnimGraphTagCondition::FUNCTION_NONE);
                condition->SetTags({"TagParam1", "TagParam2"});
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                // initialize tags to on
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(true); });
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, "TagParam2", [](MCore::AttributeBool* param) { param->SetValue(true); });

                // turn TagParam2 off on frame 15
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 15, "TagParam2", [](MCore::AttributeBool* param) { param->SetValue(false); });

                // turn TagParam1 off on frame 30
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(false); });
            }
        },
        {
            [](AnimGraphTagCondition* condition) {
                condition->SetFunction(AnimGraphTagCondition::FUNCTION_ONEORMORE);
                condition->SetTags({"TagParam1", "TagParam2"});
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                ChangeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, "TagParam1", [](MCore::AttributeBool* param) { param->SetValue(true); });
            }
        }
    };

    static const std::vector<ConditionFixtureParams<AnimGraphTimeCondition>> timeTransitionConditionData
    {
        {
            [](AnimGraphTimeCondition* condition) {
                condition->SetCountDownTime(1.3f);
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {77, {"testSkeletalMotion0"}},
                {78, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {107, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {108, {"testSkeletalMotion1"}}
            }
        }
    };

    static const std::vector<ConditionFixtureParams<AnimGraphVector2Condition>> vector2TransitionConditionData
    {
        // --------------------------------------------------------------------
        // FUNCTION_EQUAL
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_EQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.5f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_EQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.5f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_EQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(std::sqrt(0.25f / 2.0f), std::sqrt(0.25f / 2.0f))); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_NOTEQUAL
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTEQUAL);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.5f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTEQUAL);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.5f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTEQUAL);
                condition->SetTestValue(std::sqrt(0.1f*0.1f + 0.1f*0.1f));
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(std::sqrt(0.25f / 2.0f), std::sqrt(0.25f / 2.0f))); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_LESS
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESS);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.05f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESS);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.05f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESS);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.05f, 0.05f)); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_GREATER
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATER);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.15f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATER);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.15f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATER);
                condition->SetTestValue(0.2f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.25f, 0.0f)); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_GREATEREQUAL
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.2f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.2f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.2f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.3f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.2f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.2f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.2f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.3f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(1.0f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.5f, 0.0f)); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_LESSEQUAL
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.05f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.5f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.0f, 0.05f), AZ::Vector2(1.0f, 1.0f))
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.0f, 0.5f), AZ::Vector2(1.0f, 1.0f))
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.1f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.05f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_LESSEQUAL);
                condition->SetTestValue(0.5f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.5f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        },

        // --------------------------------------------------------------------
        // FUNCTION_INRANGE
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_INRANGE);
                condition->SetTestValue(0.2f);
                condition->SetRangeValue(0.3f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.25f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_INRANGE);
                condition->SetTestValue(0.2f);
                condition->SetRangeValue(0.3f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.25f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_INRANGE);
                condition->SetTestValue(0.2f);
                condition->SetRangeValue(0.3f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.15f, 0.15f)); })
        },

        // --------------------------------------------------------------------
        // FUNCTION_NOTINRANGE
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETX);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTINRANGE);
                condition->SetTestValue(0.05f);
                condition->SetRangeValue(0.15f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.25f, 0.0f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_GETY);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTINRANGE);
                condition->SetTestValue(0.05f);
                condition->SetRangeValue(0.15f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.0f, 0.25f)); })
        },
        {
            [](AnimGraphVector2Condition* condition) {
                condition->SetParameterName("Vector2Param");
                condition->SetOperation(AnimGraphVector2Condition::OPERATION_LENGTH);
                condition->SetFunction(AnimGraphParameterCondition::FUNCTION_NOTINRANGE);
                condition->SetTestValue(0.05f);
                condition->SetRangeValue(0.15f);
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, [](MCore::AttributeVector2* vec2) { vec2->SetValue(AZ::Vector2(0.15f, 0.15f)); })
        },
    };

    using MotionConditionFixture = TransitionConditionFixtureP<AnimGraphMotionCondition>;
    TEST_P(MotionConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestMotionCondition, MotionConditionFixture,
        ::testing::ValuesIn(motionTransitionConditionData)
    );

    TEST_P(RangedMotionEventConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestRangedMotionCondition, RangedMotionEventConditionFixture,
        ::testing::ValuesIn(rangedMotionTransitionConditionData)
    );

    using ParameterConditionFixture = TransitionConditionFixtureP<AnimGraphParameterCondition>;
    TEST_P(ParameterConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestParameterCondition, ParameterConditionFixture,
        ::testing::ValuesIn(parameterTransitionConditionData)
    );

    using PlayTimeConditionFixture = TransitionConditionFixtureP<AnimGraphPlayTimeCondition>;
    TEST_P(PlayTimeConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestPlayTimeCondition, PlayTimeConditionFixture,
        ::testing::ValuesIn(playTimeTransitionConditionData)
    );

    TEST_P(StateConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestStateCondition, StateConditionFixture,
        ::testing::ValuesIn(stateTransitionConditionData)
    );

    using TagConditionFixture = TransitionConditionFixtureP<AnimGraphTagCondition>;
    TEST_P(TagConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestTagCondition, TagConditionFixture,
        ::testing::ValuesIn(tagTransitionConditionData)
    );


    using TimeConditionFixture = TransitionConditionFixtureP<AnimGraphTimeCondition>;
    TEST_P(TimeConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestTimeCondition, TimeConditionFixture,
        ::testing::ValuesIn(timeTransitionConditionData)
    );

    using Vector2ConditionFixture = TransitionConditionFixtureP<AnimGraphVector2Condition>;
    TEST_P(Vector2ConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestVector2Condition, Vector2ConditionFixture,
        ::testing::ValuesIn(vector2TransitionConditionData)
    );

} // end namespace EMotionFX
