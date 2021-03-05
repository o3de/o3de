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

#include "InputDeviceGestures.h"

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceGestures::Id("gestures");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGestures::IsGesturesDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceGestures::Gesture::DoublePress("gesture_double_press");
    const InputChannelId InputDeviceGestures::Gesture::Drag("gesture_drag");
    const InputChannelId InputDeviceGestures::Gesture::Hold("gesture_hold");
    const InputChannelId InputDeviceGestures::Gesture::Pinch("gesture_pinch");
    const InputChannelId InputDeviceGestures::Gesture::Rotate("gesture_rotate");
    const InputChannelId InputDeviceGestures::Gesture::Swipe("gesture_swipe");
    const AZStd::array<InputChannelId, 6> InputDeviceGestures::Gesture::All =
    {{
        DoublePress,
        Drag,
        Hold,
        Pinch,
        Rotate,
        Swipe
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGestures::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceGestures>();
            //  for (const InputChannelId& channelId : Button::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceGestures>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Gesture::DoublePress.GetName(), BehaviorConstant(Gesture::DoublePress.GetName()))
                ->Constant(Gesture::Drag.GetName(), BehaviorConstant(Gesture::Drag.GetName()))
                ->Constant(Gesture::Hold.GetName(), BehaviorConstant(Gesture::Hold.GetName()))
                ->Constant(Gesture::Pinch.GetName(), BehaviorConstant(Gesture::Pinch.GetName()))
                ->Constant(Gesture::Rotate.GetName(), BehaviorConstant(Gesture::Rotate.GetName()))
                ->Constant(Gesture::Swipe.GetName(), BehaviorConstant(Gesture::Swipe.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGestures::InputDeviceGestures(const ConfigsByNameMap& gestureConfigsByName)
        : InputDevice(Id)
        , m_allChannelsById()
    {
        // Create all gesture input channels
        for (const auto& gestureConfigByName : gestureConfigsByName)
        {
            const InputChannelId channelId(gestureConfigByName.first.c_str());
            m_allChannelsById[channelId] = gestureConfigByName.second->CreateInputChannel(channelId,
                                                                                          *this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGestures::~InputDeviceGestures()
    {
        // Destroy all gesture input channels
        for (const auto& channelById : m_allChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceGestures::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGestures::IsSupported() const
    {
        // Mouse or touch input must be supported
        const InputDevice* inputDeviceMouse = nullptr;
        const InputDevice* inputDeviceTouch = nullptr;
        InputDeviceRequestBus::EventResult(inputDeviceMouse,
                                           InputDeviceMouse::Id,
                                           &InputDeviceRequests::GetInputDevice);
        InputDeviceRequestBus::EventResult(inputDeviceTouch,
                                           InputDeviceTouch::Id,
                                           &InputDeviceRequests::GetInputDevice);
        return (inputDeviceMouse && inputDeviceMouse->IsSupported()) ||
               (inputDeviceTouch && inputDeviceTouch->IsSupported());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGestures::IsConnected() const
    {
        // Mouse or touch input must be connected
        const InputDevice* inputDeviceMouse = nullptr;
        const InputDevice* inputDeviceTouch = nullptr;
        InputDeviceRequestBus::EventResult(inputDeviceMouse,
                                           InputDeviceMouse::Id,
                                           &InputDeviceRequests::GetInputDevice);
        InputDeviceRequestBus::EventResult(inputDeviceTouch,
                                           InputDeviceTouch::Id,
                                           &InputDeviceRequests::GetInputDevice);
        return (inputDeviceMouse && inputDeviceMouse->IsConnected()) ||
               (inputDeviceTouch && inputDeviceTouch->IsConnected());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGestures::TickInputDevice()
    {
        // All InputChannelGesture* classes listen for and process mouse and touch input directly,
        // so we don't actually need to do anything here.
    }
} // namespace Gestures
