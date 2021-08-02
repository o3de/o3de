/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannel.h>

#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzFramework/Input/Buses/Notifications/InputChannelNotificationBus.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannel& inputChannel)
        : m_channelId(inputChannel.GetInputChannelId())
        , m_deviceId(inputChannel.GetInputDevice().GetInputDeviceId())
        , m_state(inputChannel.GetState())
        , m_value(inputChannel.GetValue())
        , m_delta(inputChannel.GetDelta())
        , m_localUserId(inputChannel.GetInputDevice().GetAssignedLocalUserId())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannelId& channelId,
                                     const InputDeviceId& deviceId,
                                     State state)
        : m_channelId(channelId)
        , m_deviceId(deviceId)
        , m_state(state)
        , m_value((state == State::Began || state == State::Updated) ? 1.0f : 0.0f)
        , m_delta((state == State::Began) ? 1.0f : ((state == State::Ended) ? -1.0f : 0.0f))
        , m_localUserId(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannelId& channelId,
                                     const InputDeviceId& deviceId,
                                     State state,
                                     float value,
                                     float delta)
        : m_channelId(channelId)
        , m_deviceId(deviceId)
        , m_state(state)
        , m_value(value)
        , m_delta(delta)
        , m_localUserId(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputChannel::PositionData2D::ConvertToScreenSpaceCoordinates(float screenWidth, float screenHeight) const
    {
        return AZ::Vector2(m_normalizedPosition.GetX() * screenWidth,
                           m_normalizedPosition.GetY() * screenHeight);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannel::PositionData2D::UpdateNormalizedPositionAndDelta(const AZ::Vector2& newPosition)
    {
        const AZ::Vector2 oldPosition = m_normalizedPosition;
        m_normalizedPosition = newPosition;
        m_normalizedPositionDelta = newPosition - oldPosition;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class InputChannelNotificationBusBehaviorHandler
        : public InputChannelNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_EBUS_BEHAVIOR_BINDER(InputChannelNotificationBusBehaviorHandler
            , "{88A23B57-E8EB-49B7-817C-2C85CE0C09E8}"
            , AZ::SystemAllocator
            , OnInputChannelEvent
        );

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed) override
        {
            if (!o_hasBeenConsumed)
            {
                Call(FN_OnInputChannelEvent, &inputChannel, false);
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelRequests::BusIdType::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BusIdType>("InputChannelRequest_BusId")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Constructor<const char*>()
                ->Constructor<const char*, AZ::u32>()
                ->Property("channelId", BehaviorValueProperty(&BusIdType::m_channelId))
                ->Property("deviceIndex", BehaviorValueProperty(&BusIdType::m_deviceIndex))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Reflect input channel notifications and requests for use in lua.
    //
    // Input notification example:
    //
    //  function ScriptName:OnActivate()
    //      self.inputChannelNotificationBus = InputChannelNotificationBus.Connect(self);
    //  end
    //
    //  function ScriptName:OnDeactivate()
    //      self.inputChannelNotificationBus:Disconnect(self);
    //  end
    //
    //  function ScriptName:OnInputChannelEvent(inputChannel)
    //      Debug.Log("OnInputChannelEvent channelName = " .. inputChannel.channelName)
    //      Debug.Log("OnInputChannelEvent deviceName = " .. inputChannel.deviceName)
    //      Debug.Log("OnInputChannelEvent state = " .. inputChannel.state)
    //      Debug.Log("OnInputChannelEvent value = " .. inputChannel.value)
    //  end
    //
    //
    // Input request example:
    //
    //  function ScriptName:OnTick(deltaTime, timePoint)
    //      local channelRequestId = InputChannelRequest_BusId(InputDeviceMouse.mouse_button_left)
    //      local inputChannel = InputChannelRequestBus.Event.GetInputChannel(channelRequestId)
    //      if (inputChannel:IsActive()) then
    //          Debug.Log("Left mouse button active")
    //      end
    //  end
    //
    void InputChannel::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputChannel>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Property("channelName", [](InputChannel* thisPtr) { return thisPtr->GetInputChannelId().GetName(); }, nullptr)
                ->Property("deviceName", [](InputChannel* thisPtr) { return thisPtr->GetInputDevice().GetInputDeviceId().GetName(); }, nullptr)
                ->Property("deviceIndex", [](InputChannel* thisPtr) { return thisPtr->GetInputDevice().GetInputDeviceId().GetIndex(); }, nullptr)
                ->Property("localUserId", [](InputChannel* thisPtr) { return thisPtr->GetInputDevice().GetAssignedLocalUserId(); }, nullptr)
                ->Property("state", [](InputChannel* thisPtr) { return thisPtr->GetState(); }, nullptr)
                ->Property("value", [](InputChannel* thisPtr) { return thisPtr->GetValue(); }, nullptr)
                ->Property("delta", [](InputChannel* thisPtr) { return thisPtr->GetDelta(); }, nullptr)
                ->Method("IsStateIdle", &InputChannel::IsStateIdle)
                ->Method("IsStateBegan", &InputChannel::IsStateBegan)
                ->Method("IsStateUpdated", &InputChannel::IsStateUpdated)
                ->Method("IsStateEnded", &InputChannel::IsStateEnded)
                ->Method("IsActive", &InputChannel::IsActive)
                ->Enum<int(State::Idle)>("State_Idle")
                ->Enum<int(State::Began)>("State_Began")
                ->Enum<int(State::Updated)>("State_Updated")
                ->Enum<int(State::Ended)>("State_Ended")
            ;

            behaviorContext->EBus<InputChannelNotificationBus>("InputChannelNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Handler<InputChannelNotificationBusBehaviorHandler>()
            ;

            InputChannelRequests::BusIdType::Reflect(context);
            behaviorContext->EBus<InputChannelRequestBus>("InputChannelRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Event("GetInputChannel", &InputChannelRequestBus::Events::GetInputChannel)
                ->Event("SimulateRawInput", &InputChannelRequestBus::Events::SimulateRawInput)
                ->Event("SimulateRawInput2D", &InputChannelRequestBus::Events::SimulateRawInput2D)
                ->Event("SimulateRawInput3D", &InputChannelRequestBus::Events::SimulateRawInput3D)
                ->Event("SimulateRawInputWithPosition2D", &InputChannelRequestBus::Events::SimulateRawInputWithPosition2D)
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::InputChannel(const InputChannelId& inputChannelId, const InputDevice& inputDevice)
        : m_inputChannelId(inputChannelId)
        , m_inputDevice(inputDevice)
        , m_state(State::Idle)
    {
        const InputChannelRequests::BusIdType busId(m_inputChannelId,
                                                    m_inputDevice.GetInputDeviceId().GetIndex());
        InputChannelRequestBus::Handler::BusConnect(busId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::~InputChannel()
    {
        ResetState();

        const InputChannelRequests::BusIdType busId(m_inputChannelId,
                                                    m_inputDevice.GetInputDeviceId().GetIndex());
        InputChannelRequestBus::Handler::BusDisconnect(busId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel* InputChannel::GetInputChannel() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId& InputChannel::GetInputChannelId() const
    {
        return m_inputChannelId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice& InputChannel::GetInputDevice() const
    {
        return m_inputDevice;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::State InputChannel::GetState() const
    {
        return m_state;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateIdle() const
    {
        return m_state == State::Idle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateBegan() const
    {
        return m_state == State::Began;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateUpdated() const
    {
        return m_state == State::Updated;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateEnded() const
    {
        return m_state == State::Ended;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsActive() const
    {
        return IsStateBegan() || IsStateUpdated();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannel::GetValue() const
    {
        return 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannel::GetDelta() const
    {
        return 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannel::GetCustomData() const
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::UpdateState(bool isChannelActive)
    {
        const State previousState = m_state;
        switch (m_state)
        {
            case State::Idle:
            {
                if (isChannelActive)
                {
                    m_state = State::Began;
                }
            }
            break;
            case State::Began:
            {
                if (isChannelActive)
                {
                    m_state = State::Updated;
                }
                else
                {
                    m_state = State::Ended;
                }
            }
            break;
            case State::Updated:
            {
                if (!isChannelActive)
                {
                    m_state = State::Ended;
                }
            }
            break;
            case State::Ended:
            {
                if (!isChannelActive)
                {
                    m_state = State::Idle;
                }
                else
                {
                    m_state = State::Began;
                }
            }
            break;
        }

        if (m_state != State::Idle)
        {
            bool hasBeenConsumed = false;
            InputChannelNotificationBus::Broadcast(&InputChannelNotifications::OnInputChannelEvent,
                                                   *this,
                                                   hasBeenConsumed);
        }

        return m_state != previousState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannel::ResetState()
    {
        // Ensure the channel transitions to the 'Ended' state if it happens to currently be active
        if (IsActive())
        {
            UpdateState(false);
        }

        // Directly return the channel to the 'Idle' state
        m_state = State::Idle;
    }
} // namespace AzFramework
