/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IMovieSystem.h>
#include <Range.h>
#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>

#include "AnimSerializer.h"

#include <AzCore/Serialization/SerializeContext.h>


void AnimSerializer::ReflectAnimTypes(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
    {
        // Reflection for Maestro's AZ_TYPE_INFO'ed classes
        serializeContext->Class<CAnimParamType>()
            ->Version(1)
            ->Field("Type", &CAnimParamType::m_type)
            ->Field("Name", &CAnimParamType::m_name);

        serializeContext->Class<Range>()
            ->Field("Start", &Range::start)
            ->Field("End", &Range::end);

        // Curve Key classes
        serializeContext->Class<IKey>()
            ->Field("Time", &IKey::time)
            ->Field("Flags", &IKey::flags);

        serializeContext->Class<AZ::IAssetBlendKey, ITimeRangeKey>()
            ->Field("AssetId", &AZ::IAssetBlendKey::m_assetId)
            ->Field("Description", &AZ::IAssetBlendKey::m_description)
            ->Field("BlendInTime", &AZ::IAssetBlendKey::m_blendInTime)
            ->Field("BlendOutTime", &AZ::IAssetBlendKey::m_blendOutTime);

        serializeContext->Class<IBoolKey, IKey>();

        serializeContext->Class<ICaptureKey, IKey>()
            ->Field("Duration", &ICaptureKey::duration)
            ->Field("TimeStep", &ICaptureKey::timeStep)
            ->Field("Folder", &ICaptureKey::folder)
            ->Field("Once", &ICaptureKey::once)
            ->Field("FilePrefix", &ICaptureKey::prefix);

        serializeContext->Class<ICharacterKey, ITimeRangeKey>()
            ->Field("Animation", &ICharacterKey::m_animation)
            ->Field("BlendGap", &ICharacterKey::m_bBlendGap)
            ->Field("PlayInPlace", &ICharacterKey::m_bInPlace);

        serializeContext->Class<ICommentKey, IKey>()
            ->Field("Comment", &ICommentKey::m_strComment)
            ->Field("Duration", &ICommentKey::m_duration)
            ->Field("Font", &ICommentKey::m_strFont)
            ->Field("Color", &ICommentKey::m_color)
            ->Field("Size", &ICommentKey::m_size)
            ->Field("Align", &ICommentKey::m_align);

        serializeContext->Class<IConsoleKey, IKey>()
            ->Field("Command", &IConsoleKey::command);

        serializeContext->Class<IDiscreteFloatKey, IKey>()
            ->Field("Value", &IDiscreteFloatKey::m_fValue);

        serializeContext->Class<IEventKey, IKey>()
            ->Field("Event", &IEventKey::event)
            ->Field("EventValue", &IEventKey::eventValue)
            ->Field("Anim", &IEventKey::animation)
            ->Field("Target", &IEventKey::target)
            ->Field("Length", &IEventKey::duration);

        serializeContext->Class<ILookAtKey, IKey>()
            ->Field("LookAtNodeName", &ILookAtKey::szSelection)
            ->Field("LookPose", &ILookAtKey::lookPose)
            ->Field("Duration", &ILookAtKey::fDuration)
            ->Field("SmoothTime", &ILookAtKey::smoothTime);

        serializeContext->Class<IScreenFaderKey, IKey>()
            ->Field("FadeTime", &IScreenFaderKey::m_fadeTime)
            ->Field("FadeColor", &IScreenFaderKey::m_fadeColor)
            ->Field("FadeType", &IScreenFaderKey::m_fadeType)
            ->Field("FadeChangeType", &IScreenFaderKey::m_fadeChangeType)
            ->Field("Texture", &IScreenFaderKey::m_strTexture)
            ->Field("useCurColor", &IScreenFaderKey::m_bUseCurColor);

        serializeContext->Class<ISelectKey, IKey>()
            ->Field("SelectedName", &ISelectKey::szSelection)
            ->Field("SelectedEntityId", &ISelectKey::cameraAzEntityId)
            ->Field("Duration", &ISelectKey::fDuration)
            ->Field("BlendTime", &ISelectKey::fBlendTime);

        serializeContext->Class<ISequenceKey, IKey>()
            ->Field("Node", &ISequenceKey::szSelection)
            ->Field("SequenceEntityId", &ISequenceKey::sequenceEntityId)
            ->Field("OverrideTimes", &ISequenceKey::bOverrideTimes)
            ->Field("StartTime", &ISequenceKey::fStartTime)
            ->Field("EndTime", &ISequenceKey::fEndTime);

        serializeContext->Class<ISoundKey, IKey>()
            ->Field("StartTrigger", &ISoundKey::sStartTrigger)
            ->Field("StopTrigger", &ISoundKey::sStopTrigger)
            ->Field("Duration", &ISoundKey::fDuration)
            ->Field("Color", &ISoundKey::customColor);

        serializeContext->Class<ITimeRangeKey, IKey>()
            ->Field("Duration", &ITimeRangeKey::m_duration)
            ->Field("Start", &ITimeRangeKey::m_startTime)
            ->Field("End", &ITimeRangeKey::m_endTime)
            ->Field("Speed", &ITimeRangeKey::m_speed)
            ->Field("Loop", &ITimeRangeKey::m_bLoop);
    }
}
