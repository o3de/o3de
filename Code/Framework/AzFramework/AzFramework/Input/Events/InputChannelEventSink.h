/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Buses/Notifications/InputChannelNotificationBus.h>
#include <AzFramework/Input/Events/InputChannelEventFilter.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that consumes all input event that pass the specified filter.
    class InputChannelEventSink : public InputChannelNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputChannelEventSink();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] filter The filter used to determine whether an inut event should be consumed
        explicit InputChannelEventSink(AZStd::shared_ptr<InputChannelEventFilter> filter);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelEventSink);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannelEventSink() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::GetPriority
        AZ::s32 GetPriority() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Allow the filter to be set as necessary even if already connected to the input event bus
        //! \param[in] filter The filter used to determine whether an inut event should be consumed
        void SetFilter(AZStd::shared_ptr<InputChannelEventFilter> filter);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelNotifications::OnInputChannelEvent
        void OnInputChannelEvent(const InputChannel& inputChannel, bool& o_hasBeenConsumed) final;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::shared_ptr<InputChannelEventFilter> m_filter; //!< The shared input event filter
    };
} // namespace AzFramework
