/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/MCoreSystem.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include "EMotionFXConfig.h"
#include "AnimGraphObject.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphObject, AnimGraphAllocator)

    AnimGraphObject::AnimGraphObject()
        : m_animGraph(nullptr)
        , m_objectIndex(MCORE_INVALIDINDEX32)
    {
    }


    AnimGraphObject::AnimGraphObject(AnimGraph* animGraph)
        : AnimGraphObject()
    {
        m_animGraph = animGraph;
    }


    AnimGraphObject::~AnimGraphObject()
    {
    }

    const char* AnimGraphObject::GetCategoryName(ECategory category)
    {
        switch (category)
        {
            case CATEGORY_SOURCES: { return "Sources"; }
            case CATEGORY_BLENDING: { return "Blending"; }
            case CATEGORY_CONTROLLERS: { return "Controllers"; }
            case CATEGORY_PHYSICS: { return "Physics"; }
            case CATEGORY_LOGIC: { return "Logic"; }
            case CATEGORY_MATH: { return "Math"; }
            case CATEGORY_MISC: { return "Misc"; }
            case CATEGORY_TRANSITIONS: { return "Transitions"; }
            case CATEGORY_TRANSITIONCONDITIONS: { return "Transition conditions"; }
            case CATEGORY_TRIGGERACTIONS: { return "Trigger actions"; }
        }

        return "Unknown category";
    }

    void AnimGraphObject::Unregister()
    {
    }


    // construct and output the information summary string for this object
    void AnimGraphObject::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: %s", RTTI_GetTypeName(), MCore::ReflectionSerializer::Serialize(this).GetValue().c_str());
    }


    // construct and output the tooltip for this object
    void AnimGraphObject::GetTooltip(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: %s", RTTI_GetTypeName(), MCore::ReflectionSerializer::Serialize(this).GetValue().c_str());
    }


    const char* AnimGraphObject::GetHelpUrl() const
    {
        return "";
    }


    // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
    size_t AnimGraphObject::SaveUniqueData(AnimGraphInstance* animGraphInstance, uint8* outputBuffer) const
    {
        AnimGraphObjectData* data = animGraphInstance->FindOrCreateUniqueObjectData(this);
        if (data)
        {
            return data->Save(outputBuffer);
        }

        return 0;
    }



    // load and return number of bytes read, when dataBuffer is nullptr, 0 should be returned
    size_t AnimGraphObject::LoadUniqueData(AnimGraphInstance* animGraphInstance, const uint8* dataBuffer)
    {
        AnimGraphObjectData* data = animGraphInstance->FindOrCreateUniqueObjectData(this);
        if (data)
        {
            return data->Load(dataBuffer);
        }

        return 0;
    }


    // collect internal objects
    void AnimGraphObject::RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        outObjects.emplace_back(const_cast<AnimGraphObject*>(this));
    }

    void AnimGraphObject::InvalidateUniqueDatas()
    {
        AnimGraphObject* object = this;
        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            object->InvalidateUniqueData(animGraphInstance);
        }
    }

    void AnimGraphObject::InvalidateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->GetUniqueObjectData(m_objectIndex);
        if (uniqueData)
        {
            uniqueData->Invalidate();
        }
    }

    void AnimGraphObject::ResetUniqueData(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->GetUniqueObjectData(m_objectIndex);
        if (uniqueData)
        {
            uniqueData->Reset();
        }
    }

    void AnimGraphObject::ResetUniqueDatas()
    {
        AnimGraphObject* object = this;
        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            object->ResetUniqueData(animGraphInstance);
        }
    }

    // default update implementation
    void AnimGraphObject::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);
        animGraphInstance->EnableObjectFlags(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_UPDATE_READY);
    }

    void AnimGraphObject::Reinit()
    {
        InvalidateUniqueDatas();
    }

    void AnimGraphObject::RecursiveReinit()
    {
        Reinit();
    }

    // check for an error
    bool AnimGraphObject::GetHasErrorFlag(AnimGraphInstance* animGraphInstance) const
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindOrCreateUniqueObjectData(this);
        if (uniqueData)
        {
            return uniqueData->GetHasError();
        }
        else
        {
            return false;
        }
    }

    // set the error flag
    void AnimGraphObject::SetHasErrorFlag(AnimGraphInstance* animGraphInstance, bool hasError)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindOrCreateUniqueObjectData(this);
        if (uniqueData == nullptr)
        {
            return;
        }

        uniqueData->SetHasError(hasError);
    }

    // init the internal attributes
    void AnimGraphObject::InitInternalAttributes(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        // currently base objects do not do this, but will come later
    }


    // remove the internal attributes
    void AnimGraphObject::RemoveInternalAttributesForAllInstances()
    {
        // currently base objects themselves do not have internal attributes yet, but this will come at some stage
    }


    // decrease internal attribute indices for index values higher than the specified parameter
    void AnimGraphObject::DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan)
    {
        MCORE_UNUSED(decreaseEverythingHigherThan);
        // currently no implementation for the base object type, but this will come later
    }


    // does the init for all anim graph instances in the parent animgraph
    void AnimGraphObject::InitInternalAttributesForAllInstances()
    {
        if (m_animGraph == nullptr)
        {
            return;
        }

        const size_t numInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            InitInternalAttributes(m_animGraph->GetAnimGraphInstance(i));
        }
    }


    void AnimGraphObject::SyncVisualObject()
    {
        AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnSyncVisualObject, this);
    }


    void AnimGraphObject::CalculateMotionExtractionDelta(EExtractionMode extractionMode, AnimGraphRefCountedData* sourceRefData, AnimGraphRefCountedData* targetRefData, float weight, bool hasMotionExtractionNodeInMask, Transform& outTransform, Transform& outTransformMirrored)
    {
        // Calculate the motion extraction output based on the motion extraction mode.
        switch (extractionMode)
        {
            // Blend between the source and target.
            case EXTRACTIONMODE_BLEND:
            {
                if (hasMotionExtractionNodeInMask)
                {
                    if (weight < MCore::Math::epsilon || !targetRefData) // Weight is 0.
                    {
                        outTransform = sourceRefData->GetTrajectoryDelta();
                        outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                    }
                    else if (weight < 1.0f - MCore::Math::epsilon) // Weight between 0 and 1.
                    {
                        outTransform = sourceRefData->GetTrajectoryDelta();
                        outTransform.Blend(targetRefData->GetTrajectoryDelta(), weight);
                        outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                        outTransformMirrored.Blend(targetRefData->GetTrajectoryDeltaMirrored(), weight);
                    }
                    else // Weight is 1.
                    {
                        outTransform = targetRefData->GetTrajectoryDelta();
                        outTransformMirrored = targetRefData->GetTrajectoryDeltaMirrored();
                    }
                }
                else
                {
                    outTransform = sourceRefData->GetTrajectoryDelta();
                    outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                }
            }
            break;

            // Output only the target state's delta.
            case EXTRACTIONMODE_TARGETONLY:
            {
                if (hasMotionExtractionNodeInMask)
                {
                    if (targetRefData)
                    {
                        outTransform = targetRefData->GetTrajectoryDelta();
                        outTransformMirrored = targetRefData->GetTrajectoryDeltaMirrored();
                    }
                    else
                    {
                        outTransform = sourceRefData->GetTrajectoryDelta();
                        outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                    }
                }
                else
                {
                    outTransform.IdentityWithZeroScale();
                    outTransformMirrored.IdentityWithZeroScale();
                }
            }
            break;

            // Output only the source state's delta.
            case EXTRACTIONMODE_SOURCEONLY:
            {
                outTransform = sourceRefData->GetTrajectoryDelta();
                outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
            }
            break;

            default:
                AZ_Assert(false, "Unknown motion extraction mode used.");
        }
    }


    void AnimGraphObject::CalculateMotionExtractionDeltaAdditive(EExtractionMode extractionMode, AnimGraphRefCountedData* sourceRefData, AnimGraphRefCountedData* targetRefData, const Transform& basePoseTransform, float weight, bool hasMotionExtractionNodeInMask, Transform& outTransform, Transform& outTransformMirrored)
    {
        if (!hasMotionExtractionNodeInMask)
        {
            outTransform = sourceRefData->GetTrajectoryDelta();
            outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
            return;
        }

        // Calculate the motion extraction output based on the motion extraction mode.
        switch (extractionMode)
        {
            // Blend between the source and target.
            case EXTRACTIONMODE_BLEND:
            {
                if (weight < MCore::Math::epsilon || !targetRefData) // Weight is 0 or there is no target ref data.
                {
                    outTransform = sourceRefData->GetTrajectoryDelta();
                    outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                }
                else // Weight between 0 and 1.
                {
                    outTransform = sourceRefData->GetTrajectoryDelta();
                    outTransform.BlendAdditive(targetRefData->GetTrajectoryDelta(), basePoseTransform, weight);
                    outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                    outTransformMirrored.BlendAdditive(targetRefData->GetTrajectoryDeltaMirrored(), basePoseTransform, weight);
                }
            }
            break;

            // Output only the target state's delta if it is available.
            case EXTRACTIONMODE_TARGETONLY:
            {
                if (targetRefData)
                {
                    outTransform = sourceRefData->GetTrajectoryDelta();
                    outTransform.BlendAdditive(targetRefData->GetTrajectoryDelta(), basePoseTransform, 1.0f); // TODO: could eliminate the lerp internally
                    outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                    outTransformMirrored.BlendAdditive(targetRefData->GetTrajectoryDeltaMirrored(), basePoseTransform, 1.0f); // TODO: could eliminate the lerp internally
                }
                else
                {
                    outTransform = sourceRefData->GetTrajectoryDelta();
                    outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
                }
            }
            break;

            // Output only the source state's delta.
            case EXTRACTIONMODE_SOURCEONLY:
            {
                outTransform = sourceRefData->GetTrajectoryDelta();
                outTransformMirrored = sourceRefData->GetTrajectoryDeltaMirrored();
            }
            break;

            default:
                AZ_Assert(false, "Unknown motion extraction mode used.");
        }
    }


    void AnimGraphObject::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphObject>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<ESyncMode>("Sync mode", "The synchronization method to use. Event track based will use event tracks, full clip based will ignore the events and sync as a full clip. If set to Event Track Based while no sync events exist inside the track a full clip based sync will be performed instead.")
            ->Value("Disabled", SYNCMODE_DISABLED)
            ->Value("Event track based", SYNCMODE_TRACKBASED)
            ->Value("Full clip based", SYNCMODE_CLIPBASED);

        editContext->Enum<EEventMode>("Event filter mode", "The event filter mode, which controls which events are passed further up the hierarchy.")
            ->Value("Leader node only", EVENTMODE_LEADERONLY)
            ->Value("Follower node only", EVENTMODE_FOLLOWERONLY)
            ->Value("Both nodes", EVENTMODE_BOTHNODES)
            ->Value("Most active", EVENTMODE_MOSTACTIVE)
            ->Value("None", EVENTMODE_NONE);

        editContext->Enum<EExtractionMode>("Extraction mode", "The motion extraction blend mode to use.")
            ->Value("Blend", EXTRACTIONMODE_BLEND)
            ->Value("Target only", EXTRACTIONMODE_TARGETONLY)
            ->Value("Source only", EXTRACTIONMODE_SOURCEONLY);
    }
} // namespace EMotionFX
