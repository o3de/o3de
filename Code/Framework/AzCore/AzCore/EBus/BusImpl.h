/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file
 * Header file for internal EBus classes.
 * For more information about EBuses, see AZ::EBus and AZ::EBusTraits in this guide and
 * [Event Bus](http://docs.aws.amazon.com/lumberyard/latest/developerguide/asset-pipeline-ebus.html)
 * in the *Open 3D Engine Developer Guide*.
 */

#pragma once

#include <AzCore/Debug/Trace.h>

#include <AzCore/EBus/Internal/BusContainer.h>
#include <AzCore/EBus/Internal/Debug.h>
#include <AzCore/EBus/Policies.h>

#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    /**
     * A dummy mutex that performs no locking.
     * EBuses that do not support multithreading use this mutex
     * as their EBusTraits::MutexType.
     */
    struct NullMutex
    {
        void lock() {}
        bool try_lock() { return true; }
        void unlock() {}
    };


    /**
     * Indicates that EBusTraits::BusIdType is not set.
     * EBuses with multiple addresses must set the EBusTraits::BusIdType.
     */
    struct NullBusId
    {
        NullBusId() {};
        NullBusId(int) {};
    };

    /// @cond EXCLUDE_DOCS
    inline bool operator==(const NullBusId&, const NullBusId&) { return true; }
    inline bool operator!=(const NullBusId&, const NullBusId&) { return false; }
    /// @endcond

    /**
     * Indicates that EBusTraits::BusIdOrderCompare is not set.
     * EBuses with ordered address IDs must specify a function for
     * EBusTraits::BusIdOrderCompare.
     */
    struct NullBusIdCompare;

    namespace Internal
    {
        // Lock guard used when there is a NullMutex on a bus, or during dispatch
        // on a bus which supports LocklessDispatch.
        template <class Lock>
        struct NullLockGuard
        {
            explicit NullLockGuard(Lock&) {}
            NullLockGuard(Lock&, AZStd::adopt_lock_t) {}

            void lock() {}
            bool try_lock() { return true; }
            void unlock() {}
        };
    }

    namespace BusInternal
    {

        /**
         * Internal class that contains data about EBusTraits.
         * @tparam Interface A class whose virtual functions define the events that are
         *                   dispatched or received by the EBus.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter is optional if the `Interface` class inherits from
         *                   EBusTraits.
         */
        template <class Interface, class BusTraits>
        struct EBusImplTraits
        {
            /**
             * Properties that you use to configure an EBus.
             * For more information, see EBusTraits.
             */
            using Traits = BusTraits;

            /**
             * Allocator used by the EBus.
             * The default setting is AZStd::allocator, which uses AZ::SystemAllocator.
             */
            using AllocatorType = typename Traits::AllocatorType;

            /**
             * The class that defines the interface of the EBus.
             */
            using InterfaceType = Interface;

            /**
             * The events defined by the EBus interface.
             */
            using Events = Interface;

            /**
             * The type of ID that is used to address the EBus.
             * Used only when the address policy is AZ::EBusAddressPolicy::ById
             * or AZ::EBusAddressPolicy::ByIdAndOrdered.
             * The type must support `AZStd::hash<ID>` and
             * `bool operator==(const ID&, const ID&)`.
             */
            using BusIdType = typename Traits::BusIdType;

            /**
             * Sorting function for EBus address IDs.
             * Used only when the AddressPolicy is AZ::EBusAddressPolicy::ByIdAndOrdered.
             * If an event is dispatched without an ID, this function determines
             * the order in which each address receives the event.
             *
             * The following example shows a sorting function that meets these requirements.
             * @code{.cpp}
             * using BusIdOrderCompare = AZStd::less<BusIdType>; // Lesser IDs first.
             * @endcode
             */
            using BusIdOrderCompare = typename Traits::BusIdOrderCompare;

            /**
             * Locking primitive that is used when connecting handlers to the EBus or executing events.
             * By default, all access is assumed to be single threaded and no locking occurs.
             * For multithreaded access, specify a mutex of the following type.
             * - For simple multithreaded cases, use AZStd::mutex.
             * - For multithreaded cases where an event handler sends a new event on the same bus
             *   or connects/disconnects while handling an event on the same bus, use AZStd::recursive_mutex.
             */
            using MutexType = typename Traits::MutexType;

            /**
             * Contains all of the addresses on the EBus.
             */
            using BusesContainer = AZ::Internal::EBusContainer<Interface, Traits>;

            /**
             * Locking primitive that is used when executing events in the event queue.
             */
            using EventQueueMutexType = AZStd::conditional_t<AZStd::is_same<typename Traits::EventQueueMutexType, NullMutex>::value, // if EventQueueMutexType==NullMutex use MutexType otherwise EventQueueMutexType
                MutexType, typename Traits::EventQueueMutexType>;

            /**
             * Pointer to an address on the bus.
             */
            using BusPtr = typename BusesContainer::BusPtr;

            /**
             * Pointer to a handler node.
             */
            using HandlerNode = typename BusesContainer::HandlerNode;

            /**
             * Specifies whether the EBus supports an event queue.
             * You can use the event queue to execute events at a later time.
             * To execute the queued events, you must call
             * `<BusName>::ExecuteQueuedEvents()`.
             * By default, the event queue is disabled.
             */
            static constexpr bool EnableEventQueue = Traits::EnableEventQueue;
            static constexpr bool EventQueueingActiveByDefault = Traits::EventQueueingActiveByDefault;
            static constexpr bool EnableQueuedReferences = Traits::EnableQueuedReferences;

            /**
             * True if the EBus supports more than one address. Otherwise, false.
             */
            static constexpr bool HasId = Traits::AddressPolicy != EBusAddressPolicy::Single;

            /**
            * Template Lock Guard class that wraps around the Mutex
            * The EBus uses for Dispatching Events.
            * This is not the EBus Context Mutex if LocklessDispatch is true
            */
            template <typename DispatchMutex>
            using DispatchLockGuard = typename Traits::template DispatchLockGuard<DispatchMutex, Traits::LocklessDispatch>;
        };

        /**
         * Dispatches events to handlers that are connected to a specific address on an EBus.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusEventer
        {
            /**
             * The type of ID that is used to address the EBus.
             * Used only when the address policy is AZ::EBusAddressPolicy::ById
             * or AZ::EBusAddressPolicy::ByIdAndOrdered.
             * The type must support `AZStd::hash<ID>` and
             * `bool operator==(const ID&, const ID&)`.
             */
            using BusIdType = typename Traits::BusIdType;

            /**
             * Pointer to an address on the bus.
             */
            using BusPtr = typename Traits::BusPtr;

            /**
             * An event handler that can be attached to multiple addresses.
             */
            using MultiHandler = typename Traits::BusesContainer::MultiHandler;

            /**
             * Acquires a pointer to an EBus address.
             * @param[out] ptr A pointer that will point to the specified address
             * on the EBus. An address lookup can be avoided by calling Event() with
             * this pointer rather than by passing an ID, but that is only recommended
             * for performance-critical code.
             * @param id The ID of the EBus address that the pointer will be bound to.
             */
            static void Bind(BusPtr& ptr, const BusIdType& id);
        };

        /**
         * Provides functionality that requires enumerating over handlers that are connected
         * to an EBus. It can enumerate over all handlers or just the handlers that are connected
         * to a specific address on an EBus.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusEventEnumerator
        {
            /**
             * The type of ID that is used to address the EBus.
             * Used only when the address policy is AZ::EBusAddressPolicy::ById
             * or AZ::EBusAddressPolicy::ByIdAndOrdered.
             * The type must support `AZStd::hash<ID>` and
             * `bool operator==(const ID&, const ID&)`.
             */
            using BusIdType = typename Traits::BusIdType;

            /**
             * Pointer to an address on the bus.
             */
            using BusPtr = typename Traits::BusPtr;

            /**
             * Finds the first handler that is connected to a specific address on the EBus.
             * This function is only for special cases where you know that a particular
             * component's handler is guaranteed to exist.
             * Even if the returned pointer is valid (not null), it might point to a handler
             * that was deleted. Prefer dispatching events using EBusEventer.
             * @param id        Address ID.
             * @return          A pointer to the first handler on the EBus, even if the handler
             *                  was deleted.
             */
            static typename Traits::InterfaceType* FindFirstHandler(const BusIdType& id);

            /**
             * Finds the first handler at a cached address on the EBus.
             * This function is only for special cases where you know that a particular
             * component's handler is guaranteed to exist.
             * Even if the returned pointer is valid (not null), it might point to a handler
             * that was deleted. Prefer dispatching events using EBusEventer.
             * @param ptr       Cached address ID.
             * @return          A pointer to the first handler on the specified EBus address,
             *                  even if the handler was deleted.
             */
            static typename Traits::InterfaceType* FindFirstHandler(const BusPtr& ptr);

            /**
             * Returns the total number of event handlers that are connected to a specific
             * address on the EBus.
             * @param id    Address ID.
             * @return      The total number of handlers that are connected to the EBus address.
             */
            static size_t GetNumOfEventHandlers(const BusIdType& id);
        };

        /**
         * Dispatches an event to all handlers that are connected to an EBus.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusBroadcaster
        {
            /**
             * An event handler that can be attached to only one address at a time.
             */
            using Handler = typename Traits::BusesContainer::Handler;
        };

        /**
         * Data type that is used when an EBus doesn't support queuing.
         */
        struct EBusNullQueue
        {
        };

        /**
         * EBus functionality related to the queuing of events and functions.
         * This is specifically for queuing events and functions that will
         * be broadcast to all handlers on the EBus.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusBroadcastQueue
        {
            /**
             * Executes queued events and functions.
             * Execution will occur on the thread that calls this function.
             * @see QueueBroadcast(), EBusEventQueue::QueueEvent(), QueueFunction(), ClearQueuedEvents()
             */
            static void ExecuteQueuedEvents()
            {
                if (auto* context = Bus::GetContext())
                {
                    context->m_queue.Execute();
                }
            }

            /**
             * Clears the queue without calling events or functions.
             * Use in situations where memory must be freed immediately, such as shutdown.
             * Use with care. Cleared queued events will never be executed, and those events might have been expected.
             */
            static void ClearQueuedEvents()
            {
                if (auto* context = Bus::GetContext(false))
                {
                    context->m_queue.Clear();
                }
            }

            static size_t QueuedEventCount()
            {
                if (auto* context = Bus::GetContext(false))
                {
                    return context->m_queue.Count();
                }
                return 0;
            }

            /**
             * Sets whether function queuing is allowed.
             * This does not affect event queuing.
             * Function queuing is allowed by default when EBusTraits::EnableEventQueue
             * is true. It is never allowed when EBusTraits::EnableEventQueue is false.
             * @see QueueFunction, EBusTraits::EnableEventQueue
             * @param isAllowed Set to true to allow function queuing. Otherwise, set to false.
             */
            static void AllowFunctionQueuing(bool isAllowed) { Bus::GetOrCreateContext().m_queue.SetActive(isAllowed); }

            /**
             * Returns whether function queuing is allowed.
             * @return True if function queuing is allowed. Otherwise, false.
             * @see QueueFunction, AllowFunctionQueuing
             */
            static bool IsFunctionQueuing()
            {
                auto* context = Bus::GetContext();
                return context ? context->m_queue.IsActive() : Traits::EventQueueingActiveByDefault;
            }

            /**
            * Helper to Queue a Broadcast only when function queueing is available
            * @param func          Function pointer of the event to dispatch.
            * @param args          Function arguments that are passed to each handler.
            */
            template <class Function, class ... InputArgs>
            static void TryQueueBroadcast(Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to all handlers.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueBroadcast(Function&& func, InputArgs&& ... args);

            /**
            * Helper to Queue a Reverse Broadcast only when function queueing is available
            * @param func          Function pointer of the event to dispatch.
            * @param args          Function arguments that are passed to each handler.
            */
            template <class Function, class ... InputArgs>
            static void TryQueueBroadcastReverse(Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to all handlers in reverse order.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueBroadcastReverse(Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an arbitrary callable function to be executed asynchronously.
             * The function is not executed until ExecuteQueuedEvents() is called.
             * The function might be unrelated to this EBus or any handlers.
             * Examples of callable functions are static functions, lambdas, and
             * bound-member functions.
             *
             * One use case is to determine when a batch of queued events has finished.
             * When the function is executed, we know that all events that were queued
             * before the function have finished executing.
             *
             * @param func Callable function.
             * @param args Arguments for `func`.
             */
            template <class Function, class ... InputArgs>
            static void QueueFunction(Function&& func, InputArgs&& ... args);
        };

        /**
         * Enqueues asynchronous events to dispatch to handlers that are connected to
         * a specific address on an EBus.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusEventQueue
            : public EBusBroadcastQueue<Bus, Traits>
        {

            /**
             * The type of ID that is used to address the EBus.
             * Used only when the address policy is AZ::EBusAddressPolicy::ById
             * or AZ::EBusAddressPolicy::ByIdAndOrdered.
             * The type must support `AZStd::hash<ID>` and
             * `bool operator==(const ID&, const ID&)`.
             */
            using BusIdType = typename Traits::BusIdType;

            /**
             * Pointer to an address on the bus.
             */
            using BusPtr = typename Traits::BusPtr;

            /**
             * Helper to queue an event by BusIdType only when function queueing is enabled
             * @param id            Address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void TryQueueEvent(const BusIdType& id, Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to handlers at a specific address.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param id            Address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueEvent(const BusIdType& id, Function&& func, InputArgs&& ... args);

            /**
             * Helper to queue an event by BusPtr only when function queueing is enabled
             * @param ptr           Cached address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void TryQueueEvent(const BusPtr& ptr, Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to handlers at a cached address.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param ptr           Cached address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueEvent(const BusPtr& ptr, Function&& func, InputArgs&& ... args);

            /**
             * Helper to queue an event in reverse by BusIdType only when funciton queueing is enabled
             * @param id            Address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void TryQueueEventReverse(const BusIdType& id, Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param id            Address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueEventReverse(const BusIdType& id, Function&& func, InputArgs&& ... args);

            /**
             * Helper to queue an event in reverse by BusPtr only when function queueing is enabled
             * @param ptr           Cached address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void TryQueueEventReverse(const BusPtr& ptr, Function&& func, InputArgs&& ... args);

            /**
             * Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
             * The event is not executed until ExecuteQueuedEvents() is called.
             * @param ptr           Cached address ID. Handlers that are connected to this ID will receive the event.
             * @param func          Function pointer of the event to dispatch.
             * @param args          Function arguments that are passed to each handler.
             */
            template <class Function, class ... InputArgs>
            static void QueueEventReverse(const BusPtr& ptr, Function&& func, InputArgs&& ... args);
        };

        /**
         * Provides functionality that requires enumerating over all handlers that are
         * connected to an EBus.
         * To enumerate over handlers that are connected to a specific address
         * on the EBus, use a function from EBusEventEnumerator.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusBroadcastEnumerator
        {
            /**
             * Finds the first handler that is connected to the EBus.
             * This function is only for special cases where you know that a particular
             * component's handler is guaranteed to exist.
             * Even if the returned pointer is valid (not null), it might point to a handler
             * that was deleted. Prefer dispatching events using EBusEventer.
             * @return          A pointer to the first handler on the EBus, even if the handler
             *                  was deleted.
             */
            static typename Traits::InterfaceType* FindFirstHandler();
        };

        // This alias is required because you're not allowed to inherit from a nested type.
        template <typename Bus, typename Traits>
        using EventDispatcher = typename Traits::BusesContainer::template Dispatcher<Bus>;

        /**
         * Base class that provides eventing, queueing, and enumeration functionality
         * for EBuses that dispatch events to handlers. Supports accessing handlers
         * that are connected to specific addresses.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         * @tparam BusIdType The type of ID that is used to address the EBus.
         */
        template <class Bus, class Traits, class BusIdType>
        struct EBusImpl
            : public EventDispatcher<Bus, Traits>
            , public EBusBroadcaster<Bus, Traits>
            , public EBusEventer<Bus, Traits>
            , public EBusEventEnumerator<Bus, Traits>
            , public AZStd::conditional_t<Traits::EnableEventQueue, EBusEventQueue<Bus, Traits>, EBusNullQueue>
        {
        };

        /**
         * Base class that provides eventing, queueing, and enumeration functionality
         * for EBuses that dispatch events to all of their handlers.
         * For a base class that can access handlers at specific addresses, use EBusImpl.
         * @tparam Bus       The EBus type.
         * @tparam Traits    A class that inherits from EBusTraits and configures the EBus.
         *                   This parameter may be left unspecified if the `Interface` class
         *                   inherits from EBusTraits.
         */
        template <class Bus, class Traits>
        struct EBusImpl<Bus, Traits, NullBusId>
            : public EventDispatcher<Bus, Traits>
            , public EBusBroadcaster<Bus, Traits>
            , public EBusBroadcastEnumerator<Bus, Traits>
            , public AZStd::conditional_t<Traits::EnableEventQueue, EBusBroadcastQueue<Bus, Traits>, EBusNullQueue>
        {
        };

        template <class Bus, class Traits>
        inline void EBusEventer<Bus, Traits>::Bind(BusPtr& ptr, const BusIdType& id)
        {
            auto& context = Bus::GetOrCreateContext();
            AZStd::scoped_lock<decltype(context.m_contextMutex)> lock(context.m_contextMutex);
            context.m_buses.Bind(ptr, id);
        }

        template <class Bus, class Traits>
        typename Traits::InterfaceType * EBusEventEnumerator<Bus, Traits>::FindFirstHandler(const BusIdType& id)
        {
            typename Traits::InterfaceType* result = nullptr;
            Bus::EnumerateHandlersId(id, [&result](typename Traits::InterfaceType* handler)
            {
                result = handler;
                return false;
            });
            return result;
        }

        template <class Bus, class Traits>
        typename Traits::InterfaceType * EBusEventEnumerator<Bus, Traits>::FindFirstHandler(const BusPtr& ptr)
        {
            typename Traits::InterfaceType* result = nullptr;
            Bus::EnumerateHandlersPtr(ptr, [&result](typename Traits::InterfaceType* handler)
            {
                result = handler;
                return false;
            });
            return result;
        }

        template <class Bus, class Traits>
        size_t EBusEventEnumerator<Bus, Traits>::GetNumOfEventHandlers(const BusIdType& id)
        {
            size_t size = 0;
            Bus::EnumerateHandlersId(id, [&size](typename Bus::InterfaceType*)
            {
                ++size;
                return true;
            });
            return size;
        }

        namespace Internal
        {
            template <class Function, bool AllowQueuedReferences>
            struct QueueFunctionArgumentValidator
            {
                static void Validate() {}
            };

            template <class Function>
            struct ArgumentValidatorHelper
            {
                constexpr static void Validate()
                {
                    ValidateHelper(AZStd::make_index_sequence<AZStd::function_traits<Function>::num_args>());
                }

                template<typename T>
                using is_non_const_lvalue_reference = AZStd::integral_constant<bool, AZStd::is_lvalue_reference<T>::value && !AZStd::is_const<AZStd::remove_reference_t<T>>::value>;

                template <size_t... ArgIndices>
                constexpr static void ValidateHelper(AZStd::index_sequence<ArgIndices...>)
                {
                    static_assert(!AZStd::disjunction_v<is_non_const_lvalue_reference<AZStd::function_traits_get_arg_t<Function, ArgIndices>>...>,
                        "It is not safe to queue a function call with non-const lvalue ref arguments");
                }
            };

            template <class Function>
            struct QueueFunctionArgumentValidator<Function, false>
            {
                using Validator = ArgumentValidatorHelper<Function>;
                constexpr static void Validate()
                {
                    Validator::Validate();
                }
            };
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::TryQueueEvent(const BusIdType& id, Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueueing())
            {
                EBusEventQueue<Bus, Traits>::QueueEvent(id, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::QueueEvent(const BusIdType& id, Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Eventer = void(*)(const BusIdType&, Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Eventer>(&Bus::Event), id, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::TryQueueEvent(const BusPtr& ptr, Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueueing())
            {
                EBusEventQueue<Bus, Traits>::QueueEvent(ptr, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::QueueEvent(const BusPtr& ptr, Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Eventer = void(*)(const BusPtr&, Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Eventer>(&Bus::Event), ptr, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::TryQueueEventReverse(const BusIdType& id, Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueueing())
            {
                EBusEventQueue<Bus, Traits>::QueueEventReverse(id, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::QueueEventReverse(const BusIdType& id, Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Eventer = void(*)(const BusIdType&, Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Eventer>(&Bus::EventReverse), id, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::TryQueueEventReverse(const BusPtr& ptr, Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueueing())
            {
                EBusEventQueue<Bus, Traits>::QueueEventReverse(ptr, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusEventQueue<Bus, Traits>::QueueEventReverse(const BusPtr& ptr, Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Eventer = void(*)(const BusPtr&, Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Eventer>(&Bus::EventReverse), ptr, AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

        template <class Bus, class Traits>
        typename Traits::InterfaceType * EBusBroadcastEnumerator<Bus, Traits>::FindFirstHandler()
        {
            typename Traits::InterfaceType* result = nullptr;
            Bus::EnumerateHandlers([&result](typename Traits::InterfaceType* handler)
            {
                result = handler;
                return false;
            });
            return result;
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusBroadcastQueue<Bus, Traits>::TryQueueBroadcast(Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueuing())
            {
                EBusEventQueue<Bus, Traits>::QueueBroadcast(AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusBroadcastQueue<Bus, Traits>::QueueBroadcast(Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Broadcaster = void(*)(Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Broadcaster>(&Bus::Broadcast), AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusBroadcastQueue<Bus, Traits>::TryQueueBroadcastReverse(Function&& func, InputArgs&& ... args)
        {
            if (EBusEventQueue<Bus, Traits>::IsFunctionQueuing())
            {
                EBusEventQueue<Bus, Traits>::QueueBroadcastReverse(AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
            }
        }

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusBroadcastQueue<Bus, Traits>::QueueBroadcastReverse(Function&& func, InputArgs&& ... args)
        {
            Internal::QueueFunctionArgumentValidator<Function, Traits::EnableQueuedReferences>::Validate();
            using Broadcaster = void(*)(Function&&, InputArgs&&...);
            Bus::QueueFunction(static_cast<Broadcaster>(&Bus::BroadcastReverse), AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
        }

#undef EBUS_DO_ROUTING

        template <class Bus, class Traits>
        template <class Function, class ... InputArgs>
        inline void EBusBroadcastQueue<Bus, Traits>::QueueFunction(Function&& func, InputArgs&& ... args)
        {
            static_assert((AZStd::is_same<typename Bus::QueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::value == false),
                "This EBus doesn't support queued events! Check 'EnableEventQueue'");

            auto& context = Bus::GetOrCreateContext(false);
            if (context.m_queue.IsActive())
            {
                AZStd::scoped_lock<decltype(context.m_queue.m_messagesMutex)> messageLock(context.m_queue.m_messagesMutex);
                context.m_queue.m_messages.push(typename Bus::QueuePolicy::BusMessageCall(
                    [func = AZStd::forward<Function>(func), args...]() mutable
                {
                    AZStd::invoke(AZStd::forward<Function>(func), AZStd::forward<InputArgs>(args)...);
                },
                    typename Traits::AllocatorType()));
            }
            else
            {
                AZ_Warning("EBus", false, "Unable to queue function onto EBus.  This may be due to a previous call to AllowFunctionQueuing(false)."
                    "  Hint: This is often disabled during shutdown of a ComponentApplication");
            }
        }
    } // namespace BusInternal
} // namespace AZ
