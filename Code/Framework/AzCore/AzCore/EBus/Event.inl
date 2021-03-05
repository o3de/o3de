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

namespace AZ
{
    template <typename... Params>
    Event<Params...>::Handler::Handler(std::nullptr_t)
    {
        ;
    }


    template <typename... Params>
    Event<Params...>::Handler::Handler(Event<Params...>::Callback callback)
        : m_callback(AZStd::move(callback))
    {
        ;
    }


    template <typename... Params>
    Event<Params...>::Handler::Handler(const Handler& rhs)
        : m_callback(rhs.m_callback)
    {
        // Copy the callback function, then perform a Connect with the new event
        if (rhs.m_event)
        {
            rhs.m_event->Connect(*this);
        }
    }


    template <typename... Params>
    Event<Params...>::Handler::Handler(Handler&& rhs)
        : m_event(rhs.m_event)
        , m_index(rhs.m_index)
        , m_callback(AZStd::move(rhs.m_callback))
    {
        // Moves all of the data of the r-value handle, fixup the event to point to them, and revert the r-value handle to it's original construction state
        rhs.m_event = nullptr;
        rhs.m_index = 0;

        SwapEventHandlerPointers(rhs);
    }


    template <typename... Params>
    Event<Params...>::Handler::~Handler()
    {
        Disconnect();
    }


    template <typename... Params>
    typename Event<Params...>::Handler& Event<Params...>::Handler::operator=(const Handler& rhs)
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


    template <typename... Params>
    typename Event<Params...>::Handler& Event<Params...>::Handler::operator=(Handler&& rhs)
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


    template <typename... Params>
    void Event<Params...>::Handler::Connect(Event<Params...>& event)
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


    template <typename... Params>
    void Event<Params...>::Handler::Disconnect()
    {
        if (m_event)
        {
            m_event->Disconnect(*this);
        }
    }


    template <typename... Params>
    bool Event<Params...>::Handler::IsConnected() const
    {
        return m_event != nullptr;
    }


    template <typename... Params>
    void Event<Params...>::Handler::SwapEventHandlerPointers([[maybe_unused]] const Handler& from)
    {
        // Find the pointer to the 'from' handler and point it to this handler
        if (m_event)
        {
            // The index is negative if the handle is in the pending add list
            // The index can then be converted to the add list index in which it lives
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


    template <typename... Params>
    Event<Params...>::Event(Event&& rhs)
        : m_handlers(AZStd::move(rhs.m_handlers))
        , m_addList(AZStd::move(rhs.m_addList))
        , m_freeList(AZStd::move(rhs.m_freeList))
        , m_updating(rhs.m_updating)
    {
        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        BindHandlerEventPointers();
        rhs.m_updating = false;
    }


    template <typename... Params>
    Event<Params...>::~Event()
    {
        DisconnectAllHandlers();
    }


    template <typename... Params>
    Event<Params...>& Event<Params...>::operator=(Event&& rhs)
    {
        // Remove all previous handles which will update them as needed
        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        DisconnectAllHandlers();

        m_handlers = AZStd::move(rhs.m_handlers);
        m_addList = AZStd::move(rhs.m_addList);
        m_freeList = AZStd::move(rhs.m_freeList);
        m_updating = rhs.m_updating;

        BindHandlerEventPointers();

        rhs.m_updating = false;

        return *this;
    }


    template <typename... Params>
    bool Event<Params...>::HasHandlerConnected() const
    {
        for (Handler* handler : m_handlers)
        {
            if (handler)
            {
                return true;
            }
        }

        return false;
    }


    template <typename... Params>
    void Event<Params...>::DisconnectAllHandlers()
    {
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
    }


    template <typename... Params>
    void Event<Params...>::Signal(const Params&... params) const
    {
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


    template <typename... Params>
    inline void Event<Params...>::BindHandlerEventPointers()
    {
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


    template <typename... Params>
    inline void Event<Params...>::Connect(Handler& handler) const
    {
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


    template <typename... Params>
    inline void Event<Params...>::Disconnect(Handler& eventHandle) const
    {
        AZ_Assert(eventHandle.m_event == this, "Trying to remove a handler bound to a different event");

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
