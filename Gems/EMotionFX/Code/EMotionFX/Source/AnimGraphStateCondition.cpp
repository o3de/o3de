/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <AzCore/Math/MathUtils.h>

namespace EMotionFX
{
    const char* AnimGraphStateCondition::s_functionExitStateReached = "Trigger When Exit State Reached";
    const char* AnimGraphStateCondition::s_functionStartedTransitioning = "Started Transitioning Into State";
    const char* AnimGraphStateCondition::s_functionStateFullyBlendedIn = "State Fully Blended In";
    const char* AnimGraphStateCondition::s_functionLeavingState = "Leaving State, Transitioning Out";
    const char* AnimGraphStateCondition::s_functionStateFullyBlendedOut = "State Fully Blended Out";
    const char* AnimGraphStateCondition::s_functionHasReachedSpecifiedPlaytime = "Has Reached Specified Playtime";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateCondition, AnimGraphConditionAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateCondition::UniqueData, AnimGraphObjectUniqueDataAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateCondition::EventHandler, AnimGraphObjectUniqueDataAllocator)

    AnimGraphStateCondition::AnimGraphStateCondition()
        : AnimGraphTransitionCondition()
        , m_stateId(AnimGraphNodeId::InvalidId)
        , m_state(nullptr)
        , m_playTime(0.0f)
        , m_testFunction(FUNCTION_EXITSTATES)
    {
    }


    AnimGraphStateCondition::AnimGraphStateCondition(AnimGraph* animGraph)
        : AnimGraphStateCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphStateCondition::~AnimGraphStateCondition()
    {
    }


    void AnimGraphStateCondition::Reinit()
    {
        if (!AnimGraphNodeId(m_stateId).IsValid())
        {
            m_state = nullptr;
            return;
        }

        m_state = m_animGraph->RecursiveFindNodeById(m_stateId);
    }


    bool AnimGraphStateCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* AnimGraphStateCondition::GetPaletteName() const
    {
        return "State Condition";
    }
    

    // get the category
    AnimGraphObject::ECategory AnimGraphStateCondition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONCONDITIONS;
    }


    // test the condition
    bool AnimGraphStateCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // in case a event got triggered constantly fire true until the condition gets reset
        const UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        if (uniqueData->m_triggered)
        {
            return true;
        }

        // check what we want to do
        switch (m_testFunction)
        {
        // reached an exit state
        case FUNCTION_EXITSTATES:
        {
            // check if the state is a valid state machine
            if (m_state)
            {
                if (azrtti_typeid(m_state) == azrtti_typeid<AnimGraphStateMachine>())
                {
                    // type-cast the state to a state machine
                    AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(m_state);

                    // check if we have reached an exit state
                    return stateMachine->GetExitStateReached(animGraphInstance);
                }
                else if (azrtti_typeid(m_state) == azrtti_typeid<AnimGraphReferenceNode>())
                {
                    AnimGraphReferenceNode* referenceNode = static_cast<AnimGraphReferenceNode*>(m_state);
                    AnimGraph* referencedAnimGraph = referenceNode->GetReferencedAnimGraph();
                    if (referencedAnimGraph)
                    {
                        AnimGraphReferenceNode::UniqueData* referenceNodeUniqueData = static_cast<AnimGraphReferenceNode::UniqueData*>(referenceNode->FindOrCreateUniqueNodeData(animGraphInstance));
                        if (referenceNodeUniqueData && referenceNodeUniqueData->m_referencedAnimGraphInstance)
                        {
                            AnimGraphStateMachine* stateMachine = referencedAnimGraph->GetRootStateMachine();
                            return stateMachine->GetExitStateReached(referenceNodeUniqueData->m_referencedAnimGraphInstance);
                        }
                    }
                    else
                    {
                        // if the reference node does not have an anim graph, assume the exit state was reached
                        return true;
                    }
                }
            }
            break;
        }
        case FUNCTION_PLAYTIME:
        {
            // reached the specified play time
            if (m_state)
            {
                const float currentLocalTime = m_state->GetCurrentPlayTime(uniqueData->m_animGraphInstance);
                // the has reached play time condition is not part of the event handler, so we have to manually handle it here
                if (AZ::IsClose(currentLocalTime, m_playTime, AZ::Constants::FloatEpsilon) || currentLocalTime >= m_playTime)
                {
                    return true;
                }
            }
            break;
        }
        }

        // no event got triggered, continue playing the state and don't autostart the transition
        return false;
    }


    // reset the motion condition
    void AnimGraphStateCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data and reset it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->m_triggered = false;
    }

    // construct and output the information summary string for this object
    void AnimGraphStateCondition::GetSummary(AZStd::string* outResult) const
    {
        AZStd::string stateName;
        if (m_state)
        {
            stateName = m_state->GetNameString();
        }

        *outResult = AZStd::string::format("%s: State='%s', Test Function='%s'", RTTI_GetTypeName(), stateName.c_str(), GetTestFunctionString());
    }


    // construct and output the tooltip for this object
    void AnimGraphStateCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"100\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // add the state name
        columnName = "State Name: ";
        if (m_state)
        {
            columnValue = m_state->GetNameString();
        }
        else
        {
            columnValue.clear();
        }
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // add the test function
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td width=\"180\">%s</td></tr></table>", columnName.c_str(), columnValue.c_str());
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphStateCondition::UniqueData
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphStateCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
        , m_animGraphInstance(animGraphInstance)
    {
        m_eventHandler       = nullptr;
        m_triggered          = false;
        CreateEventHandler();
    }


    // destructor
    AnimGraphStateCondition::UniqueData::~UniqueData()
    {
        DeleteEventHandler();
    }


    void AnimGraphStateCondition::UniqueData::CreateEventHandler()
    {
        DeleteEventHandler();

        if (m_animGraphInstance)
        {
            m_eventHandler = aznew AnimGraphStateCondition::EventHandler(static_cast<AnimGraphStateCondition*>(m_object), this);
            m_animGraphInstance->AddEventHandler(m_eventHandler);
        }
    }


    void AnimGraphStateCondition::UniqueData::DeleteEventHandler()
    {
        if (m_eventHandler)
        {
            m_animGraphInstance->RemoveEventHandler(m_eventHandler);

            delete m_eventHandler;
            m_eventHandler = nullptr;
        }
    }


    // callback that gets called before a node gets removed
    void AnimGraphStateCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (m_stateId == nodeToRemove->GetId())
        {
            SetStateId(AnimGraphNodeId::InvalidId);
        }
    }


    //--------------------------------------------------------------------------------
    // class AnimGraphStateCondition::EventHandler
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphStateCondition::EventHandler::EventHandler(AnimGraphStateCondition* condition, UniqueData* uniqueData)
        : EMotionFX::AnimGraphInstanceEventHandler()
    {
        m_condition      = condition;
        m_uniqueData     = uniqueData;
    }


    // destructor
    AnimGraphStateCondition::EventHandler::~EventHandler()
    {
    }


    bool AnimGraphStateCondition::EventHandler::IsTargetState(const AnimGraphNode* state) const
    {
        const AnimGraphNode* conditionState = m_condition->GetState();
        if (conditionState)
        {
            const AZStd::string& stateName = conditionState->GetNameString();
            return (stateName.empty() || stateName == state->GetName());
        }

        return false;
    }


    void AnimGraphStateCondition::EventHandler::OnStateChange(AnimGraphInstance* animGraphInstance, AnimGraphNode* state, TestFunction targetFunction)
    {
        // check if the state and the anim graph instance are valid and return directly in case one of them is not
        if (!state || !animGraphInstance)
        {
            return;
        }

        const TestFunction testFunction = m_condition->GetTestFunction();
        if (testFunction == targetFunction && IsTargetState(state))
        {
            m_uniqueData->m_triggered = true;
        }
    }

    const AZStd::vector<EventTypes> AnimGraphStateCondition::EventHandler::GetHandledEventTypes() const
    {
        return {
            EVENT_TYPE_ON_STATE_ENTER,
            EVENT_TYPE_ON_STATE_ENTERING, 
            EVENT_TYPE_ON_STATE_EXIT,
            EVENT_TYPE_ON_STATE_END
        };
    }

    void AnimGraphStateCondition::EventHandler::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        OnStateChange(animGraphInstance, state, FUNCTION_ENTER);
    }


    void AnimGraphStateCondition::EventHandler::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        OnStateChange(animGraphInstance, state, FUNCTION_ENTERING);
    }


    void AnimGraphStateCondition::EventHandler::OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        OnStateChange(animGraphInstance, state, FUNCTION_EXIT);
    }


    void AnimGraphStateCondition::EventHandler::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        OnStateChange(animGraphInstance, state, FUNCTION_END);
    }


    void AnimGraphStateCondition::SetStateId(AnimGraphNodeId stateId)
    {
        m_stateId = stateId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AnimGraphNodeId AnimGraphStateCondition::GetStateId() const
    {
        return m_stateId;
    }


    AnimGraphNode* AnimGraphStateCondition::GetState() const
    {
        return m_state;
    }


    void AnimGraphStateCondition::SetPlayTime(float playTime)
    {
        m_playTime = playTime;
    }


    float AnimGraphStateCondition::GetPlayTime() const
    {
        return m_playTime;
    }


    void AnimGraphStateCondition::SetTestFunction(TestFunction testFunction)
    {
        m_testFunction = testFunction;
    }


    AnimGraphStateCondition::TestFunction AnimGraphStateCondition::GetTestFunction() const
    {
        return m_testFunction;
    }


    const char* AnimGraphStateCondition::GetTestFunctionString() const
    {
        switch (m_testFunction)
        {
            case FUNCTION_EXITSTATES:
            {
                return s_functionExitStateReached;
            }
            case FUNCTION_ENTERING:
            {
                return s_functionStartedTransitioning;
            }
            case FUNCTION_ENTER:
            {
                return s_functionStateFullyBlendedIn;
            }
            case FUNCTION_EXIT:
            {
                return s_functionLeavingState;
            }
            case FUNCTION_END:
            {
                return s_functionStateFullyBlendedOut;
            }
            case FUNCTION_PLAYTIME:
            {
                return s_functionHasReachedSpecifiedPlaytime;
            }
            default:
            {
                return "Unknown test function";
            }
        }
    }


    AZ::Crc32 AnimGraphStateCondition::GetTestFunctionVisibility() const
    {
        return AnimGraphNodeId(m_stateId).IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphStateCondition::GetPlayTimeVisibility() const
    {
        if (GetTestFunctionVisibility() == AZ::Edit::PropertyVisibility::Hide || m_testFunction != FUNCTION_PLAYTIME)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    void AnimGraphStateCondition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(m_stateId);
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-stateId %llu", itConvertedIds->second);
        }
    }


    void AnimGraphStateCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphStateCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("stateId", &AnimGraphStateCondition::m_stateId)
            ->Field("testFunction", &AnimGraphStateCondition::m_testFunction)
            ->Field("playTime", &AnimGraphStateCondition::m_playTime)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphStateCondition>("State Condition", "State condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphStateId"), &AnimGraphStateCondition::m_stateId, "State", "The state to watch.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphStateCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphStateCondition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateCondition::m_testFunction, "Test Function", "The type of test function or condition.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateCondition::GetTestFunctionVisibility)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(FUNCTION_EXITSTATES,s_functionExitStateReached)
                ->EnumAttribute(FUNCTION_ENTERING,  s_functionStartedTransitioning)
                ->EnumAttribute(FUNCTION_ENTER,     s_functionStateFullyBlendedIn)
                ->EnumAttribute(FUNCTION_EXIT,      s_functionLeavingState)
                ->EnumAttribute(FUNCTION_END,       s_functionStateFullyBlendedOut)
                ->EnumAttribute(FUNCTION_PLAYTIME,  s_functionHasReachedSpecifiedPlaytime)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateCondition::m_playTime, "Play Time", "The play time in seconds.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateCondition::GetPlayTimeVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
