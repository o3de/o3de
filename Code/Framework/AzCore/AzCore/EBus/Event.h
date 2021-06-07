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
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/typetraits/conditional.h>

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
    //! A thread-safe version is also available. This behaves in the same way as the regular Event but allows safe (dis)connecting from
    //! multiple threads. Keep in mind that this also means that calls to Signal can come from different threads and that there's no
    //! guarantee that a signal will come from the same thread as was used to connect to the Event.
    //! @code{.cpp}
    //!      {
    //!          ThreadSafeEvent<int32_t> event;
    //!          ThreadSafeEvent<int32_t>::Handler handler([](int32_t value) { DO_SOMETHING_WITH_VALUE(value); });
    //!          handler.Connect(event); // Called from thread X
    //!          event.Signal(1); // Can be called from thread X or another thread.
    //!      };
    //! @endcode

    namespace Internal
    {
        //! A fake mutex that can be used in case thread safety isn't an issue.
        class PlaceholderMutex
        {
            AZ_TYPE_INFO(PlaceholderMutex, "{F7B4432F-567D-4A33-80E4-E47C5F2AFC34}");

            void lock() {}
            bool try_lock() { return true; }
            void unlock() {}
        };

        //! A fake scoped lock for a fake mutex.
        struct PlaceholderScopedLock
        {
            PlaceholderScopedLock(PlaceholderMutex&){}
        };

        template<bool ThreadSafe, typename... Params>
        class Event;

        template<bool ThreadSafe, typename... Params>
        class EventHandler;

        template<bool ThreadSafe, typename... Params>
        class Event final
        {
            friend class EventHandler<ThreadSafe, Params...>; // This is required in order to allow for relocating the handle pointers on a move

        public:
            using Callback = AZStd::function<void(Params...)>;
            using Handler = EventHandler<ThreadSafe, Params...>;
            using Mutex = AZStd::conditional_t<ThreadSafe, AZStd::recursive_mutex, PlaceholderMutex>;
            using ScopedLock = AZStd::conditional_t<ThreadSafe, AZStd::scoped_lock<Mutex>, PlaceholderScopedLock>;
            
            AZ_CLASS_ALLOCATOR(Event, AZ::SystemAllocator, 0);

            Event() = default;
            Event(Event&& rhs);

            ~Event();

            Event& operator=(Event&& rhs);

            //! Returns true if at least one handler is connected to this event.
            bool HasHandlerConnected() const;

            //! Disconnects all connected handlers.
            void DisconnectAllHandlers();

            //! Signal an event.
            //! @param params variadic set of event parameters
            void Signal(const Params&... params) const;

        private:
            //! Used internally to rebind all handlers from an old event to the current event instance. This call is not thread-safe.
            void BindHandlerEventPointers();

            void Connect(Handler& handler) const;
            void Disconnect(Handler& handler) const;

            //! Main implementation of DisconnectAllHandlers that is not thread-safe.
            void DisconnectAllHandlers_impl();

        private:
            // Note that these are mutable because we want Signal() to be const, but we do a bunch of book-keeping during Signal()
            mutable AZStd::vector<Handler*> m_handlers; //< Active handlers
            mutable AZStd::vector<Handler*> m_addList; //< Handlers added during a Signal, pending being added to the active handlers
            mutable AZStd::stack<size_t> m_freeList; //< Set of unused handler indices

            mutable Mutex m_handlersMutex; //< Synchronization primitive for thread safe use of handlers.
            mutable bool m_updating = false; //< Raised during a Signal, false otherwise, used to guard m_handlers during handler iteration
        };

        //! A handler class that can connect to an Event
        template<bool ThreadSafe, typename... Params>
        class EventHandler final
        {
            friend class Event<ThreadSafe, Params...>;

        public:
            using Callback = AZStd::function<void(Params...)>;
            using EventType = Event<ThreadSafe, Params...>;

            AZ_CLASS_ALLOCATOR(EventHandler, AZ::SystemAllocator, 0);

            // We support default constructing of event handles (with no callback function being bound) to allow for better usage with
            // container types An unbound event handle cannot be added to an event and we do not support dynamically binding the callback
            // post construction (except for on assignment since that will also add the handle to the event; i.e. there is no way to unbind
            // the callback after being added to an event)
            EventHandler() = default;
            explicit EventHandler(std::nullptr_t);
            explicit EventHandler(Callback callback);
            EventHandler(const EventHandler& rhs);
            EventHandler(EventHandler&& rhs);

            ~EventHandler();

            EventHandler& operator=(const EventHandler& rhs);
            EventHandler& operator=(EventHandler&& rhs);

            //! Connects the handler to the provided event.
            //! @param event the Event to connect to
            void Connect(EventType& event);

            //! Disconnects the handler from its connected event, does nothing if the event is not connected.
            void Disconnect();

            //! Returns true if this handler is connected to an event.
            //! @return boolean true if this handler is connected to an event
            bool IsConnected() const;

        private:
            //! Swaps the event handler pointers from the from instance to this instance
            //! @param from the handler instance we are replacing on the attached event
            void SwapEventHandlerPointers(const EventHandler& from);

            Callback m_callback; //< The lambda to invoke during events
            const EventType* m_event = nullptr; //< The connected event
            int32_t m_index = 0; //< Index into the add or handler vectors (negative means pending add)
        };
    } // namespace Internal

    template<typename... Params>
    using Event = Internal::Event<false, Params...>;
    template<typename... Params>
    using EventHandler = Internal::EventHandler<false, Params...>;

    template<typename... Params>
    using ThreadSafeEvent = Internal::Event<true, Params...>;
    template<typename... Params>
    using ThreadSafeEventHandler = Internal::EventHandler<true, Params...>;

    AZ_TYPE_INFO_TEMPLATE(Event, "{B7388760-18BF-486A-BE96-D5765791C53C}", AZ_TYPE_INFO_TYPENAME_VARARGS);
    AZ_TYPE_INFO_TEMPLATE(EventHandler, "{F85EFDA5-FBD0-4557-A3EF-9E077B41EA59}", AZ_TYPE_INFO_TYPENAME_VARARGS);
    AZ_TYPE_INFO_TEMPLATE(ThreadSafeEvent, "{11C12655-B0EC-4E09-967E-98100B17EA4F}", AZ_TYPE_INFO_TYPENAME_VARARGS);
    AZ_TYPE_INFO_TEMPLATE(ThreadSafeEventHandler, "{BBC2FFE7-460C-43C2-810C-C7DEB6ACFBFB}", AZ_TYPE_INFO_TYPENAME_VARARGS);
}

#include <AzCore/EBus/Event.inl>
