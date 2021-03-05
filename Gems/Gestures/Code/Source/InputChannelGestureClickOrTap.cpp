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

#include "InputChannelGestureClickOrTap.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureClickOrTap::TypeAndConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TypeAndConfig, Type, Config>()
                ->Version(0)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TypeAndConfig>("Click Or Tap", "Gesture recognizer for clicks or taps.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        RecognizerClickOrTap::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesture* InputChannelGestureClickOrTap::TypeAndConfig::CreateInputChannel(
        const AzFramework::InputChannelId& channelId,
        const AzFramework::InputDevice& inputDevice)
    {
        return aznew InputChannelGestureClickOrTap(channelId, inputDevice, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureClickOrTap::InputChannelGestureClickOrTap(const InputChannelId& inputChannelId,
                                                                 const InputDevice& inputDevice,
                                                                 const Config& config)
        : InputChannelGesture(inputChannelId, inputDevice)
        , RecognizerClickOrTap(config)
    {
        RecognizerClickOrTap::Enable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureClickOrTap::~InputChannelGestureClickOrTap()
    {
        RecognizerClickOrTap::Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelGestureClickOrTap::GetValue() const
    {
        return InputChannel::IsActive() ? GetConfig().minClicksOrTaps : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelGestureClickOrTap::GetCustomData() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureClickOrTap::OnDiscreteGestureRecognized()
    {
        // Discrete gestures simply dispatch one-off 'fire and forget' events.
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetEndPosition());
        InputChannel::UpdateState(true);
        InputChannel::UpdateState(false);
    }
} // namespace Gestures
