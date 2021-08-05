/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>

namespace AZ
{
    template <typename... Params>
    class OrderedEventHandler;

    //! A specialized type of EBus useful for eventing on a specific event type where the handlers are invoked based on priority.
    //! Note that this system does not provide *any* thread safety, Handler Connect and Disconnect must happen on the same thread dispatching Events
    //! It is safe to connect or disconnect handlers during a signal, in this case we don't guarantee the signal will be dispatched to the disconnected handler
    //! Example Usage:
    //! @code{.cpp}
    //!      {
    //!          OrderedEvent<int32_t> event; // An Event that can event a single int32_t.
    //!          int32_t handlerPriority = 100; //give the handler a priority, higher values will be signaled before lower values.
    //!          OrderedEvent<int32_t>::Handler handler([](int32_t value) { DO_SOMETHING_WITH_VALUE(value); }, handlerPriority);
    //!          handler.Connect(event); // Our handler is now connected to the Event
    //!          event.Signal(1); // Our handlers lambda will now get invoked with the value 1
    //!      };
    //! @endcode
    template <typename... Params>
    class OrderedEvent final
    {
        friend class OrderedEventHandler<Params...>; // Required to allow for the Handler to invoke the private Connect/Disconnect functions
    public:

        OrderedEvent() = default;
        OrderedEvent(OrderedEvent&& rhs);
        OrderedEvent(const OrderedEvent& rhs) = delete; // Cannot copy events

        ~OrderedEvent();

        OrderedEvent& operator=(OrderedEvent&& rhs);
        OrderedEvent& operator=(const OrderedEvent& rhs) = delete; // Cannot copy events

        //! Returns true if at least one handler is connected to this event.
        bool HasHandlerConnected() const;

        //! Disconnects all connected handlers.
        void DisconnectAllHandlers();

        //! Signal an event.
        //! @param params variadic set of event parameters
        void Signal(const Params&... params) const;

    private:

        //! Used internally to rebind all handlers from an old event to the current event instance
        void BindHandlerEventPointers();

        void Connect(OrderedEventHandler<Params...>& handler) const;
        void Disconnect(OrderedEventHandler<Params...>& handler) const;

        // helper function to insert a new handler
        // return the index the handle was inserted at
        // indexes for other handlers will need to be updated
        int32_t InsertHandler(OrderedEventHandler<Params...>& handler) const;
        void UpdateHandlerIndexes(int32_t startIndex) const;
    private:
        // Note that these are mutable because we want Signal() to be const, but we do a bunch of book-keeping during Signal()
        mutable AZStd::vector<OrderedEventHandler<Params...>*> m_handlers; //!< Active handlers
        mutable AZStd::vector<OrderedEventHandler<Params...>*> m_addList; //!< Handlers added during a Signal, pending being added to the active handlers

        mutable bool m_updating = false; //!< Raised during a Signal, false otherwise, used to guard m_handlers during handler iteration
    };

    
    //! A handler class that can connect to an Event that will be called in order based on priority
    template <typename... Params>
    class OrderedEventHandler final
    {
        friend class OrderedEvent<Params...>;

    public:
        using Callback = AZStd::function<void(Params...)>;

        // We support default constructing of event handles (with no callback function being bound) to allow for better usage with container types
        // An unbound event handle cannot be added to an event and we do not support dynamically binding the callback post construction
        // (except for on assignment since that will also add the handle to the event; i.e. there is no way to unbind the callback after being added to an event)
        OrderedEventHandler() = default;
        explicit OrderedEventHandler(std::nullptr_t);
        explicit OrderedEventHandler(Callback callback, int32_t priority = 0);
        OrderedEventHandler(const OrderedEventHandler& rhs);
        OrderedEventHandler(OrderedEventHandler&& rhs);

        ~OrderedEventHandler();

        OrderedEventHandler& operator=(const OrderedEventHandler& rhs);
        OrderedEventHandler& operator=(OrderedEventHandler&& rhs);

        //! Connects the handler to the provided event.
        //! @param event the Event to connect to
        void Connect(OrderedEvent<Params...>& event);

        //! Disconnects the handler from its connected event, does nothing if the event is not connected.
        void Disconnect();

        //! Returns true if the Handler is currently connected to an event.
            //! @return boolean true if this handler is connected to an event
        bool IsConnected() const;

    private:

        //! Swaps the event handler pointers from the from instance to this instance
        //! @param from the handler instance we are replacing on the attached event
        void SwapEventHandlerPointers(const OrderedEventHandler& from);

        const OrderedEvent<Params...>* m_event = nullptr; //< The connected event
        int32_t m_index = 0; //!< Index into the add or handler vectors (negative means pending add)
        int32_t m_priority = 0; //!< Priority of the handler.
        Callback m_callback; //!< The lambda to invoke during events
    };
}

#include <AzCore/EBus/OrderedEvent.inl>
