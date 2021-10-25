/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZ::u32 InputDeviceMouse::MovementSampleRateDefault = 60;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZ::u32 InputDeviceMouse::MovementSampleRateQueueAll = std::numeric_limits<AZ::u32>::max();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZ::u32 InputDeviceMouse::MovementSampleRateAccumulateAll = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceMouse::Id("mouse");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouse::IsMouseDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceMouse>();
            //  for (const InputChannelId& channelId : Button::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceMouse>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Button::Left.GetName(), BehaviorConstant(Button::Left.GetName()))
                ->Constant(Button::Right.GetName(), BehaviorConstant(Button::Right.GetName()))
                ->Constant(Button::Middle.GetName(), BehaviorConstant(Button::Middle.GetName()))
                ->Constant(Button::Other1.GetName(), BehaviorConstant(Button::Other1.GetName()))
                ->Constant(Button::Other2.GetName(), BehaviorConstant(Button::Other2.GetName()))

                ->Constant(Movement::X.GetName(), BehaviorConstant(Movement::X.GetName()))
                ->Constant(Movement::Y.GetName(), BehaviorConstant(Movement::Y.GetName()))
                ->Constant(Movement::Z.GetName(), BehaviorConstant(Movement::Z.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::InputDeviceMouse(AzFramework::InputDeviceId id)
        : InputDevice(id)
        , m_allChannelsById()
        , m_buttonChannelsById()
        , m_movementChannelsById()
        , m_cursorPositionChannel()
        , m_cursorPositionData2D(AZStd::make_shared<InputChannel::PositionData2D>())
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all button input channels
        for (const InputChannelId& channelId : Button::All)
        {
            InputChannelDigitalWithSharedPosition2D* channel = aznew InputChannelDigitalWithSharedPosition2D(channelId, *this, m_cursorPositionData2D);
            m_allChannelsById[channelId] = channel;
            m_buttonChannelsById[channelId] = channel;
        }

        // Create all movement input channels
        for (const InputChannelId& channelId : Movement::All)
        {
            InputChannelDeltaWithSharedPosition2D* channel = aznew InputChannelDeltaWithSharedPosition2D(channelId, *this, m_cursorPositionData2D);
            m_allChannelsById[channelId] = channel;
            m_movementChannelsById[channelId] = channel;
        }

        // Create the cursor position input channel
        m_cursorPositionChannel = aznew InputChannelDeltaWithSharedPosition2D(SystemCursorPosition, *this, m_cursorPositionData2D);
        m_allChannelsById[SystemCursorPosition] = m_cursorPositionChannel;

        // Create the platform specific implementation
        m_pimpl.reset(Implementation::Create(*this));

        // Connect to the system cursor request bus
        InputSystemCursorRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::~InputDeviceMouse()
    {
        // Disconnect from the system cursor request bus
        InputSystemCursorRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy the cursor position input channel
        delete m_cursorPositionChannel;

        // Destroy all movement input channels
        for (const auto& channelById : m_movementChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all button input channels
        for (const auto& channelById : m_buttonChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceMouse::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouse::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouse::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (m_pimpl)
        {
            m_pimpl->SetSystemCursorState(systemCursorState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouse::GetSystemCursorState() const
    {
        return m_pimpl ? m_pimpl->GetSystemCursorState() : SystemCursorState::Unknown;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        if (m_pimpl)
        {
            m_pimpl->SetSystemCursorPositionNormalized(positionNormalized);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouse::GetSystemCursorPositionNormalized() const
    {
        return m_pimpl ? m_pimpl->GetSystemCursorPositionNormalized() : AZ::Vector2::CreateZero();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::SetRawMovementSampleRate(AZ::u32 sampleRateHertz)
    {
        if (m_pimpl)
        {
            m_pimpl->SetRawMovementSampleRate(sampleRateHertz);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation::Implementation(InputDeviceMouse& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawMovementSampleRate()
        , m_rawButtonEventQueuesById()
        , m_rawMovementEventQueuesById()
        , m_timeOfLastRawMovementSample(AZStd::chrono::system_clock::now())
    {
        SetRawMovementSampleRate(MovementSampleRateDefault);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Implementation::QueueRawButtonEvent(const InputChannelId& inputChannelId,
                                                               bool rawButtonState)
    {
        // It should not (in theory) be possible to receive multiple button events with the same id
        // and state in succession; if it happens in practice for whatever reason this is still safe.
        m_rawButtonEventQueuesById[inputChannelId].push_back(rawButtonState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Implementation::QueueRawMovementEvent(const InputChannelId& inputChannelId,
                                                                 float rawMovementDelta)
    {
        auto now = AZStd::chrono::system_clock::now();
        auto deltaTime = now - m_timeOfLastRawMovementSample;
        auto& rawEventQueue = m_rawMovementEventQueuesById[inputChannelId];

        // Depending on the movement sample rate, multiple mouse movements within a frame are either:
        if (rawEventQueue.empty() || deltaTime.count() > m_rawMovementSampleRate)
        {
            // queued (to give a better response at low frame rates)
            rawEventQueue.push_back(rawMovementDelta);
            m_timeOfLastRawMovementSample = now;
        }
        else
        {
            // or accumulated (to avoid flooding the event queue)
            rawEventQueue.back() += rawMovementDelta;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Implementation::ProcessRawEventQueues()
    {
        // Update the shared cursor position data
        const AZ::Vector2 oldNormalizedPosition = m_inputDevice.m_cursorPositionData2D->m_normalizedPosition;
        const AZ::Vector2 newNormalizedPosition = GetSystemCursorPositionNormalized();
        m_inputDevice.m_cursorPositionData2D->m_normalizedPosition = newNormalizedPosition;
        m_inputDevice.m_cursorPositionData2D->m_normalizedPositionDelta = newNormalizedPosition - oldNormalizedPosition;

        // Process all raw input events that were queued since the last call to this function
        ProcessRawInputEventQueues(m_rawButtonEventQueuesById, m_inputDevice.m_buttonChannelsById);
        ProcessRawInputEventQueues(m_rawMovementEventQueuesById, m_inputDevice.m_movementChannelsById);

        // Mouse movement events are distinct in that we may not receive an 'ended' event with delta
        // value of zero when the mouse stops moving, so queueing one here ensures the channels will
        // always correctly transition into the 'ended' state the next time this function is called,
        // unless another movement delta is queued above in which case it will simply be added to 0.
        for (const InputChannelId& movementChannelId : Movement::All)
        {
            QueueRawMovementEvent(movementChannelId, 0.0f);
        }

        // Finally, update the cursor position input channel, treating it as active if it has moved
        const float distanceMoved = newNormalizedPosition.GetDistance(oldNormalizedPosition);
        m_inputDevice.m_cursorPositionChannel->ProcessRawInputEvent(distanceMoved);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouse::Implementation::SetRawMovementSampleRate(AZ::u32 sampleRateHertz)
    {
        // Guard against dividing by zero
        if (sampleRateHertz == MovementSampleRateAccumulateAll)
        {
            m_rawMovementSampleRate = static_cast<AZStd::sys_time_t>(std::numeric_limits<AZ::u32>::max());
        }
        else
        {
            m_rawMovementSampleRate = static_cast<AZStd::sys_time_t>(1000000 / sampleRateHertz);
        }
    }
} // namespace AzFramework
