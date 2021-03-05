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

#include "InputChannelGestureDrag.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureDrag::TypeAndConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TypeAndConfig, Type, Config>()
                ->Version(0)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TypeAndConfig>("Drag", "Gesture recognizer for drags.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }

        RecognizerDrag::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGesture* InputChannelGestureDrag::TypeAndConfig::CreateInputChannel(
        const AzFramework::InputChannelId& channelId,
        const AzFramework::InputDevice& inputDevice)
    {
        return aznew InputChannelGestureDrag(channelId, inputDevice, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureDrag::InputChannelGestureDrag(const InputChannelId& inputChannelId,
                                                     const InputDevice& inputDevice,
                                                     const Config& config)
        : InputChannelGesture(inputChannelId, inputDevice)
        , RecognizerDrag(config)
    {
        RecognizerDrag::Enable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelGestureDrag::~InputChannelGestureDrag()
    {
        RecognizerDrag::Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelGestureDrag::GetValue() const
    {
        return InputChannel::IsActive() ? GetDistance() : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelGestureDrag::GetCustomData() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureDrag::OnContinuousGestureInitiated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentPosition());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureDrag::OnContinuousGestureUpdated()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentPosition());
        InputChannel::UpdateState(true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelGestureDrag::OnContinuousGestureEnded()
    {
        UpdateNormalizedPositionAndDeltaFromScreenPosition(GetCurrentPosition());
        InputChannel::UpdateState(false);
    }
} // namespace Gestures
