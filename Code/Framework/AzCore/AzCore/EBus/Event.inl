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

namespace AZ::Internal
{
    template<bool ThreadSafe, typename... Params>
    EventHandler<ThreadSafe, Params...>::EventHandler(std::nullptr_t)
    {
        ;
    }


    template<bool ThreadSafe, typename... Params>
    EventHandler<ThreadSafe, Params...>::EventHandler(Callback callback)
        : m_callback(AZStd::move(callback))
    {
        ;
    }


    template<bool ThreadSafe, typename... Params>
    EventHandler<ThreadSafe, Params...>::EventHandler(const EventHandler& rhs)
        : m_callback(rhs.m_callback)
    {
        // Copy the callback function, then perform a Connect with the new event
        if (rhs.m_event)
        {
            rhs.m_event->Connect(*this);
        }
    }


    template<bool ThreadSafe, typename... Params>
    EventHandler<ThreadSafe, Params...>::EventHandler(EventHandler&& rhs)
        : m_event(rhs.m_event)
        , m_index(rhs.m_index)
        , m_callback(AZStd::move(rhs.m_callback))
    {
        // Moves all of the data of the r-value handle, fixup the event to point to them, and revert the r-value handle to it's original construction state
        rhs.m_event = nullptr;
        rhs.m_index = 0;

        SwapEventHandlerPointers(rhs);
    }


    template<bool ThreadSafe, typename... Params>
    EventHandler<ThreadSafe, Params...>::~EventHandler()
    {
        Disconnect();
    }


    template<bool ThreadSafe, typename... Params>
    auto EventHandler<ThreadSafe, Params...>::operator=(const EventHandler& rhs) -> EventHandler&
    {
        // Copy the callback function, then perform a Connect with the new event
        if (this != &rhs)
        {
            Disconnect();
            m_callback = rhs.m_callback;
            if (rhs.m_event)
            {
                rhs.m_event->Connect(*this);
            }
        }

        return *this;
    }


    template<bool ThreadSafe, typename... Params>
    auto EventHandler<ThreadSafe, Params...>::operator=(EventHandler&& rhs) -> EventHandler& 
    {
        if (this != &rhs)
        {
            Disconnect();
            // Moves all of the data of the r-value handle, fixup the event to point to them, and revert the r-value handle to it's original construction state
            m_event = rhs.m_event;
            m_index = rhs.m_index;
            m_callback = AZStd::move(rhs.m_callback);

            rhs.m_event = nullptr;
            rhs.m_index = 0;

            SwapEventHandlerPointers(rhs);
        }

        return *this;
    }


    template<bool ThreadSafe, typename... Params>
    void EventHandler<ThreadSafe, Params...>::Connect(EventType& event)
    {
        // Cannot add an unbound event handle (no function callback) to an event, this is a programmer error
        // We explicitly do not support binding the callback after the handler has been constructed so we can just reject the event handle here
        AZ_Assert(m_callback, "Handler callback is null");
        if (!m_callback)
        {
            return;
        }

        AZ_Assert(!m_event, "Handler is already registered to an event, binding a handler to multiple events is unsupported");
        m_event = &event;

        event.Connect(*this);
    }


    template<bool ThreadSafe, typename... Params>
    void EventHandler<ThreadSafe, Params...>::Disconnect()
    {
        if (m_event)
        {
            m_event->Disconnect(*this);
        }
    }

    template<bool ThreadSafe, typename... Params>
    bool EventHandler<ThreadSafe, Params...>::IsConnected() const
    {
        return m_event != nullptr;
    }

    template<bool ThreadSafe, typename... Params>
    void EventHandler<ThreadSafe, Params...>::SwapEventHandlerPointers([[maybe_unused]] const EventHandler& from)
    {
        // Find the pointer to the 'from' handler and point it to this handler
        if (m_event)
        {
            // The index is negative if the handle is in the pending add list
            // The index can then be converted to the add list index in which it lives
            EventType::ScopedLock handlersLock(m_event->m_handlersMutex);
            if (m_index < 0)
            {
                AZ_Assert(m_event->m_addList[-(m_index + 1)] == &from, "From handle does not match");
                m_event->m_addList[-(m_index + 1)] = this;
            }
            else
            {
                AZ_Assert(m_event->m_handlers[m_index] == &from, "From handle does not match");
                m_event->m_handlers[m_index] = this;
            }
        }
    }


    template<bool ThreadSafe, typename... Params>
    Event<ThreadSafe, Params...>::Event(Event&& rhs)
        : m_handlers(AZStd::move(rhs.m_handlers))
        , m_addList(AZStd::move(rhs.m_addList))
        , m_freeList(AZStd::move(rhs.m_freeList))
        , m_updating(rhs.m_updating)
    {
        ScopedLock handlersLock(m_handlersMutex);

        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        BindHandlerEventPointers();
        rhs.m_updating = false;
    }


    template<bool ThreadSafe, typename... Params>
    Event<ThreadSafe, Params...>::~Event()
    {
        DisconnectAllHandlers();
    }


    template<bool ThreadSafe, typename... Params>
    auto Event<ThreadSafe, Params...>::operator=(Event&& rhs) -> Event&
    {
        ScopedLock handlersLock(m_handlersMutex);

        // Remove all previous handles which will update them as needed
        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        DisconnectAllHandlers_impl();

        m_handlers = AZStd::move(rhs.m_handlers);
        m_addList = AZStd::move(rhs.m_addList);
        m_freeList = AZStd::move(rhs.m_freeList);
        m_updating = rhs.m_updating;

        BindHandlerEventPointers();

        rhs.m_updating = false;

        return *this;
    }


    template<bool ThreadSafe, typename... Params>
    bool Event<ThreadSafe, Params...>::HasHandlerConnected() const
    {
        ScopedLock handlersLock(m_handlersMutex);

        for (Handler* handler : m_handlers)
        {
            if (handler)
            {
                return true;
            }
        }

        return false;
    }


    template<bool ThreadSafe, typename... Params>
    void Event<ThreadSafe, Params...>::DisconnectAllHandlers()
    {
        ScopedLock handlersLock(m_handlersMutex);
        DisconnectAllHandlers_impl();
    }

    template<bool ThreadSafe, typename... Params>
    void Event<ThreadSafe, Params...>::DisconnectAllHandlers_impl()
    {
        // Callers of this function are responsible for thread safety.

        // Clear all our added handlers
        for (Handler* handler : m_handlers)
        {
            if (handler)
            {
                AZ_Assert(handler->m_event == this, "Entry event does not match");
                handler->Disconnect();
            }
        }

        // Clear any handlers still pending registration
        for (Handler* handler : m_addList)
        {
            AZ_Assert(handler, "NULL handler encountered in Event addList");
            AZ_Assert(handler->m_event == this, "Entry event does not match");
            handler->Disconnect();
        }

        // Free up any owned memory
        AZStd::vector<Handler*> freeHandlers;
        m_handlers.swap(freeHandlers);
        AZStd::vector<Handler*> freeAdds;
        m_addList.swap(freeAdds);
        AZStd::stack<size_t> freeFree;
        m_freeList.swap(freeFree);
    }


    template<bool ThreadSafe, typename... Params>
    void Event<ThreadSafe, Params...>::Signal(const Params&... params) const
    {
        ScopedLock handlersLock(m_handlersMutex);

        m_updating = true;
        
        // Trigger all added handler callbacks
        for (Handler* handler : m_handlers)
        {
            if (handler)
            {
                handler->m_callback(params...);
            }
        }

        // Update our handlers if we have pending adds
        if (!m_addList.empty())
        {
            for (Handler* handler : m_addList)
            {
                AZ_Assert(handler, "NULL handler encountered in Event addList");
                if (m_freeList.empty())
                {
                    handler->m_index = aznumeric_cast<int32_t>(m_handlers.size());
                    m_handlers.push_back(handler);
                }
                else
                {
                    handler->m_index = aznumeric_cast<int32_t>(m_freeList.top());
                    m_freeList.pop();

                    AZ_Assert(m_handlers[handler->m_index] == nullptr, "Callback already registered");
                    m_handlers[handler->m_index] = handler;
                }
            }
            m_addList.clear();
        }

        m_updating = false;
    }


    template<bool ThreadSafe, typename... Params>
    inline void Event<ThreadSafe, Params...>::BindHandlerEventPointers()
    {
        // Callers of this function are responsible for thread-safety.

        for (Handler* handler : m_handlers)
        {
            if (handler)
            {
                // This should have happened as part of a move so none of the pointers should refer to this event (they should also all refer to the same event)
                AZ_Assert(handler->m_event != this, "Should not refer to this");
                handler->m_event = this;
            }
        }

        for (Handler* handler : m_addList)
        {
            // This should have happened as part of a move so none of the pointers should refer to this event (they should also all refer to the same event)
            AZ_Assert(handler, "NULL handler encountered in Event addList");
            AZ_Assert(handler->m_event != this, "Should not refer to this");
            handler->m_event = this;
        }
    }


    template<bool ThreadSafe, typename... Params>
    inline void Event<ThreadSafe, Params...>::Connect(Handler& handler) const
    {
        ScopedLock handlersLock(m_handlersMutex);

        if (m_updating)
        {
            handler.m_index = -aznumeric_cast<int32_t>(m_addList.size() + 1);
            m_addList.push_back(&handler);
            return;
        }

        if (m_freeList.empty())
        {
            handler.m_index = aznumeric_cast<int32_t>(m_handlers.size());
            m_handlers.push_back(&handler);
        }
        else
        {
            handler.m_index = aznumeric_cast<int32_t>(m_freeList.top());
            m_freeList.pop();

            AZ_Assert(m_handlers[handler.m_index] == nullptr, "Replacing non nullptr event");
            m_handlers[handler.m_index] = &handler;
        }
    }


    template<bool ThreadSafe, typename... Params>
    inline void Event<ThreadSafe, Params...>::Disconnect(Handler& eventHandle) const
    {
        AZ_Assert(eventHandle.m_event == this, "Trying to remove a handler bound to a different event");

        ScopedLock handlersLock(m_handlersMutex);

        int32_t index = eventHandle.m_index;
        if (index < 0)
        {
            AZ_Assert(m_addList[-(index + 1)] == &eventHandle, "Entry does not refer to handle");
            m_addList[-(index + 1)] = nullptr;
        }
        else
        {
            AZ_Assert(m_handlers[index] == &eventHandle, "Entry does not refer to handle");
            m_handlers[index] = nullptr;
            m_freeList.push(index);
        }

        eventHandle.m_event = nullptr;
    }
}
