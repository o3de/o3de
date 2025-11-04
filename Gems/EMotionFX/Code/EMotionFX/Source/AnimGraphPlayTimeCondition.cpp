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
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    const char* AnimGraphPlayTimeCondition::s_modeReachedPlayTimeX = "Reached Play Time X";
    const char* AnimGraphPlayTimeCondition::s_modeReachedEnd = "Reached End";
    const char* AnimGraphPlayTimeCondition::s_modeHasLessThanXSecondsLeft = "Less Than X Seconds Left";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphPlayTimeCondition, AnimGraphAllocator)

    AnimGraphPlayTimeCondition::AnimGraphPlayTimeCondition()
        : AnimGraphTransitionCondition()
        , m_node(nullptr)
        , m_playTime(1.0f)
        , m_mode(MODE_REACHEDTIME)
    {
    }


    AnimGraphPlayTimeCondition::AnimGraphPlayTimeCondition(AnimGraph* animGraph)
        : AnimGraphPlayTimeCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphPlayTimeCondition::~AnimGraphPlayTimeCondition()
    {
    }


    void AnimGraphPlayTimeCondition::Reinit()
    {
        if (!AnimGraphNodeId(m_nodeId).IsValid())
        {
            m_node = nullptr;
            return;
        }

        m_node = m_animGraph->RecursiveFindNodeById(m_nodeId);
    }


    bool AnimGraphPlayTimeCondition::InitAfterLoading(AnimGraph* animGraph)
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
    const char* AnimGraphPlayTimeCondition::GetPaletteName() const
    {
        return "Play Time Condition";
    }


    // test the condition
    bool AnimGraphPlayTimeCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // if no node has been selected yet, always return false
        if (!m_node)
        {
            return false;
        }

        const float x           = m_playTime;
        const float playTime    = m_node->GetCurrentPlayTime(animGraphInstance);
        const float duration    = m_node->GetDuration(animGraphInstance);
        const float timeLeft    = duration - playTime;// TODO: what about backwards playing a node?

        switch (m_mode)
        {
        // has the selected node reached the given playtime?
        case MODE_REACHEDTIME:
        {
            if (playTime >= x)
            {
                return true;
            }

            break;
        }

        // has the selected node reached the end? does only work for non-looping motions
        case MODE_REACHEDEND:
        {
            if (playTime >= duration - MCore::Math::epsilon)
            {
                return true;
            }

            break;
        }

        // has the selected node less than X seconds remaining?
        case MODE_HASLESSTHAN:
        {
            if (timeLeft <= x + MCore::Math::epsilon)
            {
                return true;
            }

            break;
        }
        }

        return false;
    }


    // construct and output the information summary string for this object
    void AnimGraphPlayTimeCondition::GetSummary(AZStd::string* outResult) const
    {
        AZStd::string nodeName;
        if (m_node)
        {
            nodeName = m_node->GetNameString();
        }

        *outResult = AZStd::string::format("%s: NodeName='%s', Play Time=%.2f secs, Mode='%s'", RTTI_GetTypeName(), nodeName.c_str(), m_playTime, GetModeString());
    }


    // construct and output the tooltip for this object
    void AnimGraphPlayTimeCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"105\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // add the node
        AZStd::string nodeName;
        if (m_node)
        {
            nodeName = m_node->GetNameString();
        }

        columnName = "Node: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), nodeName.c_str());

        // add the time
        columnName = "Play Time: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%.2f secs</td>", columnName.c_str(), m_playTime);

        // add the mode
        columnName = "Mode: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), GetModeString());
    }


    void AnimGraphPlayTimeCondition::SetNodeId(AnimGraphNodeId nodeId)
    {
        m_nodeId = nodeId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AnimGraphNodeId AnimGraphPlayTimeCondition::GetNodeId() const
    {
        return m_nodeId;
    }


    AnimGraphNode* AnimGraphPlayTimeCondition::GetNode() const
    {
        return m_node;
    }


    // callback that gets called before a node gets removed
    void AnimGraphPlayTimeCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (m_nodeId == nodeToRemove->GetId())
        {
            SetNodeId(AnimGraphNodeId::InvalidId);
        }
    }


    void AnimGraphPlayTimeCondition::SetPlayTime(float playTime)
    {
        m_playTime = playTime;
    }


    float AnimGraphPlayTimeCondition::GetPlayTime() const
    {
        return m_playTime;
    }


    void AnimGraphPlayTimeCondition::SetMode(Mode mode)
    {
        m_mode = mode;
    }


    AnimGraphPlayTimeCondition::Mode AnimGraphPlayTimeCondition::GetMode() const
    {
        return m_mode;
    }


    const char* AnimGraphPlayTimeCondition::GetModeString() const
    {
        switch (m_mode)
        {
            case MODE_REACHEDTIME:
            {
                return s_modeReachedPlayTimeX;
            }
            case MODE_REACHEDEND:
            {
                return s_modeReachedEnd;
            }
            case MODE_HASLESSTHAN:
            {
                return s_modeHasLessThanXSecondsLeft;
            }
            default:
            {
                return "Unknown mode";
            }
        }
    }


    AZ::Crc32 AnimGraphPlayTimeCondition::GetModeVisibility() const
    {
        return AnimGraphNodeId(m_nodeId).IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphPlayTimeCondition::GetPlayTimeVisibility() const
    {
        if (GetModeVisibility() == AZ::Edit::PropertyVisibility::Hide || m_mode == MODE_REACHEDEND)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    void AnimGraphPlayTimeCondition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(m_nodeId);
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-nodeId %llu", itConvertedIds->second);
        }
    }


    void AnimGraphPlayTimeCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphPlayTimeCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("nodeId", &AnimGraphPlayTimeCondition::m_nodeId)
            ->Field("mode", &AnimGraphPlayTimeCondition::m_mode)
            ->Field("playTime", &AnimGraphPlayTimeCondition::m_playTime)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphPlayTimeCondition>("Play Time Condition", "Play time condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphNodeId"), &AnimGraphPlayTimeCondition::m_nodeId, "Node", "The node to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphPlayTimeCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphPlayTimeCondition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphPlayTimeCondition::m_mode, "Mode", "The way how to check the given play time set in this condition with the playtime from the node.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphPlayTimeCondition::GetModeVisibility)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(MODE_REACHEDTIME, s_modeReachedPlayTimeX)
                ->EnumAttribute(MODE_REACHEDEND, s_modeReachedEnd)
                ->EnumAttribute(MODE_HASLESSTHAN, s_modeHasLessThanXSecondsLeft)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphPlayTimeCondition::m_playTime, "Play Time", "The play time in seconds.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphPlayTimeCondition::GetPlayTimeVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
