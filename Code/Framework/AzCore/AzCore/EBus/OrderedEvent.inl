/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    template <typename... Params>
    OrderedEventHandler<Params...>::OrderedEventHandler(std::nullptr_t)
        : m_priority(0)
    {
        ;
    }


    template <typename... Params>
    OrderedEventHandler<Params...>::OrderedEventHandler(Callback callback, int32_t priority /*= 0*/)
        : m_callback(AZStd::move(callback))
        , m_priority(priority)
    {
        ;
    }


    template <typename... Params>
    OrderedEventHandler<Params...>::OrderedEventHandler(const OrderedEventHandler& rhs)
        : m_callback(rhs.m_callback)
        , m_priority(rhs.m_priority)
    {
        // Copy the callback function, then perform a Connect with the new event
        if (rhs.m_event)
        {
            rhs.m_event->Connect(*this);
        }
    }


    template <typename... Params>
    OrderedEventHandler<Params...>::OrderedEventHandler(OrderedEventHandler&& rhs)
        : m_event(rhs.m_event)
        , m_index(rhs.m_index)
        , m_priority(rhs.m_priority)
        , m_callback(AZStd::move(rhs.m_callback))
    {
        // Moves all of the data of the r-value handle, fixup the event to point to them, and revert the r-value handle to it's original construction state
        rhs.m_event = nullptr;
        rhs.m_index = 0;
        rhs.m_priority = 0;

        SwapEventHandlerPointers(rhs);
    }


    template <typename... Params>
    OrderedEventHandler<Params...>::~OrderedEventHandler()
    {
        Disconnect();
    }


    template <typename... Params>
    OrderedEventHandler<Params...>& OrderedEventHandler<Params...>::operator=(const OrderedEventHandler& rhs)
    {
        // Copy the callback function, then perform a Connect with the new event
        if (this != &rhs)
        {
            Disconnect();
            m_callback = rhs.m_callback;
            m_priority = rhs.m_priority;
            if (rhs.m_event)
            {
                rhs.m_event->Connect(*this);
            }
        }

        return *this;
    }


    template <typename... Params>
    OrderedEventHandler<Params...>& OrderedEventHandler<Params...>::operator=(OrderedEventHandler&& rhs)
    {
        if (this != &rhs)
        {
            Disconnect();
            // Moves all of the data of the r-value handle, fixup the event to point to them, and revert the r-value handle to it's original construction state
            m_event = rhs.m_event;
            m_index = rhs.m_index;
            m_priority = rhs.m_priority;
            m_callback = AZStd::move(rhs.m_callback);

            rhs.m_event = nullptr;
            rhs.m_index = 0;
            rhs.m_priority = 0;

            SwapEventHandlerPointers(rhs);
        }
        return *this;
    }


    template <typename... Params>
    void OrderedEventHandler<Params...>::Connect(OrderedEvent<Params...>& event)
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
    void OrderedEventHandler<Params...>::Disconnect()
    {
        if (m_event)
        {
            m_event->Disconnect(*this);
        }
    }


    template <typename... Params>
    bool OrderedEventHandler<Params...>::IsConnected() const
    {
        return m_event != nullptr;
    }


    template <typename... Params>
    void OrderedEventHandler<Params...>::SwapEventHandlerPointers([[maybe_unused]] const OrderedEventHandler& from)
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
    OrderedEvent<Params...>::OrderedEvent(OrderedEvent&& rhs)
        : m_handlers(AZStd::move(rhs.m_handlers))
        , m_addList(AZStd::move(rhs.m_addList))
        , m_updating(rhs.m_updating)
    {
        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        BindHandlerEventPointers();
        rhs.m_updating = false;
    }


    template <typename... Params>
    OrderedEvent<Params...>::~OrderedEvent()
    {
        DisconnectAllHandlers();
    }


    template <typename... Params>
    OrderedEvent<Params...>& OrderedEvent<Params...>::operator=(OrderedEvent&& rhs)
    {
        // Remove all previous handles which will update them as needed
        // Move all sub-objects into this event and fixup each handle to point to this event
        // Revert the r-value event to it's default state (the moves should do it but PODs need to be set)
        DisconnectAllHandlers();

        m_handlers = AZStd::move(rhs.m_handlers);
        m_addList = AZStd::move(rhs.m_addList);
        m_updating = rhs.m_updating;

        BindHandlerEventPointers();

        rhs.m_updating = false;

        return *this;
    }


    template <typename... Params>
    bool OrderedEvent<Params...>::HasHandlerConnected() const
    {
        for (OrderedEventHandler<Params...>* handler : m_handlers)
        {
            if (handler)
            {
                return true;
            }
        }

        return false;
    }


    template <typename... Params>
    void OrderedEvent<Params...>::DisconnectAllHandlers()
    {
        // Clear all our added handlers
        for (OrderedEventHandler<Params...>* handler : m_handlers)
        {
            if (handler)
            {
                AZ_Assert(handler->m_event == this, "Entry event does not match");
                handler->Disconnect();
            }
        }

        // Clear any handlers still pending registration
        for (OrderedEventHandler<Params...>* handler : m_addList)
        {
            AZ_Assert(handler, "NULL handler encountered in Event addList");
            AZ_Assert(handler->m_event == this, "Entry event does not match");
            handler->Disconnect();
        }
    }


    template <typename... Params>
    void OrderedEvent<Params...>::Signal(const Params&... params) const
    {
        m_updating = true;

        // Trigger all added handler callbacks
        for (OrderedEventHandler<Params...>* handler : m_handlers)
        {
            if (handler)
            {
                handler->m_callback(params...);
            }
        }

        // Update our handlers if we have pending adds
        if (!m_addList.empty())
        {
            for (OrderedEventHandler<Params...>* handler : m_addList)
            {
                InsertHandler((*handler));
            }
            //fixup indexes as something was added
            UpdateHandlerIndexes(0);
            m_addList.clear();
        }

        m_updating = false;
    }


    template <typename... Params>
    inline void OrderedEvent<Params...>::BindHandlerEventPointers()
    {
        for (OrderedEventHandler<Params...>* handler : m_handlers)
        {
            if (handler)
            {
                // This should have happened as part of a move so none of the pointers should refer to this event (they should also all refer to the same event)
                AZ_Assert(handler->m_event != this, "Should not refer to this");
                handler->m_event = this;
            }
        }

        for (OrderedEventHandler<Params...>* handler : m_addList)
        {
            // This should have happened as part of a move so none of the pointers should refer to this event (they should also all refer to the same event)
            AZ_Assert(handler, "NULL handler encountered in Event addList");
            AZ_Assert(handler->m_event != this, "Should not refer to this");
            handler->m_event = this;
        }
    }


    template <typename... Params>
    inline void OrderedEvent<Params...>::Connect(OrderedEventHandler<Params...>& handler) const
    {
        if (m_updating)
        {
            handler.m_index = -aznumeric_cast<int32_t>(m_addList.size() + 1);
            m_addList.push_back(&handler);
            return;
        }

        //insert the new handler
        int32_t newHandleIdx = InsertHandler(handler);

        //update all indexes that follow
        UpdateHandlerIndexes(newHandleIdx);
    }


    template <typename... Params>
    inline void OrderedEvent<Params...>::Disconnect(OrderedEventHandler<Params...>& eventHandle) const
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
        }

        eventHandle.m_event = nullptr;
    }

    template <typename... Params>
    int32_t OrderedEvent<Params...>::InsertHandler(OrderedEventHandler<Params...>& handler) const
    {
        //find the sorted location to insert.
        auto insertLocation = AZStd::upper_bound(m_handlers.begin(), m_handlers.end(), handler.m_priority,
            [](int32_t newHandlerPriority, OrderedEventHandler<Params...>* handler)
            {
                if (handler != nullptr)
                {
                    return newHandlerPriority > handler->m_priority;
                }
                return false; //if the handler in the list is null, skip it
            });

        //if not inserting at the beginning AND the previous index is null, just replace the null with the new handler.
        if (insertLocation != m_handlers.begin() && *(AZStd::prev(insertLocation)) == nullptr)
        {
            handler.m_index = aznumeric_cast<int32_t>(AZStd::distance(m_handlers.begin(), AZStd::prev(insertLocation)));
            AZ_Assert(m_handlers[handler.m_index] == nullptr, "Replacing non nullptr event");
            m_handlers[handler.m_index] = &handler;
            return handler.m_index;
        }

        //insert the new handler
        handler.m_index = aznumeric_cast<int32_t>(AZStd::distance(m_handlers.begin(), insertLocation));
        m_handlers.insert(insertLocation, &handler);
        return handler.m_index;
    }

    template <typename... Params>
    void OrderedEvent<Params...>::UpdateHandlerIndexes(int32_t startIndex) const
    {
        for (int32_t i = startIndex; i < m_handlers.size(); i++)
        {
            if (m_handlers[i])
            {
                m_handlers[i]->m_index = i;
            }
        }
    }
}
