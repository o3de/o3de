/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file
 * Header file for event bus (EBus), a general-purpose communication system
 * that Open 3D Engine uses to dispatch notifications and receive requests.
 * EBuses are configurable and support many different use cases.
 * For more information about %EBuses, see AZ::EBus in this guide and
 * [Event Bus](https://o3de.org/docs/user-guide/engine/ebus/)
 * in the *Open 3D Engine Developer Guide*.
 */

#pragma once

#include <AzCore/EBus/BusImpl.h>
#include <AzCore/EBus/Environment.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/EBus/Internal/Debug.h>

#include <AzCore/std/typetraits/is_same.h>

#include <AzCore/std/utils.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZStd
{
    class recursive_mutex; // Forward declare for a static assert.
}

namespace AZ
{
    /**
     * %EBusTraits are properties that you use to configure an EBus.
     *
     * The key %EBusTraits to understand are #AddressPolicy, which defines
     * how many addresses the EBus contains, #HandlerPolicy, which
     * describes how many handlers can connect to each address, and
     * `BusIdType`, which is the type of ID that is used to address the EBus
     * if addresses are used.
     *
     * For example, if you want an EBus that makes requests of game objects
     * that each have a unique integer identifier, then define an EBus with
     * the following traits:
     * @code{.cpp}
     *      // The EBus has multiple addresses and each event is addressed to a
     *      // specific ID (the game object's ID), which corresponds to an address
     *      // on the bus.
     *      static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
     *
     *      // Each event is received by a single handler (the game object).
     *      static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
     *
     *      // Events are addressed by this type of ID (the game object's ID).
     *      using BusIdType = int;
     * @endcode
     *
     * For more information about %EBuses, see EBus in this guide and
     * [Event Bus](https://o3de.org/docs/user-guide/engine/ebus/)
     * in the *Open 3D Engine Developer Guide*.
     */
    struct EBusTraits
    {
    protected:

        /**
         * Note - the destructor is intentionally not virtual to avoid adding vtable overhead to every EBusTraits derived class.
         */
        ~EBusTraits() = default;

    public:
        /**
         * Allocator used by the EBus.
         * The default setting is Internal EBusEnvironmentAllocator
         * EBus code stores their Context instances in static memory
         * Therfore the configured allocator must last as long as the EBus in a module
         */
        using AllocatorType = AZ::Internal::EBusEnvironmentAllocator;

        /**
         * Defines how many handlers can connect to an address on the EBus
         * and the order in which handlers at each address receive events.
         * For available settings, see AZ::EBusHandlerPolicy.
         * By default, an EBus supports any number of handlers.
         */
        static constexpr EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;

        /**
         * Defines how many addresses exist on the EBus.
         * For available settings, see AZ::EBusAddressPolicy.
         * By default, an EBus uses a single address.
         */
        static constexpr EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;

        /**
         * The type of ID that is used to address the EBus.
         * Used only when the #AddressPolicy is AZ::EBusAddressPolicy::ById
         * or AZ::EBusAddressPolicy::ByIdAndOrdered.
         * The type must support `AZStd::hash<ID>` and
         * `bool operator==(const ID&, const ID&)`.
         */
        using BusIdType = NullBusId;

        /**
         * Sorting function for EBus address IDs.
         * Used only when the #AddressPolicy is AZ::EBusAddressPolicy::ByIdAndOrdered.
         * If an event is dispatched without an ID, this function determines
         * the order in which each address receives the event.
         *
         * The following example shows a sorting function that meets these requirements.
         * @code{.cpp}
         * using BusIdOrderCompare = AZStd::less<BusIdType>; // Lesser IDs first.
         * @endcode
         */
        using BusIdOrderCompare = NullBusIdCompare;

        /**
         * Sorting function for EBus event handlers.
         * Used only when the #HandlerPolicy is AZ::EBusHandlerPolicy::MultipleAndOrdered.
         * This function determines the order in which handlers at an address receive an event.
         *
         * By default, the function requires the handler to implement the following comparison
         * function.
         * @code{.cpp}
         * // Returns whether 'this' should precede 'other'.
         * bool Compare(const Interface* other) const;
         * @endcode
         */
        using BusHandlerOrderCompare = BusHandlerCompareDefault;

        /**
         * Locking primitive that is used when connecting handlers to the EBus or executing events.
         * By default, all access is assumed to be single threaded and no locking occurs.
         * For multithreaded access, specify a mutex of the following type.
         * - For simple multithreaded cases, use AZStd::mutex.
         * - For multithreaded cases where an event handler sends a new event on the same bus
         *   or connects/disconnects while handling an event on the same bus, use AZStd::recursive_mutex.
         */
        using MutexType = NullMutex;

        /**
         * Specifies whether the EBus supports an event queue.
         * You can use the event queue to execute events at a later time.
         * To execute the queued events, you must call
         * `<BusName>::ExecuteQueuedEvents()`.
         * By default, the event queue is disabled.
         */
        static constexpr bool EnableEventQueue = false;

        /**
         * Specifies whether the bus should accept queued messages by default or not.
         * If set to false, Bus::AllowFunctionQueuing(true) must be called before events are accepted.
         * Used only when #EnableEventQueue is true.
         */
        static constexpr bool EventQueueingActiveByDefault = true;

        /**
         * Specifies whether the EBus supports queueing functions which take reference
         * arguments. This means that the sender is responsible for the lifetime of the
         * arguments (they should be static or class members or otherwise persistently stored).
         * You should only use this if you know that the data being passed as arguments will
         * outlive the dispatch of the queued event.
         */
        static constexpr bool EnableQueuedReferences = false;

        /**
         * Locking primitive that is used when adding and removing
         * events from the queue.
         * Not used for connection or event execution.
         * Used only when #EnableEventQueue is true.
         * If left unspecified, it will use the #MutexType.
         */
        using EventQueueMutexType = NullMutex;

        /**
         * Enables custom logic to run when a handler connects or
         * disconnects from the EBus.
         * For example, you can make a handler execute an event
         * immediately upon connecting to the EBus by modifying the
         * EBusConnectionPolicy of the bus.
         * By default, no extra logic is run.
         */
        template <class Bus>
        using ConnectionPolicy = EBusConnectionPolicy<Bus>;

        /**
        * Determines whether the bus will lock during dispatch
        * On buses where handlers are attached at startup and removed at shutdown,
        * or where connect/disconnect are not done from within handlers, this is safe
        * to do.
        * By default, the standard policy is used, which locks around all dispatches
        */
        static constexpr bool LocklessDispatch = false;

        /**
         * Specifies where EBus data is stored.
         * This drives how many instances of this EBus exist at runtime.
         * Available storage policies include the following:
         * - (Default) EBusEnvironmentStoragePolicy - %EBus data is stored
         * in the AZ::Environment. With this policy, a single %EBus instance
         * is shared across all modules (DLLs) that attach to the AZ::Environment. It also
         * supports multiple EBus environments.
         * - EBusGlobalStoragePolicy - %EBus data is stored in a global static variable.
         * With this policy, each module (DLL) has its own instance of the %EBus.
         * - EBusThreadLocalStoragePolicy - %EBus data is stored in a thread_local static
         * variable. With this policy, each thread has its own instance of the %EBus.
         *
         * \note Make sure you carefully consider the implication of switching this policy. If your code use EBusEnvironments and your storage policy is not
         * complaint in the best case you will cause contention and unintended communication across environments, separation is a goal of environments. In the worst
         * case when you have listeners, you can receive messages when you environment is NOT active, potentially causing all kinds of havoc especially if you execute
         * environments in parallel.
         */
        template <class Context>
        using StoragePolicy = EBusEnvironmentStoragePolicy<Context>;

        /**
         * Controls the flow of EBus events.
         * Enables an event to be forwarded, and possibly stopped, before reaching
         * the normal event handlers.
         * Use cases for routing include tracing, debugging, and versioning an %EBus.
         * The default `EBusRouterPolicy` forwards the event to each connected
         * `EBusRouterNode` before sending the event to the normal handlers. Each
         * node can stop the event or let it continue.
         */
        template <class Bus>
        using RouterPolicy = EBusRouterPolicy<Bus>;

        /*
        * Performs the actual call on an Ebus handler.
        * Enables custom code to be run on a per-callee basis.
        * Use cases include debugging systems and profiling that need to run custom
        * code before or after an event.
        */
        using EventProcessingPolicy = EBusEventProcessingPolicy;

        /**
        * Template Lock Guard class that wraps around the Mutex
        * The EBus Context uses the LockGuard when dispatching
        * (either AZStd::scoped_lock<MutexType> or NullLockGuard<MutexType>)
        * The IsLocklessDispatch bool is there to defer evaluation of the LocklessDispatch constant
        * Otherwise the value above in EBusTraits.h is always used and not the value
        * that the derived trait class sets.
        */
        template <typename DispatchMutex, bool IsLocklessDispatch>
        using DispatchLockGuard = AZStd::conditional_t<IsLocklessDispatch, AZ::Internal::NullLockGuard<DispatchMutex>, AZStd::scoped_lock<DispatchMutex>>;
    };

    namespace Internal
    {
        template<class EBus>
        class EBusRouter;

        template<class EBus>
        class EBusNestedVersionRouter;
    }

    /**
     * Event buses (EBuses) are a general-purpose communication system
     * that Open 3D Engine uses to dispatch notifications and receive requests.
     *
     * @tparam Interface A class whose virtual functions define the events
     *                   dispatched or received by the %EBus.
     * @tparam Traits    A class that inherits from EBusTraits and configures the %EBus.
     *                   This parameter may be left unspecified if the `Interface` class
     *                   inherits from EBusTraits.
     *
     * EBuses are configurable and support many different use cases.
     * For more information about EBuses, see
     * [Event Bus](https://o3de.org/docs/user-guide/engine/ebus/)
     * and [Components and EBuses: Best Practices ](https://o3de.org/docs/user-guide/components/development/entity-system-pg-components-ebuses-best-practices/)
     * in the *Open 3D Engine Developer Guide*.
     *
     * ## How Components Use EBuses
     * Components commonly use EBuses in two ways: to dispatch events or to handle requests.
     * A bus that dispatches events is a *notification bus*. A bus that receives requests
     * is a *request bus*. Some components provide one type of bus, and some components
     * provide both types. Some components do not provide an %EBus at all. You use the %EBus
     * class for both %EBus types, but you configure the %EBuses differently. The following
     * sections show how to set up and configure notification buses, event handlers, and request
     * buses.
     *
     * ## Notification Buses
     * Notification buses dispatch events. The events are received by *handlers*, which
     * implement a function to handle the event. Handlers first connect to the bus. When the
     * bus dispatches an event, the handler's function executes. This section shows how
     * to set up a notification bus to dispatch an event and a handler to receive the event.
     *
     * ### Setting up a Notification Bus
     * To set up a bus to dispatch events, do the following:
     * 1. Define a class that inherits from EBusTraits. This class will be the interface
     *    for the %EBus.
     * 2. Override individual EBusTraits properties to define the behavior of your bus.
     *    Three EBusTraits that notification buses commonly override are `AddressPolicy`,
     *    which defines how many addresses the %EBus contains, `HandlerPolicy`, which
     *    describes how many handlers can connect to each address, and `BusIdType`,
     *    which is the type of ID that is used to address the EBus if addresses are used.
     *    For example, notification buses often need to have multiple addresses, with
     *    the addresses identified by entity ID. To do so, they override the default
     *    `AddressPolicy` with EBusAddressPolicy::ById and set the `BusIdType` to
     *    EntityId.
     * 3. Declare a function for each event that the %EBus will dispatch.
     *    Handler classes will implement these functions to handle the events.
     * 4. Declare an %EBus that takes your class as a template parameter.
     * 5. Send events. The function that you use to send the event depends on which
     *    addresses you want to send the event to, whether to return a value, the order
     *    in which to call the handlers, and whether to queue the event.
     *  - To send an event to all handlers connected to the %EBus, use Broadcast().
     *    If an %EBus has multiple addresses, you can use Event() to send the event
     *    only to handlers connected at the specified ID. For performance-critical
     *    code, you can avoid an address lookup by using Event() variants that
     *    take a pointer instead of an ID.
     *  - If an event returns a value, use BroadcastResult() or EventResult() to get the result.
     *  - If you want handlers to receive the events in reverse order, use
     *    BroadcastReverse() or EventReverse().
     *  - To send events asynchronously, queue the event. Queued events
     *    are not executed until the queue is flushed. To support queuing, set the
     *    #EnableEventQueue trait. To queue events, use `QueueBroadcast()` or `QueueEvent()`.
     *    To flush the event queue, use `ExecuteQueuedEvents()`.
     *
     * ### Setting up a Handler
     * To enable a handler class to handle the events dispatched by a notification bus, do the following:
     * 1. Derive your handler class from `<BusName>::Handler`.
          For example, a class that needs to handle tick requests should derive from TickRequestBus::Handler.
     * 2. Implement the %EBus interface to define how the handler class should handle the events.
     *    In the tick bus example, a handler class would implement `OnTick()`.
     * 3. Connect and disconnect from the bus at the appropriate places within your handler class's code.
     *    Use `<BusName>:Handler::BusConnect()` to connect to the bus and `<BusName>:Handler::BusDisconnect()`
     *    to disconnect from the bus. If the handler class is a component, connect to the bus in `Activate()`
     *    and disconnect from the bus in `Deactivate()`. Non-components typically connect in the constructor
     *    and disconnect in the destructor.
     *
     * ## Request Buses
     * A request bus receives and handles requests. Typically, only one class handles requests for a request bus.
     * ### Setting up a Request Bus
     * The first several steps for setting up a request bus are similar to setting up a notification bus.
     * After that you also need to implement the handlers for handling the requests. To set up a request bus,
     * do the following:
     * 1. Define a class that inherits from EBusTraits. This class will be the interface for requests made
     *    to the %EBus.
     * 2. Override individual EBusTraits properties to define the behavior of your bus. Two EBusTraits that
     *    request buses commonly override are `AddressPolicy`, which defines how many addresses the %EBus
     *    contains, and `HandlerPolicy`, which describes how many handlers can connect to each address. For
     *    example, because there is typically only one handler class for each request bus, request buses
     *    typically override the default handler policy with EBusHandlerPolicy::Single.
     * 3. Declare a function for each event that the handler class will receive requests about.
     *    These are the functions that other classes will use to make requests of the handler class.
     * 4. Declare an %EBus that takes your class as a template parameter.
     * 5. Implement a handler for the events as described in the previous section ("Setting up a Handler").
     */
    template<class Interface, class BusTraits = Interface>
    class EBus
        : public BusInternal::EBusImpl<AZ::EBus<Interface, BusTraits>, BusInternal::EBusImplTraits<Interface, BusTraits>, typename BusTraits::BusIdType>
    {
    public:
        class Context;

        /**
         * Contains data about EBusTraits.
         */
        using ImplTraits = BusInternal::EBusImplTraits<Interface, BusTraits>;

        /**
         * Represents an %EBus with certain broadcast, event, and routing functionality.
         */
        using BaseImpl = BusInternal::EBusImpl<AZ::EBus<Interface, BusTraits>, BusInternal::EBusImplTraits<Interface, BusTraits>, typename BusTraits::BusIdType>;

        /**
         * Alias for EBusTraits.
         */
        using Traits = typename ImplTraits::Traits;

        /**
         * The type of %EBus, which is defined by the interface and the EBusTraits.
         */
        using ThisType = AZ::EBus<Interface, Traits>;

        /**
         * Allocator used by the %EBus.
         * The default setting is AZStd::allocator, which uses AZ::SystemAllocator.
         */
        using AllocatorType = typename ImplTraits::AllocatorType;

        /**
         * The class that defines the interface of the %EBus.
         */
        using InterfaceType = typename ImplTraits::InterfaceType;

        /**
         * The events that are defined by the %EBus interface.
         */
        using Events = typename ImplTraits::Events;

        /**
         * The type of ID that is used to address the %EBus.
         * Used only when the address policy is AZ::EBusAddressPolicy::ById
         * or AZ::EBusAddressPolicy::ByIdAndOrdered.
         * The type must support `AZStd::hash<ID>` and
         * `bool operator==(const ID&, const ID&)`.
         */
        using BusIdType = typename ImplTraits::BusIdType;

        /**
         * Sorting function for %EBus address IDs.
         * Used only when the address policy is AZ::EBusAddressPolicy::ByIdAndOrdered.
         * If an event is dispatched without an ID, this function determines
         * the order in which each address receives the event.
         *
         * The following example shows a sorting function that meets these requirements.
         * @code{.cpp}
         * using BusIdOrderCompare = AZStd::less<BusIdType>; // Lesser IDs first.
         * @endcode
         */
        using BusIdOrderCompare = typename ImplTraits::BusIdOrderCompare;

        /**
         * Locking primitive that is used when connecting handlers to the EBus or executing events.
         * By default, all access is assumed to be single threaded and no locking occurs.
         * For multithreaded access, specify a mutex of the following type.
         * - For simple multithreaded cases, use AZStd::mutex.
         * - For multithreaded cases where an event handler sends a new event on the same bus
         *   or connects/disconnects while handling an event on the same bus, use AZStd::recursive_mutex.
         */
        using MutexType = typename ImplTraits::MutexType;

        /**
         * Contains all of the addresses on the %EBus.
         */
        using BusesContainer = typename ImplTraits::BusesContainer;

        /**
         * Locking primitive that is used when executing events in the event queue.
         */
        using EventQueueMutexType = typename ImplTraits::EventQueueMutexType;

        /**
         * Pointer to an address on the bus.
         */
        using BusPtr = typename ImplTraits::BusPtr;

        /**
         * Pointer to a handler node.
         */
        using HandlerNode = typename ImplTraits::HandlerNode;

        /**
         * Policy for the function queue.
         */
        using QueuePolicy = EBusQueuePolicy<Traits::EnableEventQueue, ThisType, EventQueueMutexType>;

        /**
         * Enables custom logic to run when a handler connects to
         * or disconnects from the %EBus.
         * For example, you can make a handler execute an event
         * immediately upon connecting to the %EBus.
         * For available settings, see AZ::EBusConnectionPolicy.
         * By default, no extra logic is run.
         */
        using ConnectionPolicy = typename Traits::template ConnectionPolicy<ThisType>;

        /**
         * Used to manually create a callstack entry for a call (often used by ConnectionPolicy)
         */
        using CallstackEntry = AZ::Internal::CallstackEntry<Interface, Traits>;

        /**
         * Specifies whether the %EBus supports an event queue.
         * You can use the event queue to execute events at a later time.
         * To execute the queued events, you must call
         * `<BusName>::ExecuteQueuedEvents()`.
         * By default, the event queue is disabled.
         */
        static const bool EnableEventQueue = ImplTraits::EnableEventQueue;

        /**
         * Class that implements %EBus routing functionality.
         */
        using Router = AZ::Internal::EBusRouter<ThisType>;

        /**
         * Class that implements an %EBus version router.
         */
        using NestedVersionRouter = AZ::Internal::EBusNestedVersionRouter<ThisType>;

        /**
         * Controls the flow of %EBus events.
         * Enables an event to be forwarded, and possibly stopped, before reaching
         * the normal event handlers.
         * Use cases for routing include tracing, debugging, and versioning an %EBus.
         * The default `EBusRouterPolicy` forwards the event to each connected
         * `EBusRouterNode` before sending the event to the normal handlers. Each
         * node can stop the event or let it continue.
         */
        using RouterPolicy = typename Traits::template RouterPolicy<ThisType>;

        /**
         * State that indicates whether to continue routing the event, skip all
         * handlers but notify other routers, or stop processing the event.
         */
        using RouterProcessingState = typename RouterPolicy::EventProcessingState;

        /**
         * True if the %EBus supports more than one address. Otherwise, false.
         */
        static const bool HasId = Traits::AddressPolicy != EBusAddressPolicy::Single;

        /**
        * Template Lock Guard class that wraps around the Mutex
        * The EBus uses for Dispatching Events.
        * This is not EBus Context Mutex when LocklessDispatch is set
        */
        template <typename DispatchMutex>
        using DispatchLockGuard = typename ImplTraits::template DispatchLockGuard<DispatchMutex>;

        //////////////////////////////////////////////////////////////////////////
        // Check to help identify common mistakes
        /// @cond EXCLUDE_DOCS
        static_assert((HasId || AZStd::is_same<BusIdType, NullBusId>::value),
            "When you use EBusAddressPolicy::Single there is no need to define BusIdType!");
        static_assert((!HasId || !AZStd::is_same<BusIdType, NullBusId>::value),
            "You must provide a valid BusIdType when using EBusAddressPolicy::ById or EBusAddressPolicy::ByIdAndOrdered! (ex. using BusIdType = int;");
        static_assert((BusTraits::AddressPolicy == EBusAddressPolicy::ByIdAndOrdered || AZStd::is_same<BusIdOrderCompare, NullBusIdCompare>::value),
            "When you use EBusAddressPolicy::Single or EBusAddressPolicy::ById there is no need to define BusIdOrderCompare!");
        static_assert((BusTraits::AddressPolicy != EBusAddressPolicy::ByIdAndOrdered || !AZStd::is_same<BusIdOrderCompare, NullBusIdCompare>::value),
            "When you use EBusAddressPolicy::ByIdAndOrdered you must define BusIdOrderCompare (ex. using BusIdOrderCompare = AZStd::less<BusIdType>)");
        /// @endcond
        /// //////////////////////////////////////////////////////////////////////////

        /// @cond EXCLUDE_DOCS

        /**
         * Connects a handler to an EBus address.
         * A handler will not receive EBus events until it is connected to the bus.
         * @param[out] ptr A pointer that will be bound to the EBus address that
         * the handler will be connected to.
         * @param handler The handler to connect to the EBus address.
         * @param id The ID of the EBus address that the handler will be connected to.
        */
        static void Connect(HandlerNode& handler, const BusIdType& id = 0);

         /**
         * Disconnects a handler from an EBus address.
         * @param handler The handler to disconnect from the EBus address.
         */
        static void Disconnect(HandlerNode& handler);

        /**
         * Disconnects a handler from an EBus address without locking the mutex
         * Only call this if the context mutex is held already
         * @param handler The handler to disconnect from the EBus address.
         */
        static void DisconnectInternal(Context& context, HandlerNode& handler);
        /// @endcond

        /**
         * Returns the total number of handlers that are connected to the EBus.
         * @return The total number of handlers that are connected to the EBus.
         */
        static size_t GetTotalNumOfEventHandlers();

        /**
         * Returns whether any handlers are connected to the EBus.
         * @return True if there are any handlers connected to the
         * EBus. Otherwise, false.
         */
        static inline bool HasHandlers();

        /**
         * Returns whether handlers are connected to this specific address.
         * @return True if there are any handlers connected at the address.
         * Otherwise, false.
         */
        static inline bool HasHandlers(const BusIdType& id);

        /**
         * Returns whether handlers are connected to the specific cached address.
         * @return True if there are any handlers connected at the cached address.
         * Otherwise, false.
         */
        static inline bool HasHandlers(const BusPtr& ptr);

        /**
         * Gets the ID of the address that is currently receiving an event.
         * You can use this function while handling an event to determine which ID
         * the event concerns.
         * This is especially useful for handlers that connect to multiple address IDs.
         * @return Pointer to the address ID that is currently receiving an event.
         * Returns a null pointer if the EBus is not currently sending an event
         * or the EBus does not use an AZ::EBusAddressPolicy that has multiple addresses.
         */
        static const BusIdType* GetCurrentBusId();

        /// @cond EXCLUDE_DOCS
        /**
         * Sets the current event processing state. This function has an
         * effect only when it is called within a router event.
         * @param state A new processing state.
         */
        static void SetRouterProcessingState(RouterProcessingState state);

        /**
         * Determines whether the current event is being routed as a queued event.
         * This function has an effect only when it is called within a router event.
         * @return True if the current event is being routed as a queued event.
         * Otherwise, false.
         */
        static bool IsRoutingQueuedEvent();

        /**
         * Determines whether the current event is being routed in reverse order.
         * This function has an effect only when it is called within a router event.
         * @return True if the current event is being routed in reverse order.
         * Otherwise, false.
         */
        static bool IsRoutingReverseEvent();
        /// @endcond

        /**
         * Returns a unique signature for the EBus.
         * @return A unique signature for the EBus.
         */
        static const char* GetName();

        /// @cond EXCLUDE_DOCS
        class Context : public AZ::Internal::ContextBase
        {
            friend ThisType;
            friend Router;
        public:
            /**
             * The mutex type to use during broadcast/event dispatch.
             * When LocklessDispatch is set on the EBus and a NullMutex is supplied a shared_mutex is used to protect the context otherwise the supplied MutexType is used
             * The reason why a recursive_mutex is used in this situation, is that specifying LocklessDispatch is implies that the EBus will be used across multiple threads
             * @see EBusTraits::LocklessDispatch
             */
            using ContextMutexType = AZStd::conditional_t<BusTraits::LocklessDispatch && AZStd::is_same_v<MutexType, AZ::NullMutex>, AZStd::shared_mutex, MutexType>;

            /**
             * The scoped lock guard to use
             * during broadcast/event dispatch.
             * @see EBusTraits::LocklessDispatch
             */
            using DispatchLockGuard = DispatchLockGuard<ContextMutexType>;

            /**
            * The scoped lock guard to use during connection.  Some specialized policies execute handler methods which
            * can cause unnecessary delays holding the context mutex or in some cases perform blocking waits and
            * must unlock the context mutex before doing so to prevent deadlock when the wait is for
            * an event in another thread which is trying to connect to the same bus before it can complete
            */
            using ConnectLockGuard = AZStd::conditional_t<AZStd::is_same_v<ContextMutexType, AZ::NullMutex>, AZ::Internal::NullLockGuard<ContextMutexType>, AZStd::unique_lock<ContextMutexType>>;

            BusesContainer          m_buses;         ///< The actual bus container, which is a static map for each bus type.
            ContextMutexType        m_contextMutex;  ///< Mutex to control access when modifying the context
            QueuePolicy             m_queue;
            RouterPolicy            m_routing;

            Context();
            Context(EBusEnvironment* environment);
            ~Context() override;

            // Disallow all copying/moving
            Context(const Context&) = delete;
            Context(Context&&) = delete;
            Context& operator=(const Context&) = delete;
            Context& operator=(Context&&) = delete;

        private:
            using CallstackEntryBase = AZ::Internal::CallstackEntryBase<Interface, Traits>;
            using CallstackEntryRoot = AZ::Internal::CallstackEntryRoot<Interface, Traits>;
            using CallstackEntryStorageType = AZ::Internal::EBusCallstackStorage<CallstackEntryBase, !AZStd::is_same_v<ContextMutexType, AZ::NullMutex>>;

            mutable AZStd::unordered_map<AZStd::native_thread_id_type, CallstackEntryRoot, AZStd::hash<AZStd::native_thread_id_type>, AZStd::equal_to<AZStd::native_thread_id_type>, AZ::Internal::EBusEnvironmentAllocator> m_callstackRoots;
            CallstackEntryStorageType s_callstack;    ///< Linked list of other bus calls to this bus on the stack, per thread if MutexType is defined
            AZStd::atomic_uint m_dispatches;   ///< Number of active dispatches in progress

            friend CallstackEntry;
        };
        /// @endcond

        /**
         * Specifies where %EBus data is stored.
         * This drives how many instances of this %EBus exist at runtime.
         * Available storage policies include the following:
         * - (Default) EBusEnvironmentStoragePolicy - %EBus data is stored
         * in the AZ::Environment. With this policy, a single %EBus instance
         * is shared across all modules (DLLs) that attach to the AZ::Environment.
         * - EBusGlobalStoragePolicy - %EBus data is stored in a global static variable.
         * With this policy, each module (DLL) has its own instance of the %EBus.
         * - EBusThreadLocalStoragePolicy - %EBus data is stored in a thread_local static
         * variable. With this policy, each thread has its own instance of the %EBus.
         */
        using StoragePolicy = typename Traits::template StoragePolicy<Context>;

        /**
         * Returns the global bus data (if it was created).
         * Depending on the storage policy, there might be one or multiple instances
         * of the bus data.
         * @return A reference to the bus context.
         */
        static Context* GetContext(bool trackCallstack=true);

        using ConnectLockGuard = typename Context::ConnectLockGuard;
        /**
         * Connects a handler to an EBus address without locking the mutex
         * Only call this if the context mutex is held already
         * A handler will not receive EBus events until it is connected to the bus.
         * the handler will be connected to.
         * @param handler The handler to connect to the EBus address.
         * @param id The ID of the EBus address that the handler will be connected to.
        */
        static void ConnectInternal(Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id = 0);

        /**
         * Returns the global bus data. Creates it if it wasn't already created.
         * Depending on the storage policy, there might be one or multiple instances
         * of the bus data.
         * @return A reference to the bus context.
         */
        static Context& GetOrCreateContext(bool trackCallstack=true);

        static bool IsInDispatch(Context* context = GetContext(false));

        /**
         * Returns whether the EBus context is in the middle of a dispatch on the current thread
        */
        static bool IsInDispatchThisThread(Context* context = GetContext(false));
        /// @cond EXCLUDE_DOCS
        struct RouterCallstackEntry
            : public CallstackEntry
        {
            typedef typename RouterPolicy::Container::iterator Iterator;

            RouterCallstackEntry(Iterator it, const BusIdType* busId, bool isQueued, bool isReverse);

            ~RouterCallstackEntry() override = default;

            void SetRouterProcessingState(RouterProcessingState state) override;

            bool IsRoutingQueuedEvent() const override;

            bool IsRoutingReverseEvent() const override;

            Iterator m_iterator;
            RouterProcessingState m_processingState;
            bool m_isQueued;
            bool m_isReverse;
        };
        /// @endcond
    };

    /// Helper macro to deprecate the helper typedef EBus<_Interface> _BusName
    /// Where _Interface is a deprecated  EBus API class
#   define DEPRECATE_EBUS(_Interface, _BusName, _message) DEPRECATE_EBUS_WITH_TRAITS(_Interface, _Interface, _BusName, _message)
    /// Helper macro to deprecate the helper typedef EBus<_Interface, _BusTraits> _BusName
    /// Where _Interface is a deprecated EBus API class and/or _BusTraits is a deprecated EBusTraits class
#   define DEPRECATE_EBUS_WITH_TRAITS(_Interface, _BusTraits, _BusName, _message)       \
    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")                          \
    typedef AZ::EBus<_Interface, _BusTraits> DeprecatedBus_##_Interface##_BusTraits;    \
    AZ_POP_DISABLE_WARNING                                                              \
    AZ_DEPRECATED(typedef DeprecatedBus_##_Interface##_BusTraits _BusName, _message);

    // The macros below correspond to functions in BusImpl.h.
    // The macros enable you to write shorter code, but don't work as well for code completion.

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)  _EBUS::Event(_BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_EVENT_PTR_RESULT(_Result, _BusPtr, _EBUS, /*EventName,*/ ...) _EBUS::EventResult(_Result, _BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...)    _EBUS::Event(_BusId, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_EVENT_ID_RESULT(_Result, _BusId, _EBUS, /*EventName,*/ ...) _EBUS::EventResult(_Result, _BusId, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers.
#   define EBUS_EVENT(_EBUS, /*EventName,*/ ...) _EBUS::Broadcast(&_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_EVENT_RESULT(_Result, _EBUS, /*EventName,*/ ...) _EBUS::BroadcastResult(_Result, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...)  _EBUS::EventReverse(_BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EBUS, /*EventName,*/ ...) _EBUS::EventResultReverse(_Result, _BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...) _EBUS::EventReverse(_BusId, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EBUS, /*EventName,*/ ...) _EBUS::EventReverse(_Result, _BusId, &_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_EVENT_REVERSE(_EBUS, /*EventName,*/ ...) _EBUS::BroadcastReverse(&_EBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_EVENT_RESULT_REVERSE(_Result, _EBUS, /*EventName,*/ ...) _EBUS::BroadcastResultReverse(_Result, &_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_QUEUE_EVENT(_EBUS, /*EventName,*/ ...)                _EBUS::QueueBroadcast(&_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_QUEUE_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)    _EBUS::QueueEvent(_BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_QUEUE_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...)      _EBUS::QueueEvent(_BusId, &_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_QUEUE_EVENT_REVERSE(_EBUS, /*EventName,*/ ...)                _EBUS::QueueBroadcastReverse(&_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...)    _EBUS::QueueEventReverse(_BusPtr, &_EBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_QUEUE_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...)      _EBUS::QueueEventReverse(_BusId, &_EBUS::Events::__VA_ARGS__)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_QUEUE_FUNCTION(_EBUS, /*Function pointer, params*/ ...)       _EBUS::QueueFunction(__VA_ARGS__)

    //////////////////////////////////////////////////////////////////////////
    // Debug events active only when AZ_DEBUG_BUILD is defined
#if defined(AZ_DEBUG_BUILD)

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_DBG_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)   EBUS_EVENT_PTR(_BusPtr, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT(_Result, _BusPtr, _EBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_RESULT(_Result, _BusPtr, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_DBG_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...) EBUS_EVENT_ID(_BusId, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT(_Result, _BusId, _EBUS, /*EventName,*/ ...) EBUS_EVENT_ID_RESULT(_Result, _BusId, _EBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers.
#   define EBUS_DBG_EVENT(_EBUS, /*EventName,*/ ...) EBUS_EVENT(_EBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_DBG_EVENT_RESULT(_Result, _EBUS, /*EventName,*/ ...)  EBUS_EVENT_RESULT(_Result, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_DBG_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_REVERSE(_BusPtr, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_DBG_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...) EBUS_EVENT_ID_REVERSE(_BusId, _EBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EBUS, /*EventName,*/ ...) EBUS_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_DBG_EVENT_REVERSE(_EBUS, /*EventName,*/ ...) EBUS_EVENT_REVERSE(_EBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_DBG_EVENT_RESULT_REVERSE(_Result, _EBUS, /*EventName,*/ ...) EBUS_EVENT_RESULT_REVERSE(_Result, _EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_DBG_QUEUE_EVENT(_EBUS, /*EventName,*/ ...)                EBUS_QUEUE_EVENT(_EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_DBG_QUEUE_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)    EBUS_QUEUE_EVENT_PTR(BusPtr, _EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_DBG_QUEUE_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...)      EBUS_QUEUE_EVENT_ID(_BusId, _EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_REVERSE(_EBUS, /*EventName,*/ ...)                EBUS_QUEUE_EVENT_REVERSE(_EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...)    EBUS_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...)      EBUS_QUEUE_EVENT_ID_REVERSE(_BusId, _EBUS, __VA_ARGS__)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_DBG_QUEUE_FUNCTION(_EBUS, /*Function pointer, params*/ ...)   EBUS_QUEUE_FUNCTION(_EBUS, __VA_ARGS__)

#else // !AZ_DEBUG_BUILD

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_DBG_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT(_Result, _BusPtr, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_DBG_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT(_Result, _BusId, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers.
#   define EBUS_DBG_EVENT(_EBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_DBG_EVENT_RESULT(_Result, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_DBG_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_DBG_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_DBG_EVENT_REVERSE(_EBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_DBG_EVENT_RESULT_REVERSE(_Result, _EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_DBG_QUEUE_EVENT(_EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_DBG_QUEUE_EVENT_PTR(_BusPtr, _EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_DBG_QUEUE_EVENT_ID(_BusId, _EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_REVERSE(_EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_ID_REVERSE(_BusId, _EBUS, /*EventName,*/ ...)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_DBG_QUEUE_FUNCTION(_EBUS, /*Function name*/ ...)

#endif // !AZ_DEBUG BUILD

    // Debug events
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // EBus implementations

    namespace Internal
    {
        template <class C>
        AZ_THREAD_LOCAL C* EBusCallstackStorage<C, true>::s_entry = nullptr;
    }

    //=========================================================================
    // Context::Context
    //=========================================================================
    template<class Interface, class Traits>
    EBus<Interface, Traits>::Context::Context()
        : m_dispatches(0)
    {
        s_callstack = nullptr;
    }

    //=========================================================================
    // Context::Context
    //=========================================================================
    template<class Interface, class Traits>
    EBus<Interface, Traits>::Context::Context(EBusEnvironment* environment)
        : AZ::Internal::ContextBase(environment)
        , m_dispatches(0)
    {
        s_callstack = nullptr;
    }

    template <class Interface, class Traits>
    EBus<Interface, Traits>::Context::~Context()
    {
        // Clear the callstack in this thread. It is expected that most buses will be lifetime managed
        // by the thread that creates them (almost certainly the main thread). This allows a bus
        // to be re-entrant within the same main thread (useful for unit tests and code reloading).
        s_callstack = nullptr;
    }

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")

    //=========================================================================
    // Connect
    //=========================================================================
    template<class Interface, class Traits>
    inline void EBus<Interface, Traits>::Connect(HandlerNode& handler, const BusIdType& id)
    {
        Context& context = GetOrCreateContext();
        // scoped lock guard in case of exception / other odd situation
        // Context mutex is separate from the Dispatch lock guard and therefore this is safe to lock this mutex while in the middle of a dispatch
        ConnectLockGuard lock(context.m_contextMutex);
        ConnectInternal(context, handler, lock, id);
    }

    //=========================================================================
    // ConnectInternal
    //=========================================================================
    template<class Interface, class Traits>
    inline void EBus<Interface, Traits>::ConnectInternal(Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id)
    {
        // To call this while executing a message, you need to make sure this mutex is AZStd::recursive_mutex. Otherwise, a deadlock will occur.
        AZ_Assert(!Traits::LocklessDispatch || !IsInDispatch(&context), "It is not safe to connect during dispatch on a lockless dispatch EBus");

        // Do the actual connection
        context.m_buses.Connect(handler, id);

        BusPtr ptr;
        if constexpr (EBus::HasId)
        {
            ptr = handler.m_holder;
        }
        CallstackEntry entry(&context, &id);
        ConnectionPolicy::Connect(ptr, context, handler, contextLock, id);
    }

    //=========================================================================
    // Disconnect
    //=========================================================================
    template<class Interface, class Traits>
    inline void EBus<Interface, Traits>::Disconnect(HandlerNode& handler)
    {
        // To call Disconnect() from a message while being thread safe, you need to make sure the context.m_contextMutex is AZStd::recursive_mutex. Otherwise, a deadlock will occur.
        if (Context* context = GetContext())
        {
            // scoped lock guard in case of exception / other odd situation
            AZStd::scoped_lock<decltype(context->m_contextMutex)> lock(context->m_contextMutex);
            DisconnectInternal(*context, handler);
        }
    }

    //=========================================================================
    // DisconnectInternal
    //=========================================================================
    template<class Interface, class Traits>
    inline void EBus<Interface, Traits>::DisconnectInternal(Context& context, HandlerNode& handler)
    {
        // To call this while executing a message, you need to make sure this mutex is AZStd::recursive_mutex. Otherwise, a deadlock will occur.
        AZ_Assert(!Traits::LocklessDispatch || !IsInDispatch(&context), "It is not safe to disconnect during dispatch on a lockless dispatch EBus");

        auto callstack = context.s_callstack->m_prev;
        if (callstack)
        {
            callstack->OnRemoveHandler(handler);
        }

        BusPtr ptr;
        if constexpr (EBus::HasId)
        {
            ptr = handler.m_holder;
        }
        ConnectionPolicy::Disconnect(context, handler, ptr);

        CallstackEntry entry(&context, nullptr);

        // Do the actual disconnection
        context.m_buses.Disconnect(handler);

        if (callstack)
        {
            callstack->OnPostRemoveHandler();
        }

        handler = nullptr;
    }

AZ_POP_DISABLE_WARNING

    //=========================================================================
    // GetTotalNumOfEventHandlers
    //=========================================================================
    template<class Interface, class Traits>
    size_t  EBus<Interface, Traits>::GetTotalNumOfEventHandlers()
    {
        size_t size = 0;
        BaseImpl::EnumerateHandlers([&size](Interface*)
        {
            ++size;
            return true;
        });
        return size;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EBus<Interface, Traits>::HasHandlers()
    {
        bool hasHandlers = false;
        auto findFirstHandler = [&hasHandlers](InterfaceType*)
        {
            hasHandlers = true;
            return false;
        };
        BaseImpl::EnumerateHandlers(findFirstHandler);
        return hasHandlers;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EBus<Interface, Traits>::HasHandlers(const BusIdType& id)
    {
        return BaseImpl::FindFirstHandler(id) != nullptr;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EBus<Interface, Traits>::HasHandlers(const BusPtr& ptr)
    {
        return BaseImpl::FindFirstHandler(ptr) != nullptr;
    }

    //=========================================================================
    // GetCurrentBusId
    //=========================================================================
    template<class Interface, class Traits>
    const typename EBus<Interface, Traits>::BusIdType * EBus<Interface, Traits>::GetCurrentBusId()
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            return context->s_callstack->m_prev->m_busId;
        }
        return nullptr;
    }

    //=========================================================================
    // SetRouterProcessingState
    //=========================================================================
    template<class Interface, class Traits>
    void EBus<Interface, Traits>::SetRouterProcessingState(RouterProcessingState state)
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            context->s_callstack->m_prev->SetRouterProcessingState(state);
        }
    }

    //=========================================================================
    // IsRoutingQueuedEvent
    //=========================================================================
    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::IsRoutingQueuedEvent()
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            return context->s_callstack->m_prev->IsRoutingQueuedEvent();
        }

        return false;
    }

    //=========================================================================
    // IsRoutingReverseEvent
    //=========================================================================
    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::IsRoutingReverseEvent()
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            return context->s_callstack->m_prev->IsRoutingReverseEvent();
        }

        return false;
    }

    //=========================================================================
    // GetName
    //=========================================================================
    template<class Interface, class Traits>
    const char* EBus<Interface, Traits>::GetName()
    {
        return AZ_FUNCTION_SIGNATURE;
    }

    //=========================================================================
    // GetContext
    //=========================================================================
    template<class Interface, class Traits>
    typename EBus<Interface, Traits>::Context* EBus<Interface, Traits>::GetContext(bool trackCallstack)
    {
        Context* context = StoragePolicy::Get();
        if (trackCallstack && context && !context->s_callstack)
        {
            // cache the callstack into this thread/dll
            AZStd::scoped_lock<decltype(context->m_contextMutex)> lock(context->m_contextMutex);
            context->s_callstack = &context->m_callstackRoots[AZStd::this_thread::get_id().m_id];
        }
        return context;
    }

    //=========================================================================
    // GetContext
    //=========================================================================
    template<class Interface, class Traits>
    typename EBus<Interface, Traits>::Context& EBus<Interface, Traits>::GetOrCreateContext(bool trackCallstack)
    {
        Context& context = StoragePolicy::GetOrCreate();
        if (trackCallstack && !context.s_callstack)
        {
            // cache the callstack into this thread/dll
            AZStd::scoped_lock<decltype(context.m_contextMutex)> lock(context.m_contextMutex);
            context.s_callstack = &context.m_callstackRoots[AZStd::this_thread::get_id().m_id];
        }
        return context;
    }

    //=========================================================================
    // IsInDispatch
    //=========================================================================
    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::IsInDispatch(Context* context)
    {
        return context != nullptr && context->m_dispatches > 0;
    }

    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::IsInDispatchThisThread(Context* context)
    {
        return context != nullptr && context->s_callstack != nullptr
            && context->s_callstack->m_prev != nullptr;
    }

    //=========================================================================
    template<class Interface, class Traits>
    EBus<Interface, Traits>::RouterCallstackEntry::RouterCallstackEntry(Iterator it, const BusIdType* busId, bool isQueued, bool isReverse)
        : CallstackEntry(EBus::GetContext(), busId)
        , m_iterator(it)
        , m_processingState(RouterPolicy::EventProcessingState::ContinueProcess)
        , m_isQueued(isQueued)
        , m_isReverse(isReverse)
    {
    }

    //=========================================================================
    template<class Interface, class Traits>
    void EBus<Interface, Traits>::RouterCallstackEntry::SetRouterProcessingState(RouterProcessingState state)
    {
        m_processingState = state;
    }

    //=========================================================================
    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::RouterCallstackEntry::IsRoutingQueuedEvent() const
    {
        return m_isQueued;
    }

    //=========================================================================
    template<class Interface, class Traits>
    bool EBus<Interface, Traits>::RouterCallstackEntry::IsRoutingReverseEvent() const
    {
        return m_isReverse;
    }

    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        // NonIdHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void NonIdHandler<Interface, Traits, ContainerType>::BusConnect()
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (!BusIsConnected())
            {
                typename Traits::BusIdType id;
                m_node = this;
                BusType::ConnectInternal(context, m_node, contextLock, id);
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void NonIdHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                if (BusIsConnected())
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // IdHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusConnect(const IdType& id)
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (BusIsConnected())
            {
                // Connecting on the BusId that is already connected is a no-op
                if (m_node.GetBusId() == id)
                {
                    return;
                }
                AZ_Assert(false, "Connecting to a different id on this bus without disconnecting first! Please ensure you call BusDisconnect before calling BusConnect again, or if multiple connections are desired you must use a MultiHandler instead.");
                BusType::DisconnectInternal(context, m_node);
            }

            m_node = this;
            BusType::ConnectInternal(context, m_node, contextLock, id);
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusDisconnect(const IdType& id)
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                if (BusIsConnectedId(id))
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                if (BusIsConnected())
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // MultiHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusConnect(const IdType& id)
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (m_handlerNodes.find(id) == m_handlerNodes.end())
            {
                void* handlerNodeAddr = m_handlerNodes.get_allocator().allocate(sizeof(HandlerNode), AZStd::alignment_of<HandlerNode>::value);
                auto handlerNode = new(handlerNodeAddr) HandlerNode(this);
                m_handlerNodes.emplace(id, AZStd::move(handlerNode));
                BusType::ConnectInternal(context, *handlerNode, contextLock, id);
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusDisconnect(const IdType& id)
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                auto nodeIt = m_handlerNodes.find(id);
                if (nodeIt != m_handlerNodes.end())
                {
                    HandlerNode* handlerNode = nodeIt->second;
                    BusType::DisconnectInternal(*context, *handlerNode);
                    m_handlerNodes.erase(nodeIt);
                    handlerNode->~HandlerNode();
                    m_handlerNodes.get_allocator().deallocate(handlerNode, sizeof(HandlerNode), alignof(HandlerNode));
                }
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            decltype(m_handlerNodes) handlerNodesToDisconnect;
            if (typename BusType::Context* context = BusType::GetContext())
            {
                AZStd::scoped_lock<decltype(context->m_contextMutex)> contextLock(context->m_contextMutex);
                handlerNodesToDisconnect = AZStd::move(m_handlerNodes);

                for (const auto& nodePair : handlerNodesToDisconnect)
                {
                    BusType::DisconnectInternal(*context, *nodePair.second);

                    nodePair.second->~HandlerNode();
                    handlerNodesToDisconnect.get_allocator().deallocate(nodePair.second, sizeof(HandlerNode), AZStd::alignment_of<HandlerNode>::value);
                }
            }
        }

        template <class EBus, class TargetEBus, class BusIdType>
        struct EBusRouterQueueEventForwarder
        {
            static_assert((AZStd::is_same<BusIdType, typename EBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");
            static_assert((AZStd::is_same<BusIdType, typename TargetEBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");

            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template <class Event, class... Args>
            static void ForwardEventResult(Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus, class BusIdType>
        template<class Event, class... Args>
        void EBusRouterQueueEventForwarder<EBus, TargetEBus, BusIdType>::ForwardEvent(Event event, Args&&... args)
        {
            const BusIdType* busId = EBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EBus::IsRoutingQueuedEvent())
                {
                    // Queue broadcast
                    if (EBus::IsRoutingReverseEvent())
                    {
                        // Queue broadcast reverse
                        TargetEBus::QueueBroadcastReverse(event, args...);
                    }
                    else
                    {
                        // Queue broadcast forward
                        TargetEBus::QueueBroadcast(event, args...);
                    }
                }
                else
                {
                    // In place broadcast
                    if (EBus::IsRoutingReverseEvent())
                    {
                        // In place broadcast reverse
                        TargetEBus::BroadcastReverse(event, args...);
                    }
                    else
                    {
                        // In place broadcast forward
                        TargetEBus::Broadcast(event, args...);
                    }
                }
            }
            else
            {
                // Event with an ID
                if (EBus::IsRoutingQueuedEvent())
                {
                    // Queue event
                    if (EBus::IsRoutingReverseEvent())
                    {
                        // Queue event reverse
                        TargetEBus::QueueEventReverse(*busId, event, args...);
                    }
                    else
                    {
                        // Queue event forward
                        TargetEBus::QueueEvent(*busId, event, args...);
                    }
                }
                else
                {
                    // In place event
                    if (EBus::IsRoutingReverseEvent())
                    {
                        // In place event reverse
                        TargetEBus::EventReverse(*busId, event, args...);
                    }
                    else
                    {
                        // In place event forward
                        TargetEBus::Event(*busId, event, args...);
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus, class BusIdType>
        template <class Event, class... Args>
        void EBusRouterQueueEventForwarder<EBus, TargetEBus, BusIdType>::ForwardEventResult(Event event, Args&&... args)
        {

        }

        template <class EBus, class TargetEBus>
        struct EBusRouterQueueEventForwarder<EBus, TargetEBus, NullBusId>
        {
            static_assert((AZStd::is_same<NullBusId, typename EBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");
            static_assert((AZStd::is_same<NullBusId, typename TargetEBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");

            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template <class Event, class... Args>
            static void ForwardEventResult(Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus>
        template<class Event, class... Args>
        void EBusRouterQueueEventForwarder<EBus, TargetEBus, NullBusId>::ForwardEvent(Event event, Args&&... args)
        {
            // Broadcast
            if (EBus::IsRoutingQueuedEvent())
            {
                // Queue broadcast
                if (EBus::IsRoutingReverseEvent())
                {
                    // Queue broadcast reverse
                    TargetEBus::QueueBroadcastReverse(event, args...);
                }
                else
                {
                    // Queue broadcast forward
                    TargetEBus::QueueBroadcast(event, args...);
                }
            }
            else
            {
                // In place broadcast
                if (EBus::IsRoutingReverseEvent())
                {
                    // In place broadcast reverse
                    TargetEBus::BroadcastReverse(event, args...);
                }
                else
                {
                    // In place broadcast forward
                    TargetEBus::Broadcast(event, args...);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus>
        template <class Event, class... Args>
        void EBusRouterQueueEventForwarder<EBus, TargetEBus, NullBusId>::ForwardEventResult(Event event, Args&&... args)
        {
        }

        template <class EBus, class TargetEBus, class BusIdType>
        struct EBusRouterEventForwarder
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus, class BusIdType>
        template<class Event, class... Args>
        void EBusRouterEventForwarder<EBus, TargetEBus, BusIdType>::ForwardEvent(Event event, Args&&... args)
        {
            const BusIdType* busId = EBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EBus::IsRoutingReverseEvent())
                {
                    // Broadcast reverse
                    TargetEBus::BroadcastReverse(event, args...);
                }
                else
                {
                    // Broadcast forward
                    TargetEBus::Broadcast(event, args...);
                }
            }
            else
            {
                // Event.
                if (EBus::IsRoutingReverseEvent())
                {
                    // Event reverse
                    TargetEBus::EventReverse(*busId, event, args...);
                }
                else
                {
                    // Event forward
                    TargetEBus::Event(*busId, event, args...);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus, class BusIdType>
        template<class Result, class Event, class... Args>
        void EBusRouterEventForwarder<EBus, TargetEBus, BusIdType>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            const BusIdType* busId = EBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EBus::IsRoutingReverseEvent())
                {
                    // Broadcast reverse
                    TargetEBus::BroadcastResultReverse(result, event, args...);
                }
                else
                {
                    // Broadcast forward
                    TargetEBus::BroadcastResult(result, event, args...);
                }
            }
            else
            {
                // Event
                if (EBus::IsRoutingReverseEvent())
                {
                    // Event reverse
                    TargetEBus::EventResultReverse(result, *busId, event, args...);
                }
                else
                {
                    // Event forward
                    TargetEBus::EventResult(result, *busId, event, args...);
                }
            }
        }

        template <class EBus, class TargetEBus>
        struct EBusRouterEventForwarder<EBus, TargetEBus, NullBusId>
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus>
        template<class Event, class... Args>
        void EBusRouterEventForwarder<EBus, TargetEBus, NullBusId>::ForwardEvent(Event event, Args&&... args)
        {
            // Broadcast
            if (EBus::IsRoutingReverseEvent())
            {
                // Broadcast reverse
                TargetEBus::BroadcastReverse(event, args...);
            }
            else
            {
                // Broadcast forward
                TargetEBus::Broadcast(event, args...);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBus, class TargetEBus>
        template<class Result, class Event, class... Args>
        void EBusRouterEventForwarder<EBus, TargetEBus, NullBusId>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            // Broadcast
            if (EBus::IsRoutingReverseEvent())
            {
                // Broadcast reverse
                TargetEBus::BroadcastResultReverse(result, event, args...);
            }
            else
            {
                // Broadcast forward
                TargetEBus::BroadcastResult(result, event, args...);
            }
        }

        template<class EBus, class TargetEBus, bool allowQueueing = EBus::EnableEventQueue>
        struct EBusRouterForwarderHelper
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args)
            {
                EBusRouterQueueEventForwarder<EBus, TargetEBus, typename EBus::BusIdType>::ForwardEvent(event, args...);
            }

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result&, Event, Args&&...)
            {

            }
        };

        template<class EBus, class TargetEBus>
        struct EBusRouterForwarderHelper<EBus, TargetEBus, false>
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args)
            {
                EBusRouterEventForwarder<EBus, TargetEBus, typename EBus::BusIdType>::ForwardEvent(event, args...);
            }

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args)
            {
                EBusRouterEventForwarder<EBus, TargetEBus, typename EBus::BusIdType>::ForwardEventResult(result, event, args...);
            }
        };

        /**
        * EBus router helper class. Inherit from this class the same way
        * you do with EBus::Handlers, to implement router functionality.
        *
        */
        template<class EBus>
        class EBusRouter
            : public EBus::InterfaceType
        {
            EBusRouterNode<typename EBus::InterfaceType> m_routerNode;
            bool m_isConnected;
        public:
            EBusRouter();
            virtual ~EBusRouter();

            void BusRouterConnect(int order = 0);

            void BusRouterDisconnect();

            bool BusRouterIsConnected() const;

            template<class TargetEBus, class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class TargetEBus, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        /**
         * Helper class for when we are writing an EBus version router that will be part
         * of an EBusRouter policy (that is, active the entire time the bus is used).
         * It will be created when a bus context is created.
         */
        template<class EBus>
        class EBusNestedVersionRouter
                : public EBus::InterfaceType
        {
            EBusRouterNode<typename EBus::InterfaceType> m_routerNode;
        public:
            virtual ~EBusNestedVersionRouter() = default;
            template<class Container>
            void BusRouterConnect(Container& container, int order = 0);

            template<class Container>
            void BusRouterDisconnect(Container& container);

            template<class TargetEBus, class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class TargetEBus, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        EBusRouter<EBus>::EBusRouter()
            : m_isConnected(false)
        {
            m_routerNode.m_handler = this;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        EBusRouter<EBus>::~EBusRouter()
        {
            BusRouterDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        void EBusRouter<EBus>::BusRouterConnect(int order)
        {
            if (!m_isConnected)
            {
                m_routerNode.m_order = order;
                auto& context = EBus::GetOrCreateContext();
                // We could support connection/disconnection while routing a message, but it would require a call to a fix
                // function because there is already a stack entry. This is typically not a good pattern because routers are
                // executed often. If time is not important to you, you can always queue the connect/disconnect functions
                // on the TickBus or another safe bus.
                AZ_Assert(context.s_callstack->m_prev == nullptr, "Current we don't allow router connect while in a message on the bus!");
                {
                    AZStd::scoped_lock<decltype(context.m_contextMutex)> lock(context.m_contextMutex);
                    context.m_routing.m_routers.insert(&m_routerNode);
                }
                m_isConnected = true;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        void EBusRouter<EBus>::BusRouterDisconnect()
        {
            if (m_isConnected)
            {
                auto* context = EBus::GetContext();
                EBUS_ASSERT(context, "Internal error: context deleted while router attached.");
                {
                    AZStd::scoped_lock<decltype(context->m_contextMutex)> lock(context->m_contextMutex);
                    // We could support connection/disconnection while routing a message, but it would require a call to a fix
                    // function because there is already a stack entry. This is typically not a good pattern because routers are
                    // executed often. If time is not important to you, you can always queue the connect/disconnect functions
                    // on the TickBus or another safe bus.
                    AZ_Assert(context->s_callstack->m_prev == nullptr, "Current we don't allow router disconnect while in a message on the bus!");
                    context->m_routing.m_routers.erase(&m_routerNode);
                }
                m_isConnected = false;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        bool EBusRouter<EBus>::BusRouterIsConnected() const
        {
            return m_isConnected;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class TargetEBus, class Event, class... Args>
        void EBusRouter<EBus>::ForwardEvent(Event event, Args&&... args)
        {
            EBusRouterForwarderHelper<EBus, TargetEBus>::ForwardEvent(event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class Result, class TargetEBus, class Event, class... Args>
        void EBusRouter<EBus>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            EBusRouterForwarderHelper<EBus, TargetEBus>::ForwardEventResult(result, event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class Container>
        void EBusNestedVersionRouter<EBus>::BusRouterConnect(Container& container, int order)
        {
            m_routerNode.m_handler = this;
            m_routerNode.m_order = order;
            // We don't need to worry about removing this because we
            // will be alive as long as the container is.
            container.insert(&m_routerNode);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class Container>
        void EBusNestedVersionRouter<EBus>::BusRouterDisconnect(Container& container)
        {
            container.erase(&m_routerNode);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class TargetEBus, class Event, class... Args>
        void EBusNestedVersionRouter<EBus>::ForwardEvent(Event event, Args&&... args)
        {
            EBusRouterForwarderHelper<EBus, TargetEBus>::ForwardEvent(event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus>
        template<class Result, class TargetEBus, class Event, class... Args>
        void EBusNestedVersionRouter<EBus>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            EBusRouterForwarderHelper<EBus, TargetEBus>::ForwardEventResult(result, event, args...);
        }
    } // namespace Internal
}
