/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InputChannelGestureRotate.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureRotate::TypeAndConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TypeAndConfig, Type, Config>()
                ->Version(0)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TypeAndConfig>("Rotate", "Gesture recognizer for rotations.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        RecognizerRotate::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesture* InputChannelGestureRotate::TypeAndConfig::CreateInputChannel(
        const AzFramework::InputChannelId& channelId,
        const AzFramework::InputDevice& inputDevice)
    {
        return aznew InputChannelGestureRotate(channelId, inputDevice, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureRotate::InputChannelGestureRotate(const InputChannelId& inputChannelId,
                                                         const InputDevice& inputDevice,
                                                         const Config& config)
        : InputChannelGesture(inputChannelId, inputDevice)
        , RecognizerRotate(config)
    {
        RecognizerRotate::Enable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureRotate::~InputChannelGestureRotate()
    {
        RecognizerRotate::Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelGestureRotate::GetValue() const
    {
        return InputChannel::IsActive() ? GetSignedRotationInDegrees() : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelGestureRotate::GetCustomData() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureRotate::OnContinuousGestureInitiated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureRotate::OnContinuousGestureUpdated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureRotate::OnContinuousGestureEnded()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(false);
    }
} // namespace Gestures
