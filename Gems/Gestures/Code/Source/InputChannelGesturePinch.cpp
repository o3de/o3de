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

#include "Gestures_precompiled.h"

#include "InputChannelGesturePinch.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGesturePinch::TypeAndConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TypeAndConfig, Type, Config>()
                ->Version(0)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TypeAndConfig>("Pinch", "Gesture recognizer for pinches.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        RecognizerPinch::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesture* InputChannelGesturePinch::TypeAndConfig::CreateInputChannel(
        const AzFramework::InputChannelId& channelId,
        const AzFramework::InputDevice& inputDevice)
    {
        return aznew InputChannelGesturePinch(channelId, inputDevice, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesturePinch::InputChannelGesturePinch(const InputChannelId& inputChannelId,
                                                       const InputDevice& inputDevice,
                                                       const Config& config)
        : InputChannelGesture(inputChannelId, inputDevice)
        , RecognizerPinch(config)
    {
        RecognizerPinch::Enable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesturePinch::~InputChannelGesturePinch()
    {
        RecognizerPinch::Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelGesturePinch::GetValue() const
    {
        return InputChannel::IsActive() ? GetPinchRatio() : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelGesturePinch::GetCustomData() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGesturePinch::OnContinuousGestureInitiated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGesturePinch::OnContinuousGestureUpdated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGesturePinch::OnContinuousGestureEnded()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentMidpoint());
        InputChannel::UpdateState(false);
    }
} // namespace Gestures
