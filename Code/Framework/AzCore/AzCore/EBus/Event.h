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

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    //! A specialized type of EBus useful for eventing on a specific event type
    //! Whereas with EBus you would have to define a bus, implement the bus, and manage connecting and disconnecting from the bus
    //! Event only requires you declare an Event<>, and then connect Event<>::Handlers
    //! Note that this system does not provide *any* thread safety, Handler Connect and Disconnect must happen on the same thread dispatching Events
    //! It is safe to connect or disconnect handlers during a signal, in this case we don't guarantee the signal will be dispatched to the disconnected handler
    //! Example Usage:
    //! @code{.cpp}
    //!      {
    //!          Event<int32_t> event; // An Event that can event a single int32_t
    //!          Event<int32_t>::Handler handler([](int32_t value) { DO_SOMETHING_WITH_VALUE(value); });
    //!          handler.Connect(event); // Our handler is now connected to the Event
    //!          event.Signal(1); // Our handlers lambda will now get invoked with the value 1
    //!      };
    //! @endcode
    template <typename... Params>
    class Event final
    {
    public:

        using Callback = AZStd::function<void(Params...)>;

        //! A handler class that can connect to an Event
        class Handler final
        {
            friend class Event;

        public:

            // We support default constructing of event handles (with no callback function being bound) to allow for better usage with container types
            // An unbound event handle cannot be added to an event and we do not support dynamically binding the callback post construction
            // (except for on assignment since that will also add the handle to the event; i.e. there is no way to unbind the callback after being added to an event)
            Handler() = default;
            explicit Handler(std::nullptr_t);
            explicit Handler(Callback callback);
            Handler(const Handler& rhs);
            Handler(Handler&& rhs);

            ~Handler();

            Handler& operator=(const Handler& rhs);
            Handler& operator=(Handler&& rhs);

            //! Connects the handler to the provided event.
            //! @param event the Event to connect to
            void Connect(Event<Params...>& event);

            //! Disconnects the handler from its connected event, does nothing if the event is not connected.
            void Disconnect();

            //! Returns true if this handler is connected to an event.
            //! @return boolean true if this handler is connected to an event
            bool IsConnected() const;

        private:

            //! Swaps the event handler pointers from the from instance to this instance
            //! @param from the handler instance we are replacing on the attached event
            void SwapEventHandlerPointers(const Handler& from);

            const Event<Params...>* m_event = nullptr; //< The connected event
            int32_t m_index = 0; //< Index into the add or handler vectors (negative means pending add)
            Callback m_callback; //< The lambda to invoke during events
        };

        Event() = default;
        Event(Event&& rhs);
        Event(const Event& rhs) = delete; // Cannot copy events

        ~Event();

        Event& operator=(Event&& rhs);
        Event& operator=(const Event& rhs) = delete; // Cannot copy events

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

        void Connect(Handler& handler) const;
        void Disconnect(Handler& handler) const;

    private:

        // Note that these are mutable because we want Signal() to be const, but we do a bunch of book-keeping during Signal()
        mutable AZStd::vector<Handler*> m_handlers; //< Active handlers
        mutable AZStd::vector<Handler*> m_addList; //< Handlers added during a Signal, pending being added to the active handlers
        mutable AZStd::stack<size_t> m_freeList; //< Set of unused handler indices

        mutable bool m_updating = false; //< Raised during a Signal, false otherwise, used to guard m_handlers during handler iteration
    };
}

#include <AzCore/EBus/Event.inl>
