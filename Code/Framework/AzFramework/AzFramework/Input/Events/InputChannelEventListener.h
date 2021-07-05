/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Buses/Notifications/InputChannelNotificationBus.h>
#include <AzFramework/Input/Events/InputChannelEventFilter.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

// Start: fix for windows defining max/min macros
#pragma push_macro("max")
#pragma push_macro("min")
#undef max
#undef min

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that handles input notifications by priority, and that allows events to be filtered by
    //! their channel name, device name, device index (local player) or any combination of the three.
    class InputChannelEventListener : public InputChannelNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Predefined input event listener priority, used to sort handlers from highest to lowest
        inline static AZ::s32 GetPriorityFirst()    { return std::numeric_limits<AZ::s32>::max(); }
        inline static AZ::s32 GetPriorityDebug()    { return (GetPriorityFirst() / 4) * 3;        }
        inline static AZ::s32 GetPriorityUI()       { return GetPriorityFirst() / 2;              }
        inline static AZ::s32 GetPriorityDefault()  { return 0;                                   }
        inline static AZ::s32 GetPriorityLast()     { return std::numeric_limits<AZ::s32>::min(); }
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputChannelEventListener();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other input event listeners
        explicit InputChannelEventListener(AZ::s32 priority);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other input event listeners
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(AZ::s32 priority, bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        explicit InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        //! \param[in] priority The priority used to sort relative to other input event listeners
        explicit InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                           AZ::s32 priority);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        //! \param[in] priority The priority used to sort relative to other input event listeners
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                           AZ::s32 priority,
                                           bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventListener);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelEventListener() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::GetPriority
        AZ::s32 GetPriority() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Allow the filter to be set as necessary even if already connected to the input event bus
        //! \param[in] filter The filter used to determine whether an inut event should be handled
        void SetFilter(AZStd::shared_ptr<InputChannelEventFilter> filter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Connect to the input notification bus to start receiving input notifications
        void Connect();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Disconnect from the input notification bus to stop receiving input notifications
        void Disconnect();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::OnInputChannelEvent
        void OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed) final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when an input channel is active or its state or value is updated,
        //! unless the event was consumed by a higher priority listener, or did not pass the filter.
        //! \param[in] inputChannel The input channel that is active or whose state or value updated
        //! \return True if the input event has been consumed, false otherwise
        virtual bool OnInputChannelEventFiltered(const InputChannel& inputChannel) = 0;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::shared_ptr<InputChannelEventFilter> m_filter;   //!< The shared input event filter
        AZ::s32                                    m_priority; //!< The priority used for sorting
    };
} // namespace AzFramework

// End: fix for windows defining max/min macros
#pragma pop_macro("max")
#pragma pop_macro("min")
