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

#include "VirtualGamepad_precompiled.h"

#include "InputDeviceVirtualGamepad.h"

#include "VirtualGamepadButtonRequestBus.h"
#include "VirtualGamepadThumbStickRequestBus.h"

#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceVirtualGamepad::Id("virtual_gamepad");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualGamepad::IsVirtualGamepadDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualGamepad::InputDeviceVirtualGamepad(
        const AZStd::unordered_set<AZStd::string>& buttonNames,
        const AZStd::unordered_set<AZStd::string>& thumbStickNames)
    : InputDevice(Id)
    , m_allChannelsById()
    , m_buttonChannelsByName()
    , m_thumbStickAxis1DChannelsByName()
    , m_thumbStickAxis2DChannelsByName()
    , m_thumbStickDirectionChannelsByName()
    {
        // Create all button input channels
        for (const AZStd::string& buttonName : buttonNames)
        {
            CreateButtonChannel(buttonName);
        }

        // Create all thumb-stick input channels
        for (const AZStd::string& thumbStickName : thumbStickNames)
        {
            CreateThumbStickAxis2DChannel(thumbStickName);
            CreateThumbStickAxis1DChannel(thumbStickName + "_x");
            CreateThumbStickAxis1DChannel(thumbStickName + "_y");
            CreateThumbStickDirectionChannel(thumbStickName + "_u");
            CreateThumbStickDirectionChannel(thumbStickName + "_d");
            CreateThumbStickDirectionChannel(thumbStickName + "_l");
            CreateThumbStickDirectionChannel(thumbStickName + "_r");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualGamepad::~InputDeviceVirtualGamepad()
    {
        // Destroy all thumb-stick direction input channels
        for (const auto& channelByName : m_thumbStickDirectionChannelsByName)
        {
            delete channelByName.second;
        }

        // Destroy all thumb-stick 2D axis input channels
        for (const auto& channelByName : m_thumbStickAxis2DChannelsByName)
        {
            delete channelByName.second;
        }

        // Destroy all thumb-stick 1D axis input channels
        for (const auto& channelByName : m_thumbStickAxis1DChannelsByName)
        {
            delete channelByName.second;
        }

        // Destroy all button input channels
        for (const auto& channelByName : m_buttonChannelsByName)
        {
            delete channelByName.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceVirtualGamepad::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualGamepad::IsSupported() const
    {
        // Touch input must be supported
        const InputDevice* inputDevice = nullptr;
        InputDeviceRequestBus::EventResult(inputDevice,
                                           InputDeviceTouch::Id,
                                           &InputDeviceRequests::GetInputDevice);
        return inputDevice && inputDevice->IsSupported();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualGamepad::IsConnected() const
    {
        // Touch input must be connected
        const InputDevice* inputDevice = nullptr;
        InputDeviceRequestBus::EventResult(inputDevice,
                                           InputDeviceTouch::Id,
                                           &InputDeviceRequests::GetInputDevice);
        return inputDevice && inputDevice->IsConnected();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualGamepad::TickInputDevice()
    {
        for (const auto& buttonChannelByName : m_buttonChannelsByName)
        {
            bool isButtonPressed = false;
            VirtualGamepadButtonRequestBus::EventResult(isButtonPressed,
                                                        buttonChannelByName.first,
                                                        &VirtualGamepadButtonRequests::IsPressed);
            buttonChannelByName.second->ProcessRawInputEvent(isButtonPressed);
        }

        for (const auto& thumbStickAxis2DChannelByName : m_thumbStickAxis2DChannelsByName)
        {
            const AZStd::string& thumbStickName = thumbStickAxis2DChannelByName.first;
            AZ::Vector2 axisValues(0.0f, 0.0f);
            VirtualGamepadThumbStickRequestBus::EventResult(axisValues,
                                                            thumbStickName,
                                                            &VirtualGamepadThumbStickRequests::GetCurrentAxisValuesNormalized);

            thumbStickAxis2DChannelByName.second->ProcessRawInputEvent(axisValues);
            m_thumbStickAxis1DChannelsByName[thumbStickName + "_x"]->ProcessRawInputEvent(axisValues.GetX());
            m_thumbStickAxis1DChannelsByName[thumbStickName + "_y"]->ProcessRawInputEvent(axisValues.GetY());

            const float upValue = AZ::GetClamp(axisValues.GetY(), 0.0f, 1.0f);
            const float downValue = AZ::GetClamp(axisValues.GetY(), -1.0f, 0.0f);
            const float leftValue = AZ::GetClamp(axisValues.GetX(), -1.0f, 0.0f);
            const float rightValue = AZ::GetClamp(axisValues.GetX(), 0.0f, 1.0f);
            m_thumbStickDirectionChannelsByName[thumbStickName + "_u"]->ProcessRawInputEvent(upValue);
            m_thumbStickDirectionChannelsByName[thumbStickName + "_d"]->ProcessRawInputEvent(downValue);
            m_thumbStickDirectionChannelsByName[thumbStickName + "_l"]->ProcessRawInputEvent(leftValue);
            m_thumbStickDirectionChannelsByName[thumbStickName + "_r"]->ProcessRawInputEvent(rightValue);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualGamepad::CreateButtonChannel(const AZStd::string& channelName)
    {
        const InputChannelId channelId(channelName.c_str());
        InputChannelDigital* channel = aznew InputChannelDigital(channelId, *this);
        m_allChannelsById[channelId] = channel;
        m_buttonChannelsByName[channelName] = channel;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualGamepad::CreateThumbStickAxis1DChannel(const AZStd::string& channelName)
    {
        const InputChannelId channelId(channelName.c_str());
        InputChannelAxis1D* channel = aznew InputChannelAxis1D(channelId, *this);
        m_allChannelsById[channelId] = channel;
        m_thumbStickAxis1DChannelsByName[channelName] = channel;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualGamepad::CreateThumbStickAxis2DChannel(const AZStd::string& channelName)
    {
        const InputChannelId channelId(channelName.c_str());
        InputChannelAxis2D* channel = aznew InputChannelAxis2D(channelId, *this);
        m_allChannelsById[channelId] = channel;
        m_thumbStickAxis2DChannelsByName[channelName] = channel;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualGamepad::CreateThumbStickDirectionChannel(const AZStd::string& channelName)
    {
        const InputChannelId channelId(channelName.c_str());
        InputChannelAnalog* channel = aznew InputChannelAnalog(channelId, *this);
        m_allChannelsById[channelId] = channel;
        m_thumbStickDirectionChannelsByName[channelName] = channel;
    }
} // namespace VirtualGamepad
