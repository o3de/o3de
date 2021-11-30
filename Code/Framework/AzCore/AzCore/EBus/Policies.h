/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/**
 * @file
 * Header file for EBus policies regarding addresses, handlers, connections, and storage.
 * These are internal policies. Do not include this file directly.
 */

// Includes for the event queue.
#include <AzCore/std/functional.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/std/parallel/scoped_lock.h>


namespace AZ
{
    /**
     * Defines how many addresses exist on the EBus.
     */
    enum class EBusAddressPolicy
    {
        /**
         * (Default) The EBus has a single address.
         */
        Single,

        /**
         * The EBus has multiple addresses; the order in which addresses
         * receive events is undefined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         */
        ById,

        /**
         * The EBus has multiple addresses; the order in which addresses
         * receive events is defined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         * The order in which addresses receive events is defined by
         * AZ::EBusTraits::BusIdOrderCompare.
         */
        ByIdAndOrdered,
    };

    /**
     * Defines how many handlers can connect to an address on the EBus
     * and the order in which handlers at each address receive events.
     */
    enum class EBusHandlerPolicy
    {
        /**
         * The EBus supports one handler for each address.
         */
        Single,

        /**
         * (Default) Allows any number of handlers for each address;
         * handlers at an address receive events in the order
         * in which the handlers are connected to the EBus.
         */
        Multiple,

        /**
         * Allows any number of handlers for each address; the order
         * in which each address receives an event is defined
         * by AZ::EBusTraits::BusHandlerOrderCompare.
         */
        MultipleAndOrdered,
    };

    namespace Internal
    {
        struct NullBusMessageCall
        {
            template<class Function>
            NullBusMessageCall(Function) {}
            template<class Function, class Allocator>
            NullBusMessageCall(Function, const Allocator&) {}
        };
    } // namespace Internal

    /**
     * Defines the default connection policy that is used when
     * AZ::EBusTraits::ConnectionPolicy is left unspecified.
     * Use this as a template for custom connection policies.
     * @tparam Bus A class that defines an EBus.
     */
    template <class Bus>
    struct EBusConnectionPolicy
    {
        /**
         * A pointer to a specific address on the EBus.
         */
        typedef typename Bus::BusPtr BusPtr;

        /**
         * The type of ID that is used to address the EBus.
         * This type is used when the address policy is EBusAddressPolicy::ById
         * or EBusAddressPolicy::ByIdAndOrdered only.
         */
        typedef typename Bus::BusIdType BusIdType;

        /**
         * A handler connected to the EBus.
         */
        typedef typename Bus::HandlerNode HandlerNode;

        /**
         * Global data for the EBus.
         * There is only one context for each EBus type.
         */
        typedef typename Bus::Context Context;

        /**
        * Default MutexType used for connecting, disconnecting
        * and dispatch when enabled
        */
        typedef typename Bus::MutexType MutexType;

        /**
        * Lock type used for connecting to the bus.  When NullMutex isn't used as the
        * default mutex this will be a unique lock to allow for unlocking before connection
        * dispatches which some specialized policies perform
        *
        * E.g RequestConnect -> LockMutex -> ConnectInternal ->
        * UnlockMutex -> ExecuteHandlerMethod -> Return
        */
        typedef typename Bus::Context::ConnectLockGuard ConnectLockGuard;

        /**
         * Connects a handler to an EBus address.
         * @param ptr[out] A pointer that will be bound to the EBus address that
         * the handler will be connected to.
         * @param context Global data for the EBus.
         * @param handler The handler to connect to the EBus address.
         * @param id The ID of the EBus address that the handler will be connected to.
         */
        static void Connect(BusPtr& ptr, Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id = 0);

        /**
         * Disconnects a handler from an EBus address.
         * @param context Global data for the EBus.
         * @param handler The handler to disconnect from the EBus address.
         * @param ptr A pointer to a specific address on the EBus.
         */
        static void Disconnect(Context& context, HandlerNode& handler, BusPtr& ptr);
    };

    /**
     * A choice of AZ::EBusTraits::StoragePolicy that specifies
     * that EBus data is stored in a global static variable.
     * With this policy, each module (DLL) has its own instance of the EBus.
     * @tparam Context A class that contains EBus data.
     */
    template <class Context>
    struct EBusGlobalStoragePolicy
    {
        /**
         * Returns the EBus data.
         * @return A pointer to the EBus data.
         */
        static Context* Get()
        {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /**
         * Returns the EBus data.
         * @return A reference to the EBus data.
         */
        static Context& GetOrCreate()
        {
            static Context s_context;
            return s_context;
        }
    };

    /**
     * A choice of AZ::EBusTraits::StoragePolicy that specifies
     * that EBus data is stored in a thread_local static variable.
     * With this policy, each thread has its own instance of the EBus.
     * @tparam Context A class that contains EBus data.
     */
    template <class Context>
    struct EBusThreadLocalStoragePolicy
    {
        /**
         * Returns the EBus data.
         * @return A pointer to the EBus data.
         */
        static Context* Get()
        {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /**
         * Returns the EBus data.
         * @return A reference to the EBus data.
         */
        static Context& GetOrCreate()
        {
            thread_local static Context s_context;
            return s_context;
        }
    };

    template <bool IsEnabled, class Bus, class MutexType>
    struct EBusQueuePolicy
    {
        typedef AZ::Internal::NullBusMessageCall BusMessageCall;
        void Execute() {};
        void Clear() {};
        void SetActive(bool /*isActive*/) {};
        bool IsActive() { return false; }
        size_t Count() const { return 0; }
    };

    template <class Bus, class MutexType>
    struct EBusQueuePolicy<true, Bus, MutexType>
    {
        typedef AZStd::function<void()> BusMessageCall;

        typedef AZStd::deque<BusMessageCall, typename Bus::AllocatorType> DequeType;
        typedef AZStd::queue<BusMessageCall, DequeType > MessageQueueType;

        EBusQueuePolicy() = default;

        bool                        m_isActive = Bus::Traits::EventQueueingActiveByDefault;
        MessageQueueType            m_messages;
        MutexType                   m_messagesMutex;        ///< Used to control access to the m_messages. Make sure you never interlock with the EBus mutex. Otherwise, a deadlock can occur.

        void Execute()
        {
            AZ_Warning("System", m_isActive, "You are calling execute queued functions on a bus which has not activated its function queuing! Call YourBus::AllowFunctionQueuing(true)!");

            MessageQueueType localMessages;

            // Swap the current list of queue functions with a local instance
            {
                AZStd::scoped_lock lock(m_messagesMutex);
                AZStd::swap(localMessages, m_messages);
            }

            // Execute the queue functions safely now that are owned by the function
            while (!localMessages.empty())
            {
                const BusMessageCall& localMessage = localMessages.front();
                localMessage();
                localMessages.pop();
            }
        }

        void Clear()
        {
            AZStd::lock_guard<MutexType> lock(m_messagesMutex);
            m_messages = {};
        }

        void SetActive(bool isActive)
        {
            AZStd::lock_guard<MutexType> lock(m_messagesMutex);
            m_isActive = isActive;
            if (!m_isActive)
            {
                m_messages = {};
            }
        };

        bool IsActive()
        {
            return m_isActive;
        }

        size_t Count()
        {
            AZStd::lock_guard<MutexType> lock(m_messagesMutex);
            return m_messages.size();
        }
    };

    /// @endcond

    ////////////////////////////////////////////////////////////
    // Implementations
    ////////////////////////////////////////////////////////////
    template <class Bus>
    void EBusConnectionPolicy<Bus>::Connect(BusPtr&, Context&, HandlerNode&, ConnectLockGuard&, const BusIdType&)
    {
    }

    template <class Bus>
    void EBusConnectionPolicy<Bus>::Disconnect(Context&, HandlerNode&, BusPtr&)
    {
    }

    /**
     * This is the default bus event compare operator. If not overridden in your bus traits and you
     * use ordered handlers (HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered), you will need
     * to declare the following function in your interface: 'bool Compare(const Interface* rhs) const'.
     */
    struct BusHandlerCompareDefault;

    //////////////////////////////////////////////////////////////////////////
    // Router Policy
    /// @cond EXCLUDE_DOCS
    template <class Interface>
    struct EBusRouterNode
        : public AZStd::intrusive_multiset_node<EBusRouterNode<Interface>>
    {
        Interface*  m_handler = nullptr;
        int m_order = 0;

        EBusRouterNode& operator=(Interface* handler);

        Interface* operator->() const;

        operator Interface*() const;

        bool operator<(const EBusRouterNode& rhs) const;
    };

    template <class Bus>
    struct EBusRouterPolicy
    {
        using RouterNode = EBusRouterNode<typename Bus::InterfaceType>;
        typedef AZStd::intrusive_multiset<RouterNode, AZStd::intrusive_multiset_base_hook<RouterNode>> Container;

        // We have to share this with a general processing event if we want to support stopping messages in progress.
        enum class EventProcessingState : int
        {
            ContinueProcess, ///< Continue with the event process as usual (default).
            SkipListeners, ///< Skip all listeners but notify all other routers.
            SkipListenersAndRouters, ///< Skip everybody. Nobody should receive the event after this.
        };

        template <class Function, class... InputArgs>
        inline bool RouteEvent(const typename Bus::BusIdType* busIdPtr, bool isQueued, bool isReverse, Function&& func, InputArgs&&... args);

        Container m_routers;
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline EBusRouterNode<Interface>& EBusRouterNode<Interface>::operator=(Interface* handler)
    {
        m_handler = handler;
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline Interface* EBusRouterNode<Interface>::operator->() const
    {
        return m_handler;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline EBusRouterNode<Interface>::operator Interface*() const
    {
        return m_handler;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    bool EBusRouterNode<Interface>::operator<(const EBusRouterNode& rhs) const
    {
        return m_order < rhs.m_order;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template <class Bus>
    template <class Function, class... InputArgs>
    inline bool EBusRouterPolicy<Bus>::RouteEvent(const typename Bus::BusIdType* busIdPtr, bool isQueued, bool isReverse, Function&& func, InputArgs&&... args)
    {
        auto rtLast = m_routers.end();
        typename Bus::RouterCallstackEntry rtCurrent(m_routers.begin(), busIdPtr, isQueued, isReverse);
        while (rtCurrent.m_iterator != rtLast)
        {
            AZStd::invoke(func, (*rtCurrent.m_iterator++), args...);

            if (rtCurrent.m_processingState == EventProcessingState::SkipListenersAndRouters)
            {
                return true;
            }
        }

        if (rtCurrent.m_processingState != EventProcessingState::ContinueProcess)
        {
            return true;
        }

        return false;
    }

    /// @endcond
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    struct EBusEventProcessingPolicy
    {
        template<class Results, class Function, class Interface, class... InputArgs>
        static void CallResult(Results& results, Function&& func, Interface&& iface, InputArgs&&... args)
        {
            results = AZStd::invoke(AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
        }

        template<class Function, class Interface, class... InputArgs>
        static void Call(Function&& func, Interface&& iface, InputArgs&&... args)
        {
            AZStd::invoke(AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
        }
    };


} // namespace AZ
