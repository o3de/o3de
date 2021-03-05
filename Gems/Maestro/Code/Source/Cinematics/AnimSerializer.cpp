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
#include "Maestro_precompiled.h"
#include <IMovieSystem.h>
#include <Range.h>
#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>

#include "AnimSerializer.h"

void AnimSerializer::ReflectAnimTypes(AZ::SerializeContext* context)
{
    // Reflection for Maestro's AZ_TYPE_INFO'ed classes
    context->Class<CAnimParamType>()
        ->Version(1)
        ->Field("Type", &CAnimParamType::m_type)
        ->Field("Name", &CAnimParamType::m_name);

    context->Class<Range>()
        ->Field("Start", &Range::start)
        ->Field("End", &Range::end);

    // Curve Key classes
    context->Class<IKey>()
        ->Field("Time", &IKey::time)
        ->Field("Flags", &IKey::flags);

    context->Class<AZ::IAssetBlendKey, ITimeRangeKey>()
        ->Field("AssetId", &AZ::IAssetBlendKey::m_assetId)
        ->Field("Description", &AZ::IAssetBlendKey::m_description)
        ->Field("BlendInTime", &AZ::IAssetBlendKey::m_blendInTime)
        ->Field("BlendOutTime", &AZ::IAssetBlendKey::m_blendOutTime);

    context->Class<IBoolKey, IKey>();

    context->Class<ICaptureKey, IKey>()
        ->Field("Duration", &ICaptureKey::duration)
        ->Field("TimeStep", &ICaptureKey::timeStep)
        ->Field("Folder", &ICaptureKey::folder)
        ->Field("Once", &ICaptureKey::once)
        ->Field("FilePrefix", &ICaptureKey::prefix)
        ->Field("Format", &ICaptureKey::format)
        ->Field("BufferType", &ICaptureKey::captureBufferIndex);

    context->Class<ICharacterKey, ITimeRangeKey>()
        ->Field("Animation", &ICharacterKey::m_animation)
        ->Field("BlendGap", &ICharacterKey::m_bBlendGap)
        ->Field("PlayInPlace", &ICharacterKey::m_bInPlace);

    context->Class<ICommentKey, IKey>()
        ->Field("Comment", &ICommentKey::m_strComment)
        ->Field("Duration", &ICommentKey::m_duration)
        ->Field("Font", &ICommentKey::m_strFont)
        ->Field("Color", &ICommentKey::m_color)
        ->Field("Size", &ICommentKey::m_size)
        ->Field("Align", &ICommentKey::m_align);

    context->Class<IConsoleKey, IKey>()
        ->Field("Command", &IConsoleKey::command);

    context->Class<IDiscreteFloatKey, IKey>()
        ->Field("Value", &IDiscreteFloatKey::m_fValue);

    context->Class<IEventKey, IKey>()
        ->Field("Event", &IEventKey::event)
        ->Field("EventValue", &IEventKey::eventValue)
        ->Field("Anim", &IEventKey::animation)
        ->Field("Target", &IEventKey::target)
        ->Field("Length", &IEventKey::duration);

    context->Class<ILookAtKey, IKey>()
        ->Field("LookAtNodeName", &ILookAtKey::szSelection)
        ->Field("LookPose", &ILookAtKey::lookPose)
        ->Field("Duration", &ILookAtKey::fDuration)
        ->Field("SmoothTime", &ILookAtKey::smoothTime);

    context->Class<IScreenFaderKey, IKey>()
        ->Field("FadeTime", &IScreenFaderKey::m_fadeTime)
        ->Field("FadeColor", &IScreenFaderKey::m_fadeColor)
        ->Field("FadeType", &IScreenFaderKey::m_fadeType)
        ->Field("FadeChangeType", &IScreenFaderKey::m_fadeChangeType)
        ->Field("Texture", &IScreenFaderKey::m_strTexture)
        ->Field("useCurColor", &IScreenFaderKey::m_bUseCurColor);

    context->Class<ISelectKey, IKey>()
        ->Field("SelectedName", &ISelectKey::szSelection)
        ->Field("SelectedEntityId", &ISelectKey::cameraAzEntityId)
        ->Field("Duration", &ISelectKey::fDuration)
        ->Field("BlendTime", &ISelectKey::fBlendTime);

    context->Class<ISequenceKey, IKey>()
        ->Field("Node", &ISequenceKey::szSelection)
        ->Field("SequenceEntityId", &ISequenceKey::sequenceEntityId)
        ->Field("OverrideTimes", &ISequenceKey::bOverrideTimes)
        ->Field("StartTime", &ISequenceKey::fStartTime)
        ->Field("EndTime", &ISequenceKey::fEndTime);

    context->Class<ISoundKey, IKey>()
        ->Field("StartTrigger", &ISoundKey::sStartTrigger)
        ->Field("StopTrigger", &ISoundKey::sStopTrigger)
        ->Field("Duration", &ISoundKey::fDuration)
        ->Field("Color", &ISoundKey::customColor);

    context->Class<ITimeRangeKey, IKey>()
        ->Field("Duration", &ITimeRangeKey::m_duration)
        ->Field("Start", &ITimeRangeKey::m_startTime)
        ->Field("End", &ITimeRangeKey::m_endTime)
        ->Field("Speed", &ITimeRangeKey::m_speed)
        ->Field("Loop", &ITimeRangeKey::m_bLoop);
}
