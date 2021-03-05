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

#include "InputChannelGestureSwipe.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureSwipe::TypeAndConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TypeAndConfig, Type, Config>()
                ->Version(0)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TypeAndConfig>("Swipe", "Gesture recognizer for swipes.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        RecognizerSwipe::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesture* InputChannelGestureSwipe::TypeAndConfig::CreateInputChannel(
        const AzFramework::InputChannelId& channelId,
        const AzFramework::InputDevice& inputDevice)
    {
        return aznew InputChannelGestureSwipe(channelId, inputDevice, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureSwipe::InputChannelGestureSwipe(const InputChannelId& inputChannelId,
                                                       const InputDevice& inputDevice,
                                                       const Config& config)
        : InputChannelGesture(inputChannelId, inputDevice)
        , RecognizerSwipe(config)
    {
        RecognizerSwipe::Enable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureSwipe::~InputChannelGestureSwipe()
    {
        RecognizerSwipe::Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelGestureSwipe::GetValue() const
    {
        return InputChannel::IsActive() ? GetVelocity() : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelGestureSwipe::GetCustomData() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureSwipe::OnDiscreteGestureRecognized()
    {
        // Discrete gestures simply dispatch one-off 'fire and forget' events.
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetEndPosition());
        InputChannel::UpdateState(true);
        InputChannel::UpdateState(false);
    }
} // namespace Gestures
