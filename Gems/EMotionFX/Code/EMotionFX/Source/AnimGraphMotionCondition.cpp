/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/TwoStringEventData.h>

namespace EMotionFX
{
    const char* AnimGraphMotionCondition::s_functionMotionEvent = "Motion Event";
    const char* AnimGraphMotionCondition::s_functionHasEnded = "Has Ended";
    const char* AnimGraphMotionCondition::s_functionHasReachedMaxNumLoops = "Has Reached Max Num Loops";
    const char* AnimGraphMotionCondition::s_functionHasReachedPlayTime = "Has Reached Specified Play Time";
    const char* AnimGraphMotionCondition::s_functionHasLessThan = "Has Less Play Time Left";
    const char* AnimGraphMotionCondition::s_functionIsMotionAssigned = "Is Motion Assigned?";
    const char* AnimGraphMotionCondition::s_functionIsMotionNotAssigned = "Is Motion Not Assigned?";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionCondition, AnimGraphConditionAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionCondition::UniqueData, AnimGraphObjectUniqueDataAllocator)

    AnimGraphMotionCondition::AnimGraphMotionCondition()
        : AnimGraphTransitionCondition()
        , m_motionNodeId(AnimGraphNodeId::InvalidId)
        , m_motionNode(nullptr)
        , m_numLoops(1)
        , m_playTime(0.0f)
        , m_testFunction(FUNCTION_HASENDED)
    {
    }


    AnimGraphMotionCondition::AnimGraphMotionCondition(AnimGraph* animGraph)
        : AnimGraphMotionCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphMotionCondition::~AnimGraphMotionCondition()
    {
    }


    void AnimGraphMotionCondition::Reinit()
    {
        if (!AnimGraphNodeId(m_motionNodeId).IsValid())
        {
            m_motionNode = nullptr;
            return;
        }

        AnimGraphNode* node = m_animGraph->RecursiveFindNodeById(m_motionNodeId);
        m_motionNode = azdynamic_cast<AnimGraphMotionNode*>(node);
    }


    bool AnimGraphMotionCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* AnimGraphMotionCondition::GetPaletteName() const
    {
        return "Motion Condition";
    }


    bool AnimGraphMotionCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // Make sure the motion node to which the motion condition is linked to is valid.
        if (!m_motionNode)
        {
            return false;
        }

        // Early condition function check pass for the 'Is motion assigned?'. We can do this before retrieving the unique data.
        if (m_testFunction == FUNCTION_ISMOTIONASSIGNED || m_testFunction == FUNCTION_ISMOTIONNOTASSIGNED)
        {
            const EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
            if (!motionSet)
            {
                return false;
            }

            // Iterate over motion entries from the motion node and check if there are motions assigned to them.
            const size_t numMotions = m_motionNode->GetNumMotions();
            for (size_t i = 0; i < numMotions; ++i)
            {
                const char* motionId = m_motionNode->GetMotionId(i);
                const EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(motionId);

                if (m_testFunction == FUNCTION_ISMOTIONASSIGNED)
                {
                    // Any unassigned motion entry will make the condition to fail.
                    if (!motionEntry || (motionEntry && motionEntry->GetFilenameString().empty()))
                    {
                        return false;
                    }
                }
                else
                {
                    if (motionEntry && !motionEntry->GetFilenameString().empty())
                    {
                        // Any assigned motion entry will make the condition to fail.
                        return false;
                    }
                }
            }

            return true;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        MotionInstance* motionInstance = m_motionNode->FindMotionInstance(animGraphInstance);
        if (!motionInstance)
        {
            return false;
        }

        // Update the unique data.
        if (uniqueData->m_motionInstance != motionInstance)
        {
            uniqueData->m_motionInstance = motionInstance;
        }

        // Process the condition depending on the function used.
        switch (m_testFunction)
        {
        case FUNCTION_EVENT:
        {
            const EMotionFX::AnimGraphEventBuffer& eventBuffer = animGraphInstance->GetEventBuffer();
            const size_t numEvents = eventBuffer.GetNumEvents();

            // Check if the triggered motion event is of the given type and parameter from the motion condition.
            for (size_t i = 0; i < numEvents; ++i)
            {
                const EMotionFX::EventInfo& eventInfo = eventBuffer.GetEvent(i);
                const EventDataSet& eventDatas = eventInfo.m_event->GetEventDatas();

                size_t matches = 0;
                for (const EventDataPtr& checkAgainstData : m_eventDatas)
                {
                    if (checkAgainstData)
                    {
                        for (const auto& emittedData : eventDatas)
                        {
                            if (checkAgainstData->Equal(*emittedData, /*ignoreEmptyFields = */true))
                            {
                                ++matches;
                            }
                        }
                    }
                }
                if (m_eventDatas.size() == matches)
                {
                    return true;
                }
            }
        }
        break;
        // Has motion finished playing?
        case FUNCTION_HASENDED:
        {
            // Special case for non looping motions only.
            if (!motionInstance->GetIsPlayingForever())
            {
                // Get the play time and the animation length.
                const float currentTime = m_motionNode->GetCurrentPlayTime(animGraphInstance);
                const float maxTime     = motionInstance->GetDuration();

                // Differentiate between the play modes.
                const EPlayMode playMode = motionInstance->GetPlayMode();
                if (playMode == PLAYMODE_FORWARD)
                {
                    // Return true in case the current playtime has reached the animation end.
                    return currentTime >= maxTime - MCore::Math::epsilon;
                }
                else if (playMode == PLAYMODE_BACKWARD)
                {
                    // Return true in case the current playtime has reached the animation start.
                    return currentTime <= MCore::Math::epsilon;
                }
            }
            else
            {
                return motionInstance->GetHasLooped();
            }
        }
        break;

        // Less than a given amount of play time left.
        case FUNCTION_PLAYTIMELEFT:
        {
            const float timeLeft = motionInstance->GetDuration() - m_motionNode->GetCurrentPlayTime(animGraphInstance);
            return timeLeft <= m_playTime || AZ::IsClose(timeLeft, m_playTime, 0.0001f);
        }
        break;

        // Maximum number of loops.
        case FUNCTION_HASREACHEDMAXNUMLOOPS:
        {
            return motionInstance->GetNumCurrentLoops() >= m_numLoops;
        }
        break;

        // Reached the specified play time.
        // The has reached play time condition is not part of the event handler, so we have to manually handle it here.
        case FUNCTION_PLAYTIME:
        {
            return m_motionNode->GetCurrentPlayTime(animGraphInstance) >= (m_playTime - AZ::Constants::FloatEpsilon);
        }
        break;

        case FUNCTION_NONE:
        default:
        break;
        }

        // No event got triggered, continue playing the state and don't autostart the transition.
        return false;
    }

    void AnimGraphMotionCondition::SetTestFunction(TestFunction testFunction)
    {
        m_testFunction = testFunction;
    }


    AnimGraphMotionCondition::TestFunction AnimGraphMotionCondition::GetTestFunction() const
    {
        return m_testFunction;
    }


    const char* AnimGraphMotionCondition::GetTestFunctionString() const
    {
        switch (m_testFunction)
        {
            case FUNCTION_EVENT:
            {
                return s_functionMotionEvent;
            }
            case FUNCTION_HASENDED:
            {
                return s_functionHasEnded;
            }
            case FUNCTION_HASREACHEDMAXNUMLOOPS:
            {
                return s_functionHasReachedMaxNumLoops;
            }
            case FUNCTION_PLAYTIME:
            {
                return s_functionHasReachedPlayTime;
            }
            case FUNCTION_PLAYTIMELEFT:
            {
                return s_functionHasLessThan;
            }
            case FUNCTION_ISMOTIONASSIGNED:
            {
                return s_functionIsMotionAssigned;
            }
            case FUNCTION_ISMOTIONNOTASSIGNED:
            {
                return s_functionIsMotionNotAssigned;
            }
            default:
            {
                return "None";
            }
        }
    }


    void AnimGraphMotionCondition::SetEventDatas(EventDataSet&& eventData)
    {
        m_eventDatas = AZStd::move(eventData);
    }


    const EventDataSet& AnimGraphMotionCondition::GetEventDatas() const
    {
        return m_eventDatas;
    }


    void AnimGraphMotionCondition::SetMotionNodeId(AnimGraphNodeId motionNodeId)
    {
        m_motionNodeId = motionNodeId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AnimGraphNodeId AnimGraphMotionCondition::GetMotionNodeId() const
    {
        return m_motionNodeId;
    }


    AnimGraphNode* AnimGraphMotionCondition::GetMotionNode() const
    {
        return m_motionNode;
    }



    void AnimGraphMotionCondition::SetNumLoops(AZ::u32 numLoops)
    {
        m_numLoops = numLoops;
    }


    AZ::u32 AnimGraphMotionCondition::GetNumLoops() const
    {
        return m_numLoops;
    }



    void AnimGraphMotionCondition::SetPlayTime(float playTime)
    {
        m_playTime = playTime;
    }


    float AnimGraphMotionCondition::GetPlayTime() const
    {
        return m_playTime;
    }


    void AnimGraphMotionCondition::GetSummary(AZStd::string* outResult) const
    {
        AZStd::string nodeName;
        if (m_motionNode)
        {
            nodeName = m_motionNode->GetNameString();
        }

        *outResult = AZStd::string::format("%s: Motion Node Name='%s', Test Function='%s'", RTTI_GetTypeName(), nodeName.c_str(), GetTestFunctionString());
    }


    void AnimGraphMotionCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName;
        AZStd::string columnValue;

        // Add the condition type.
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"130\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the motion node name.
        AZStd::string nodeName;
        if (m_motionNode)
        {
            nodeName = m_motionNode->GetNameString();
        }

        columnName = "Motion Node Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), nodeName.c_str());

        // Add the test function.
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td></tr></table>", columnName.c_str(), columnValue.c_str());
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphMotionCondition::UniqueData
    //--------------------------------------------------------------------------------
    AnimGraphMotionCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, MotionInstance* motionInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        m_motionInstance = motionInstance;
    }


    // Callback that gets called before a node gets removed.
    void AnimGraphMotionCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (m_motionNodeId == nodeToRemove->GetId())
        {
            SetMotionNodeId(AnimGraphNodeId::InvalidId);
        }
    }

    AZ::Crc32 AnimGraphMotionCondition::GetNumLoopsVisibility() const
    {
        return m_testFunction == FUNCTION_HASREACHEDMAXNUMLOOPS ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphMotionCondition::GetPlayTimeVisibility() const
    {
        if (m_testFunction == FUNCTION_PLAYTIME || m_testFunction == FUNCTION_PLAYTIMELEFT)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }
        
        return AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphMotionCondition::GetEventPropertiesVisibility() const
    {
        return m_testFunction == FUNCTION_EVENT ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    void AnimGraphMotionCondition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(GetMotionNodeId());
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-motionNodeId %llu", itConvertedIds->second);
        }
    }


    bool AnimGraphMotionConditionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() >= 2)
        {
            return false;
        }

        AZ::GenericClassInfo* classInfo = serializeContext.FindGenericClassInfo(azrtti_typeid<AZStd::vector<AZStd::shared_ptr<const EventData>>>());
        const int elementIndex = rootElementNode.AddElement(serializeContext, "eventDatas", classInfo);
        AZ::SerializeContext::DataElementNode& eventDataSharedPtrElement = rootElementNode.GetSubElement(elementIndex);

        // Read the old eventType and eventParameter fields
        AZStd::string eventType;
        const int eventTypeIndex = rootElementNode.FindElement(AZ_CRC_CE("eventType"));
        if (eventTypeIndex < 0)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode eventTypeNode = rootElementNode.GetSubElement(eventTypeIndex);
        if (!eventTypeNode.GetData(eventType))
        {
            return false;
        }

        AZStd::string eventParameter;
        const int eventParameterIndex = rootElementNode.FindElement(AZ_CRC_CE("eventParameter"));
        if (eventParameterIndex < 0)
        {
            return false;
        }

        AZ::SerializeContext::DataElementNode eventParameterNode = rootElementNode.GetSubElement(eventParameterIndex);
        if (!eventParameterNode.GetData(eventParameter))
        {
            return false;
        }

        // Add the new data
        if (!eventDataSharedPtrElement.SetData(serializeContext, EventDataSet {AZStd::make_shared<TwoStringEventData>(eventType, eventParameter)}))
        {
            return false;
        }

        // Remove the old fields
        rootElementNode.RemoveElementByName(AZ_CRC_CE("eventType"));
        rootElementNode.RemoveElementByName(AZ_CRC_CE("eventParameter"));
        return true;
    }

    void AnimGraphMotionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphMotionCondition, AnimGraphTransitionCondition>()
            ->Version(2, &AnimGraphMotionConditionConverter)
            ->Field("motionNodeId", &AnimGraphMotionCondition::m_motionNodeId)
            ->Field("testFunction", &AnimGraphMotionCondition::m_testFunction)
            ->Field("numLoops", &AnimGraphMotionCondition::m_numLoops)
            ->Field("playTime", &AnimGraphMotionCondition::m_playTime)
            ->Field("eventDatas", &AnimGraphMotionCondition::m_eventDatas)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphMotionCondition>("Motion Condition", "Motion condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphMotionNodeId"), &AnimGraphMotionCondition::m_motionNodeId, "Motion", "The motion node to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphMotionCondition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphMotionCondition::m_testFunction, "Test Function", "The type of test function or condition.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(FUNCTION_EVENT,                 s_functionMotionEvent)
                ->EnumAttribute(FUNCTION_HASENDED,              s_functionHasEnded)
                ->EnumAttribute(FUNCTION_HASREACHEDMAXNUMLOOPS, s_functionHasReachedMaxNumLoops)
                ->EnumAttribute(FUNCTION_PLAYTIME,              s_functionHasReachedPlayTime)
                ->EnumAttribute(FUNCTION_PLAYTIMELEFT,          s_functionHasLessThan)
                ->EnumAttribute(FUNCTION_ISMOTIONASSIGNED,      s_functionIsMotionAssigned)
                ->EnumAttribute(FUNCTION_ISMOTIONNOTASSIGNED,   s_functionIsMotionNotAssigned)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_numLoops, "Num Loops", "The int value to test against the number of loops the motion already played.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<AZ::s32>::max())
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetNumLoopsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_playTime, "Time Value", "The float value in seconds to test against.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetPlayTimeVisibility)
            ->DataElement(AZ_CRC_CE("EMotionFX::EventData"), &AnimGraphMotionCondition::m_eventDatas, "Event Parameters", "The event parameters to match.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetEventPropertiesVisibility)
                ->ElementAttribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("EMotionFX::EventData"))
            ;
    }
} // namespace EMotionFX
