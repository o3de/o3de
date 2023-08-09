/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboard::IsVirtualKeyboardDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceVirtualKeyboard>();
            //  for (const InputChannelId& channelId : Command::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceVirtualKeyboard>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Command::EditClear.GetName(), BehaviorConstant(Command::EditClear.GetName()))
                ->Constant(Command::EditEnter.GetName(), BehaviorConstant(Command::EditEnter.GetName()))
                ->Constant(Command::NavigationBack.GetName(), BehaviorConstant(Command::NavigationBack.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::InputDeviceVirtualKeyboard(const InputDeviceId& inputDeviceId,
                                                           ImplementationFactory* implementationFactory)
        : InputDevice(inputDeviceId)
        , m_allChannelsById()
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all command input channels
        for (const InputChannelId& channelId : Command::All)
        {
            InputChannel* channel = aznew InputChannel(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_commandChannelsById[channelId] = channel;
        }

        // Create the platform specific or custom implementation
        m_pimpl = (implementationFactory != nullptr) ? implementationFactory->Create(*this) : nullptr;

        // Connect to the text entry request bus
        InputTextEntryRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::~InputDeviceVirtualKeyboard()
    {
        // Disconnect from the text entry request bus
        InputTextEntryRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all command input channels
        for (const auto& channelById : m_commandChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceVirtualKeyboard::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboard::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboard::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboard::HasTextEntryStarted() const
    {
        return m_pimpl ? m_pimpl->HasTextEntryStarted() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::TextEntryStart(const VirtualKeyboardOptions& options)
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStart(options);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::TextEntryStop()
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStop();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation::Implementation(InputDeviceVirtualKeyboard& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawCommandEventQueue()
        , m_rawTextEventQueue()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::Implementation::QueueRawCommandEvent(
        const InputChannelId& inputChannelId)
    {
        // Virtual keyboard commands are unique in that they don't go through states like most
        // other input channels. Rather, they simply dispatch one-off 'fire and forget' events.
        // But we still want to queue them so that they're dispatched in ProcessRawEventQueues
        // at the same as all other input events during the call to TickInputDevice each frame.
        m_rawCommandEventQueue.push_back(inputChannelId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::Implementation::QueueRawTextEvent(const AZStd::string& textUTF8)
    {
        m_rawTextEventQueue.push_back(textUTF8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::Implementation::ProcessRawEventQueues()
    {
        // Process all raw input events that were queued since the last call to this function.
        // Text events should be processed first in case text input is disabled by a command event.
        ProcessRawInputTextEventQueue(m_rawTextEventQueue);

        // Virtual keyboard commands are unique in that they don't go through states like most
        // other input channels. Rather, they simply dispatch one-off 'fire and forget' events.
        for (const InputChannelId& channelId : m_rawCommandEventQueue)
        {
            const auto& channelIt = m_inputDevice.m_commandChannelsById.find(channelId);
            if (channelIt != m_inputDevice.m_commandChannelsById.end() && channelIt->second)
            {
                const InputChannel& channel = *(channelIt->second);
                m_inputDevice.BroadcastInputChannelEvent(channel);
            }
            else
            {
                // Unknown channel id, warn but handle gracefully
                AZ_Warning("InputDeviceVirtualKeyboard::Implementation::ProcessRawEventQueues", false,
                           "Raw input event queued with unrecognized id: %s", channelId.GetName());
            }
        }
        m_rawCommandEventQueue.clear();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboard::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
