/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputChannel;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for input events as they're broadcast from input channels when
    //! they are active or when their state/value changes. Most common input consumers should derive
    //! instead from InputChannelEventListener (which respects the 'o_hasBeenConsumed' parameter for
    //! OnInputChannelEvent) to ensure events are only processed once. However, if a system needs to
    //! process input events that may have already been consumed by a higher priority listener, they
    //! are free to derive from InputChannelNotificationBus::Handler and ignore 'o_hasBeenConsumed'.
    class InputChannelNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications can be handled by multiple (ordered) listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelNotifications() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when an input channel is active or its state or value is updated
        //! \param[in] inputChannel The input channel that is active or whose state or value updated
        //! \param[in,out] o_hasBeenConsumed Check and/or set whether the event has been handled
        virtual void OnInputChannelEvent(const InputChannel& /*inputChannel*/,
                                         bool& /*o_hasBeenConsumed*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the priority of the input notification handler (sorted from highest to lowest)
        //! \return Priority of the input notification handler
        virtual AZ::s32 GetPriority() const { return 0; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        //! \param[in] other Another instance of the class to compare
        //! \return True if the priority of this handler is greater than the other, false otherwise
        inline bool Compare(const InputChannelNotifications* other) const
        {
            return GetPriority() > other->GetPriority();
        }
    };
    using InputChannelNotificationBus = AZ::EBus<InputChannelNotifications>;
} // namespace AzFramework
