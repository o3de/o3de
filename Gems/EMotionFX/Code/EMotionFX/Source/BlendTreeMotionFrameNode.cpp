/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeMotionFrameNode.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "MotionInstance.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionFrameNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionFrameNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeMotionFrameNode::BlendTreeMotionFrameNode()
        : AnimGraphNode()
        , m_normalizedTimeValue(0.0f)
        , m_emitEventsFromStart(false)
    {
        // setup input ports
        InitInputPorts(2);
        SetupInputPort("Motion", INPUTPORT_MOTION, AttributeMotionInstance::TYPE_ID, PORTID_INPUT_MOTION);
        SetupInputPortAsNumber("Time", INPUTPORT_TIME, PORTID_INPUT_TIME);

        // link the output port value to the local pose object (it stores a pointer to the local pose)
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_RESULT);
    }


    BlendTreeMotionFrameNode::~BlendTreeMotionFrameNode()
    {
    }


    bool BlendTreeMotionFrameNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreeMotionFrameNode::GetPaletteName() const
    {
        return "Motion Frame";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeMotionFrameNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // perform the calculations / actions
    void BlendTreeMotionFrameNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // make sure our transform buffer is large enough
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
        AnimGraphPose* outputPose;

        // get the motion instance object
        BlendTreeConnection* motionConnection = m_inputPorts[INPUTPORT_MOTION].m_connection;
        if (motionConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // get the motion instance value
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_MOTION));
        MotionInstance* motionInstance = static_cast<MotionInstance*>(GetInputMotionInstance(animGraphInstance, INPUTPORT_MOTION)->GetValue());
        if (motionInstance == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // get the time value
        float timeValue = 0.0f;
        BlendTreeConnection* timeConnection = m_inputPorts[INPUTPORT_TIME].m_connection;
        if (!timeConnection) // get it from the parameter value if there is no connection
        {
            timeValue = m_normalizedTimeValue;
        }
        else
        {
            // get the time value and make sure it is in range
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TIME));
            timeValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TIME);
            timeValue = MCore::Clamp<float>(timeValue, 0.0f, 1.0f);
        }

        // output the transformations
        const float oldTime = motionInstance->GetCurrentTime();
        motionInstance->SetCurrentTimeNormalized(timeValue);
        motionInstance->SetPause(true);

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        // init the output pose with the bind pose for safety
        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        // output the pose
        motionInstance->GetMotion()->Update(&outputPose->GetPose(), &outputPose->GetPose(), motionInstance);

        // restore the time
        motionInstance->SetCurrentTime(oldTime);

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    // post sync update
    void BlendTreeMotionFrameNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // clear the event buffer
        if (m_disabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the time input
        BlendTreeConnection* timeConnection = m_inputPorts[INPUTPORT_TIME].m_connection;
        if (timeConnection)
        {
            PostUpdateIncomingNode(animGraphInstance, timeConnection->GetSourceNode(), timePassedInSeconds);
        }

        // update the input motion
        BlendTreeConnection* motionConnection = m_inputPorts[INPUTPORT_MOTION].m_connection;
        if (motionConnection)
        {
            PostUpdateIncomingNode(animGraphInstance, motionConnection->GetSourceNode(), timePassedInSeconds);

            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();

            AnimGraphNode* sourceNode = motionConnection->GetSourceNode();
            MCORE_ASSERT(azrtti_typeid(sourceNode) == azrtti_typeid<AnimGraphMotionNode>());
            AnimGraphMotionNode* motionNode = static_cast<AnimGraphMotionNode*>(sourceNode);

            const bool triggerEvents = motionNode->GetEmitEvents();
            MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
            if (triggerEvents && motionInstance)
            {
                motionInstance->ExtractEventsNonLoop(uniqueData->m_oldTime, uniqueData->m_newTime, &uniqueData->GetRefCountedData()->GetEventBuffer());
                data->GetEventBuffer().UpdateEmitters(this);
            }
        }
        else
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
        }
    }


    // default update implementation
    void BlendTreeMotionFrameNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the time input
        BlendTreeConnection* timeConnection = m_inputPorts[INPUTPORT_TIME].m_connection;
        if (timeConnection)
        {
            UpdateIncomingNode(animGraphInstance, timeConnection->GetSourceNode(), timePassedInSeconds);
        }

        // update the input motion
        BlendTreeConnection* motionConnection = m_inputPorts[INPUTPORT_MOTION].m_connection;
        if (motionConnection)
        {
            UpdateIncomingNode(animGraphInstance, motionConnection->GetSourceNode(), timePassedInSeconds);
        }

        // get the time value
        float timeValue = 0.0f;
        if (!timeConnection) // get it from the parameter value if there is no connection
        {
            timeValue = m_normalizedTimeValue;
        }
        else
        {
            timeValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TIME);
            timeValue = MCore::Clamp<float>(timeValue, 0.0f, 1.0f);
        }

        // output the right synctrack etc
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        if (uniqueData->m_rewindRequested)
        {
            if (m_emitEventsFromStart)
            {
                uniqueData->m_newTime = 0.0f;
                uniqueData->m_oldTime = 0.0f;
            }
            else
            {
                const float newTimeValue = uniqueData->GetDuration() * timeValue;
                uniqueData->m_newTime = newTimeValue;
                uniqueData->m_oldTime = newTimeValue;
            }
            uniqueData->m_rewindRequested = false;
        }

        if (motionConnection)
        {
            AnimGraphNode* motionNode = motionConnection->GetSourceNode();
            uniqueData->Init(animGraphInstance, motionNode);
            uniqueData->SetCurrentPlayTime(uniqueData->GetDuration() * timeValue);

            uniqueData->m_oldTime = uniqueData->m_newTime;
            uniqueData->m_newTime = uniqueData->GetDuration() * timeValue;
        }
        else
        {
            uniqueData->Clear();
        }
    }

    void BlendTreeMotionFrameNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Rewind(animGraphInstance);

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        if (uniqueData)
        {
            uniqueData->m_rewindRequested = true;
        }
    }

    void BlendTreeMotionFrameNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeMotionFrameNode, AnimGraphNode>()
            ->Version(1)
            -> Field("normalizedTimeValue", &BlendTreeMotionFrameNode::m_normalizedTimeValue)
            -> Field("emitEventsFromStart", &BlendTreeMotionFrameNode::m_emitEventsFromStart)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeMotionFrameNode>("Motion Frame", "Motion frame attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeMotionFrameNode::m_normalizedTimeValue, "Normalized time", "The normalized time value, which must be between 0 and 1. This is used when there is no connection plugged into the Time port.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionFrameNode::m_emitEventsFromStart, "Emit events from start", "On rewinding the motion frame node, all motion events from the start of the motion up to the set normalized time will be emitted.")
            ;
    }

    void BlendTreeMotionFrameNode::SetNormalizedTimeValue(float value)
    {
        m_normalizedTimeValue = value;
    }

    float BlendTreeMotionFrameNode::GetNormalizedTimeValue() const
    {
        return m_normalizedTimeValue;
    }

    void BlendTreeMotionFrameNode::SetEmitEventsFromStart(bool emitEventsFromStart)
    {
        m_emitEventsFromStart = emitEventsFromStart;
    }

    bool BlendTreeMotionFrameNode::GetEmitEventsFromStart() const
    {
        return m_emitEventsFromStart;
    }
} // namespace EMotionFX
