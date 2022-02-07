/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceTouch::Id("touch");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouch::IsTouchDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceTouch>();
            //  for (const InputChannelId& channelId : Touch::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceTouch>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Touch::Index0.GetName(), BehaviorConstant(Touch::Index0.GetName()))
                ->Constant(Touch::Index1.GetName(), BehaviorConstant(Touch::Index1.GetName()))
                ->Constant(Touch::Index2.GetName(), BehaviorConstant(Touch::Index2.GetName()))
                ->Constant(Touch::Index3.GetName(), BehaviorConstant(Touch::Index3.GetName()))
                ->Constant(Touch::Index4.GetName(), BehaviorConstant(Touch::Index4.GetName()))
                ->Constant(Touch::Index5.GetName(), BehaviorConstant(Touch::Index5.GetName()))
                ->Constant(Touch::Index6.GetName(), BehaviorConstant(Touch::Index6.GetName()))
                ->Constant(Touch::Index7.GetName(), BehaviorConstant(Touch::Index7.GetName()))
                ->Constant(Touch::Index8.GetName(), BehaviorConstant(Touch::Index8.GetName()))
                ->Constant(Touch::Index9.GetName(), BehaviorConstant(Touch::Index9.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::InputDeviceTouch()
        : InputDevice(Id)
        , m_allChannelsById()
        , m_touchChannelsById()
        , m_pimpl(nullptr)
        , m_implementationRequestHandler(*this)
    {
        // Create all touch input channels
        for (AZ::u32 i = 0; i < Touch::All.size(); ++i)
        {
            const InputChannelId& channelId = Touch::All[i];
            InputChannelAnalogWithPosition2D* channel = aznew InputChannelAnalogWithPosition2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_touchChannelsById[channelId] = channel;
        }

        // Create the platform specific implementation
        m_pimpl.reset(Implementation::Create(*this));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::~InputDeviceTouch()
    {
        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all touch input channels
        for (const auto& channelById : m_touchChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceTouch::GetAssignedLocalUserId() const
    {
        return m_pimpl ? m_pimpl->GetAssignedLocalUserId() : InputDevice::GetAssignedLocalUserId();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceTouch::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouch::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouch::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::Implementation(InputDeviceTouch& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawTouchEventQueuesById()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceTouch::Implementation::GetAssignedLocalUserId() const
    {
        return m_inputDevice.GetInputDeviceId().GetIndex();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::RawTouchEvent::RawTouchEvent(float normalizedX,
                                                                   float normalizedY,
                                                                   float pressure,
                                                                   AZ::u32 index,
                                                                   State state)
        : InputChannelAnalogWithPosition2D::RawInputEvent(normalizedX, normalizedY, pressure)
        , m_index(index)
        , m_state(state)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::QueueRawTouchEvent(const RawTouchEvent& rawTouchEvent)
    {
        if (rawTouchEvent.m_index >= Touch::All.size())
        {
            AZ_Warning("InputDeviceTouch", false,
                       "Raw touch has index: %d that is >= max: %zd",
                       rawTouchEvent.m_index, Touch::All.size());
            return;
        }

        auto& rawEventQueue = m_rawTouchEventQueuesById[Touch::All[rawTouchEvent.m_index]];
        if (rawEventQueue.empty() || rawEventQueue.back().m_state != rawTouchEvent.m_state)
        {
            // No raw touches for this index have been queued this frame,
            // or the last raw touch queued was in a different state.
            rawEventQueue.push_back(rawTouchEvent);
        }
        else
        {
            // Because touches are sampled at a rate (~30-60fps) independent of the simulation
            // (which is necessary for the controls to remain responsive), when the simulation
            // frame rate drops we end up receiving multiple touch move events each frame, for
            // the same finger, which (especially with multi-touch) can generate enough events
            // to cause the simulation frame rate to drop even further. To combat this we will
            // combine multiple touch events for the same finger if they are in the same state.
            //
            // For example, the following sequence of events with the same index in one frame:
            // - Began, Moved, Moved, Ended, Began, Moved, Moved, Moved
            //
            // Will be collapsed as follows:
            // - Began, Moved, Ended, Began, Moved
            //
            // This seems to maintain a good balance between responsiveness vs accuracy. While
            // it should not (in theory) be possible to receive multiple Began or Ended events
            // in succession, if it happens in practice for whatever reason this is still safe.
            rawEventQueue.back() = rawTouchEvent;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::ProcessRawEventQueues()
    {
        // Process all raw input events that were queued since the last call to this function.
        ProcessRawInputEventQueues(m_rawTouchEventQueuesById, m_inputDevice.m_touchChannelsById);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
