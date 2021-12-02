/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzFramework/Input/Buses/Notifications/InputChannelNotificationBus.h>
#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>
#include <AzFramework/Input/Buses/Notifications/InputTextNotificationBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class InputDeviceNotificationBusBehaviorHandler
        : public InputDeviceNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_EBUS_BEHAVIOR_BINDER(InputDeviceNotificationBusBehaviorHandler
            , "{95C1315E-C568-458B-B29F-8FC610B25EF7}"
            , AZ::SystemAllocator
            , OnInputDeviceConnectedEvent
            , OnInputDeviceDisconnectedEvent
        );

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnInputDeviceConnectedEvent(const InputDevice& inputDevice) override
        {
            Call(FN_OnInputDeviceConnectedEvent, &inputDevice);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnInputDeviceDisconnectedEvent(const InputDevice& inputDevice) override
        {
            Call(FN_OnInputDeviceDisconnectedEvent, &inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputDevice>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Property("deviceName", [](InputDevice* thisPtr) { return thisPtr->GetInputDeviceId().GetName(); }, nullptr)
                ->Property("deviceIndex", [](InputDevice* thisPtr) { return thisPtr->GetInputDeviceId().GetIndex(); }, nullptr)
                ->Property("localUserId", [](InputDevice* thisPtr) { return thisPtr->GetAssignedLocalUserId(); }, nullptr)
                ->Method("PromptLocalUserSignIn", &InputDevice::PromptLocalUserSignIn)
                ->Method("IsSupported", &InputDevice::IsSupported)
                ->Method("IsConnected", &InputDevice::IsConnected)
            ;

            behaviorContext->EBus<InputDeviceNotificationBus>("InputDeviceNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Handler<InputDeviceNotificationBusBehaviorHandler>()
            ;

            behaviorContext->EBus<InputDeviceRequestBus>("InputDeviceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Event("GetInputDevice", &InputDeviceRequestBus::Events::GetInputDevice)
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDevice::InputDevice(const InputDeviceId& inputDeviceId)
        : m_inputDeviceId(inputDeviceId)
    {
        InputDeviceRequestBus::Handler::BusConnect(m_inputDeviceId);
        ApplicationLifecycleEvents::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDevice::~InputDevice()
    {
        ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        InputDeviceRequestBus::Handler::BusDisconnect(m_inputDeviceId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice* InputDevice::GetInputDevice() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId& InputDevice::GetInputDeviceId() const
    {
        return m_inputDeviceId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDevice::GetAssignedLocalUserId() const
    {
        return GetInputDeviceId().GetIndex();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::BroadcastInputChannelEvent(const InputChannel& inputChannel) const
    {
        bool hasBeenConsumed = false;
        InputChannelNotificationBus::Broadcast(
            &InputChannelNotifications::OnInputChannelEvent, inputChannel, hasBeenConsumed);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::BroadcastInputTextEvent(const AZStd::string& textUTF8) const
    {
        bool hasBeenConsumed = false;
        InputTextNotificationBus::Broadcast(
            &InputTextNotifications::OnInputTextEvent, textUTF8, hasBeenConsumed);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::BroadcastInputDeviceConnectedEvent() const
    {
        InputDeviceNotificationBus::Broadcast(
            &InputDeviceNotifications::OnInputDeviceConnectedEvent, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::BroadcastInputDeviceDisconnectedEvent() const
    {
        InputDeviceNotificationBus::Broadcast(
            &InputDeviceNotifications::OnInputDeviceDisconnectedEvent, *this);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::ResetInputChannelStates()
    {
        const InputChannelByIdMap& inputChannelsById = GetInputChannelsById();
        for (const auto& inputChannelById : inputChannelsById)
        {
            // We could do this more efficiently using a const_cast:
            //const_cast<InputChannel*>(inputChannelById.second)->ResetState();
            const InputChannelRequests::BusIdType requestId(inputChannelById.first,
                                                            m_inputDeviceId.GetIndex());
            InputChannelRequestBus::Event(requestId, &InputChannelRequests::ResetState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::GetInputDeviceIds(InputDeviceIdSet& o_deviceIds) const
    {
        o_deviceIds.insert(m_inputDeviceId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::GetInputDevicesById(InputDeviceByIdMap& o_devicesById) const
    {
        o_devicesById[m_inputDeviceId] = this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::GetInputDevicesByIdWithAssignedLocalUserId(InputDeviceByIdMap& o_devicesById,
                                                                 LocalUserId localUserId) const
    {
        if (localUserId == GetAssignedLocalUserId())
        {
            o_devicesById[m_inputDeviceId] = this;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::GetInputChannelIds(InputChannelIdSet& o_channelIds) const
    {
        const InputChannelByIdMap& inputChannelsById = GetInputChannelsById();
        for (const auto& inputChannelById : inputChannelsById)
        {
            o_channelIds.insert(inputChannelById.first);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::GetInputChannelsById(InputChannelByIdMap& o_channelsById) const
    {
        const InputChannelByIdMap& inputChannelsById = GetInputChannelsById();
        for (const auto& inputChannelById : inputChannelsById)
        {
            o_channelsById[inputChannelById.first] = inputChannelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDevice::OnApplicationConstrained(Event /*lastEvent*/)
    {
        ResetInputChannelStates();
    }
} // namespace AzFramework
