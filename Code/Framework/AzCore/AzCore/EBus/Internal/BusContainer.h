/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <AzCore/EBus/Internal/CallstackEntry.h>
#include <AzCore/EBus/Internal/Handlers.h>
#include <AzCore/EBus/Internal/StoragePolicies.h>
#include <AzCore/EBus/Internal/Debug.h>

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")

namespace AZ
{
    namespace Internal
    {
        // Helpers for the BusContainer implementations
        namespace
        {
            // Handles updating iterators when disconnecting mid-dispatch
            template <typename Bus, typename PreHandler, typename PostHandler>
            class MidDispatchDisconnectFixer
                : public AZ::Internal::CallstackEntry<typename Bus::InterfaceType, typename Bus::Traits>
            {
                using Base = AZ::Internal::CallstackEntry<typename Bus::InterfaceType, typename Bus::Traits>;

            public:
                template<typename PreRemoveHandler, typename PostRemoveHandler>
                MidDispatchDisconnectFixer(typename Bus::Context* context, const typename Bus::BusIdType* busId, PreRemoveHandler&& pre, PostRemoveHandler&& post)
                    : Base(context, busId)
                    , m_onPreDisconnect(AZStd::forward<PreRemoveHandler>(pre))
                    , m_onPostDisconnect(AZStd::forward<PostRemoveHandler>(post))
                {
                    static_assert(AZStd::is_invocable<PreHandler, typename Bus::InterfaceType*>::value, "Pre handler requires accepting a parameter of Bus::InterfaceType pointer");
                    static_assert(AZStd::is_invocable<PostHandler>::value, "Post handler does not accept any parameters");
                }

                void OnRemoveHandler(typename Bus::InterfaceType* handler) override
                {
                    m_onPreDisconnect(handler);
                    Base::OnRemoveHandler(handler);
                }

                void OnPostRemoveHandler() override
                {
                    m_onPostDisconnect();
                    Base::OnPostRemoveHandler();
                }

            private:
                PreHandler m_onPreDisconnect;
                PostHandler m_onPostDisconnect;
            };

            // Helper for creating a MidDispatchDisconnectFixer, with support for deducing handler types (required for lambdas)
            template <typename Bus, typename PreHandler, typename PostHandler>
            auto MakeDisconnectFixer(typename Bus::Context* context, const typename Bus::BusIdType* busId, PreHandler&& remove, PostHandler&& post)
                -> MidDispatchDisconnectFixer<Bus, PreHandler, PostHandler>
            {
                return MidDispatchDisconnectFixer<Bus, PreHandler, PostHandler>(context, busId, AZStd::forward<PreHandler>(remove), AZStd::forward<PostHandler>(post));
            }
        }

// Executes router handling in a generic way
#define EBUS_DO_ROUTING(contextParam, id, isQueued, isReverse) \
    do {                                                                                        \
        auto& local_context = (contextParam);                                                   \
        if (local_context.m_routing.m_routers.size()) {                                         \
            if (local_context.m_routing.RouteEvent(id, isQueued, isReverse, func, args...)) {   \
                return;                                                                         \
            }                                                                                   \
        }                                                                                       \
    } while(false)

        // Default impl, used when there are multiple addresses and multiple handlers
        template <typename Interface, typename Traits, EBusAddressPolicy addressPolicy = Traits::AddressPolicy, EBusHandlerPolicy handlerPolicy = Traits::HandlerPolicy>
        struct EBusContainer
        {
        public:
            using ContainerType = EBusContainer;
            using IdType = typename Traits::BusIdType;

            using CallstackEntry = AZ::Internal::CallstackEntry<Interface, Traits>;

            // This struct will hold the handlers per address
            struct HandlerHolder;
            // This struct will hold each handler
            using HandlerNode = HandlerNode<Interface, Traits, HandlerHolder>;
            // Defines how handler holders are stored (will be some sort of map-like structure from id -> handler holder)
            using AddressStorage = AddressStoragePolicy<Traits, HandlerHolder>;
            // Defines how handlers are stored per address (will be some sort of list)
            using HandlerStorage = HandlerStoragePolicy<Interface, Traits, HandlerNode>;

            using Handler = IdHandler<Interface, Traits, ContainerType>;
            using MultiHandler = MultiHandler<Interface, Traits, ContainerType>;
            using BusPtr = AZStd::intrusive_ptr<HandlerHolder>;

            EBusContainer() = default;

            // EBus will extend this class to gain the Event*/Broadcast* functions
            template <typename Bus>
            struct Dispatcher
            {
                // Event family
                template <typename Function, typename... ArgsT>
                static void Event(const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.begin();
                            auto handlersEnd = handlers.end();

                            auto fixer = MakeDisconnectFixer<Bus>(context, &id,
                                [&handlerIt, &handlersEnd](Interface* handler)
                                {
                                     if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                    {
                                        ++handlerIt;
                                    }
                                },
                                [&handlers, &handlersEnd]()
                                {
                                    handlersEnd = handlers.end();
                                }
                            );

                            while (handlerIt != handlersEnd)
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::Call(func, *itr, args...);
                            }

                            holder.release();
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResult(Results& results, const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.begin();
                            auto handlersEnd = handlers.end();

                            auto fixer = MakeDisconnectFixer<Bus>(context, &id,
                                [&handlerIt, &handlersEnd](Interface* handler)
                                {
                                    if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                    {
                                        ++handlerIt;
                                    }
                                },
                                [&handlers, &handlersEnd]()
                                {
                                    handlersEnd = handlers.end();
                                }
                            );

                            while (handlerIt != handlersEnd)
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                            }

                            holder.release();
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void EventReverse(const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.rbegin();

                            CallstackEntry entry(context, &id);
                            while (handlerIt != handlers.rend())
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::Call(func, *itr, args...);
                            }

                            holder.release();
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResultReverse(Results& results, const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.rbegin();

                            CallstackEntry entry(context, &id);
                            while (handlerIt != handlers.rend())
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                            }

                            holder.release();
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void Event(const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, false);

                        auto& handlers = busPtr->m_handlers;
                        auto handlerIt = handlers.begin();
                        auto handlersEnd = handlers.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, &busPtr->m_busId,
                            [&handlerIt, &handlersEnd](Interface* handler)
                            {
                                if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                {
                                    ++handlerIt;
                                }
                            },
                            [&handlers, &handlersEnd]()
                            {
                                handlersEnd = handlers.end();
                            }
                        );

                        while (handlerIt != handlersEnd)
                        {
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::Call(func, *itr, args...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResult(Results& results, const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, false);

                        auto& handlers = busPtr->m_handlers;
                        auto handlerIt = handlers.begin();
                        auto handlersEnd = handlers.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, &busPtr->m_busId,
                            [&handlerIt, &handlersEnd](Interface* handler)
                            {
                                if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                {
                                    ++handlerIt;
                                }
                            },
                            [&handlers, &handlersEnd]()
                            {
                                handlersEnd = handlers.end();
                            }
                        );

                        while (handlerIt != handlersEnd)
                        {
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void EventReverse(const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, true);

                        auto& handlers = busPtr->m_handlers;
                        auto handlerIt = handlers.rbegin();

                        CallstackEntry entry(context, &busPtr->m_busId);
                        while (handlerIt != handlers.rend())
                        {
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::Call(func, *itr, args...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResultReverse(Results& results, const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, true);

                        auto& handlers = busPtr->m_handlers;
                        auto handlerIt = handlers.rbegin();

                        CallstackEntry entry(context, &busPtr->m_busId);
                        while (handlerIt != handlers.rend())
                        {
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                        }
                    }
                }

                // Broadcast family
                template <typename Function, typename... ArgsT>
                static void Broadcast(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        while (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.begin();
                            auto handlersEnd = handlers.end();

                            auto fixer = MakeDisconnectFixer<Bus>(context, &holder.m_busId,
                                [&handlerIt, &handlersEnd](Interface* handler)
                                {
                                    if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                    {
                                        ++handlerIt;
                                    }
                                },
                                [&handlers, &handlersEnd]()
                                {
                                    handlersEnd = handlers.end();
                                }
                            );

                            while (handlerIt != handlersEnd)
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::Call(func, *itr, args...);
                            }

                            // Increment before release so that if holder goes away, iterator is still valid
                            ++addressIt;

                            holder.release();
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResult(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        while (addressIt != addresses.end())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.begin();
                            auto handlersEnd = handlers.end();

                            auto fixer = MakeDisconnectFixer<Bus>(context, &holder.m_busId,
                                [&handlerIt, &handlersEnd](Interface* handler)
                                {
                                    if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                    {
                                        ++handlerIt;
                                    }
                                },
                                [&handlers, &handlersEnd]()
                                {
                                    handlersEnd = handlers.end();
                                }
                            );

                            while (handlerIt != handlersEnd)
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                            }

                            // Increment before release so that if holder goes away, iterator is still valid
                            ++addressIt;

                            holder.release();
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void BroadcastReverse(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.rbegin();
                        while (addressIt != addresses.rend())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            HandlerHolder* nextHolder = nullptr;
                            if (addressIt != addresses.rbegin())
                            {
                                // The reverse iterator (addressIt) internally points to the previous element (the next forward iterator).
                                // It's important to make sure that this iterator stays valid, so we hold a lock on that element until we
                                // have moved to the next one. Moving to the next element will happen after processing current bus. While
                                // processing, we can change the bus container (add/remove listeners).
                                nextHolder = &*AZStd::prev(addressIt);
                                nextHolder->add_ref();
                            }

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.rbegin();

                            CallstackEntry entry(context, &holder.m_busId);
                            while (handlerIt != handlers.rend())
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::Call(func, *itr, args...);
                            }
                            holder.release();

                            // Since rend() internally points to begin(), it might have changed during the message (e.g., bus was removed).
                            // Make sure that we are not at the end before moving to the next element, which would be an invalid operation.
                            if (addressIt != addresses.rend())
                            {
                                // During message processing we could have removed/added buses. Since the reverse iterator points
                                // to the next forward iterator, the elements it points to can change. In this case, we don't need
                                // to move the reverse iterator.
                                if (&*addressIt == &holder)
                                {
                                    ++addressIt; // If we did not remove the bus we just processed, move to the next one.
                                }
                            }

                            if (nextHolder)
                            {
                                // We are done moving the ebCurrentBus iterator. It's safe to release the lock on the element the iterator was pointing to.
                                nextHolder->release();
                            }
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResultReverse(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.rbegin();
                        while (addressIt != addresses.rend())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            HandlerHolder* nextHolder = nullptr;
                            if (addressIt != addresses.rbegin())
                            {
                                // The reverse iterator (addressIt) internally points to the previous element (the next forward iterator).
                                // It's important to make sure that this iterator stays valid, so we hold a lock on that element until we
                                // have moved to the next one. Moving to the next element will happen after processing current bus. While
                                // processing, we can change the bus container (add/remove listeners).
                                nextHolder = &*AZStd::prev(addressIt);
                                nextHolder->add_ref();
                            }

                            auto& handlers = holder.m_handlers;
                            auto handlerIt = handlers.rbegin();

                            CallstackEntry entry(context, &holder.m_busId);
                            while (handlerIt != handlers.rend())
                            {
                                auto itr = handlerIt++;
                                Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                            }
                            holder.release();

                            // Since rend() internally points to begin(), it might have changed during the message (e.g., bus was removed).
                            // Make sure that we are not at the end before moving to the next element, which would be an invalid operation.
                            if (addressIt != addresses.rend())
                            {
                                // During message processing we could have removed/added buses. Since the reverse iterator points
                                // to the next forward iterator, the elements it points to can change. In this case, we don't need
                                // to move the reverse iterator.
                                if (&*addressIt == &holder)
                                {
                                    ++addressIt; // If we did not remove the bus we just processed, move to the next one.
                                }
                            }

                            if (nextHolder)
                            {
                                // We are done moving the ebCurrentBus iterator. It's safe to release the lock on the element the iterator was pointing to.
                                nextHolder->release();
                            }
                        }
                    }
                }

                // Enumerate family
                template <class Callback>
                static void EnumerateHandlers(Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        auto addressesEnd = addresses.end();
                        while (addressIt != addressesEnd)
                        {
                            if (!context->m_buses.EnumerateHandlersImpl(context, *addressIt++, AZStd::forward<Callback>(callback)))
                            {
                                break;
                            }
                        }
                    }
                }
                template <class Callback>
                static void EnumerateHandlersId(const IdType& id, Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            context->m_buses.EnumerateHandlersImpl(context, *addressIt, AZStd::forward<Callback>(callback));
                        }
                    }
                }
                template <class Callback>
                static void EnumerateHandlersPtr(const BusPtr& ptr, Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        if (ptr)
                        {
                            context->m_buses.EnumerateHandlersImpl(context, *ptr, AZStd::forward<Callback>(callback));
                        }
                    }
                }
            };

            // All enumerate functions do basically the same thing once they have a holder, so implement it here
            template <class Callback>
            static bool EnumerateHandlersImpl(void* context, HandlerHolder& holder, Callback&& callback)
            {
                auto& handlers = holder.m_handlers;
                auto handlerIt = handlers.begin();
                auto handlersEnd = handlers.end();

                // This must be done via void* and static cast because the EBus type
                // is not available for resolution while function signatures are compiled.
                using BusType = EBus<Interface, Traits>;
                auto fixer = MakeDisconnectFixer<BusType>(static_cast<typename BusType::Context*>(context), &holder.m_busId,
                    [&handlerIt, &handlersEnd](Interface* handler)
                    {
                        if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                        {
                            ++handlerIt;
                        }
                    },
                    [&handlers, &handlersEnd]()
                    {
                        handlersEnd = handlers.end();
                    }
                );

                bool shouldContinue = true;
                while (handlerIt != handlersEnd)
                {
                    bool result = false;
                    auto itr = handlerIt++;
                    Traits::EventProcessingPolicy::CallResult(result, callback, itr->m_interface);

                    if (!result)
                    {
                        shouldContinue = false;
                        break;
                    }
                }

                return shouldContinue;
            }

            void Bind(BusPtr& busPtr, const IdType& id)
            {
                busPtr = &FindOrCreateHandlerHolder(id);
            }

            struct HandlerHolder
            {
                ContainerType& m_busContainer;
                IdType m_busId;
                typename HandlerStorage::StorageType m_handlers;
                AZStd::atomic_uint m_refCount{ 0 };

                HandlerHolder(ContainerType& storage, const IdType& id)
                    : m_busContainer(storage)
                    , m_busId(id)
                { }

                HandlerHolder(HandlerHolder&& rhs)
                    : m_busContainer(rhs.m_busContainer)
                    , m_busId(rhs.m_busId)
                    , m_handlers(AZStd::move(rhs.m_handlers))
                {
                    m_refCount.store(rhs.m_refCount.load());
                    rhs.m_refCount.store(0);
                }

                HandlerHolder(const HandlerHolder&) = delete;
                HandlerHolder& operator=(const HandlerHolder&) = delete;
                HandlerHolder& operator=(HandlerHolder&&) = delete;

                ~HandlerHolder() = default;

                // Returns true if this HandlerHolder actually has handers.
                // This will return false when this id is Bind'ed to, but there are no handlers.
                bool HasHandlers() const
                {
                    return !m_handlers.empty();
                }

                void add_ref()
                {
                    m_refCount.fetch_add(1);
                }
                void release()
                {
                    // Must check against 1 because fetch_sub returns the value before decrementing
                    if (m_refCount.fetch_sub(1) == 1)
                    {
                        m_busContainer.m_addresses.erase(m_busId);
                    }
                }
            };

            HandlerHolder& FindOrCreateHandlerHolder(const IdType& id)
            {
                auto addressIt = m_addresses.find(id);
                if (addressIt == m_addresses.end())
                {
                    addressIt = m_addresses.emplace(*this, id);
                }

                return *addressIt;
            }

            void Connect(HandlerNode& handler, const IdType& id)
            {
                EBUS_ASSERT(!handler.m_holder,
                    "Internal error: Connect() may not be called by a handler that is already connected."
                    "BusConnect() should already have handled this case.");

                HandlerHolder& holder = FindOrCreateHandlerHolder(id);
                holder.m_handlers.insert(handler);
                handler.m_holder = &holder;
            }

            void Disconnect(HandlerNode& handler)
            {
                EBUS_ASSERT(handler.m_holder, "Internal error: disconnecting handler that is incompletely connected");

                handler.m_holder->m_handlers.erase(handler);

                // Must reset handler after removing it from the list, otherwise m_holder could have been destroyed already (and handlerList would be invalid)
                handler.m_holder.reset();
            }

            typename AddressStorage::StorageType m_addresses;
        };

        // Specialization for multi address, single handler
        template <typename Interface, typename Traits, EBusAddressPolicy addressPolicy>
        struct EBusContainer<Interface, Traits, addressPolicy, EBusHandlerPolicy::Single>
        {
        public:
            using ContainerType = EBusContainer;
            using IdType = typename Traits::BusIdType;

            using CallstackEntry = AZ::Internal::CallstackEntry<Interface, Traits>;

            // This struct will hold the handler per address
            struct HandlerHolder;
            // This struct will hold each handler
            using HandlerNode = HandlerNode<Interface, Traits, HandlerHolder>;
            // Defines how handler holders are stored (will be some sort of map-like structure from id -> handler holder)
            using AddressStorage = AddressStoragePolicy<Traits, HandlerHolder>;
            // No need for HandlerStorage, there's only 1 so it will always just be a HandlerNode*

            using Handler = IdHandler<Interface, Traits, ContainerType>;
            using MultiHandler = MultiHandler<Interface, Traits, ContainerType>;
            using BusPtr = AZStd::intrusive_ptr<HandlerHolder>;

            EBusContainer() = default;

            // EBus will extend this class to gain the Event*/Broadcast* functions
            template <typename Bus>
            struct Dispatcher
            {
                // Event family
                template <typename Function, typename... ArgsT>
                static void Event(const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end() && addressIt->m_interface)
                        {
                            CallstackEntry entry(context, &addressIt->m_busId);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), addressIt->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResult(Results& results, const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end() && addressIt->m_interface)
                        {
                            CallstackEntry entry(context, &addressIt->m_busId);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), addressIt->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void EventReverse(const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end() && addressIt->m_interface)
                        {
                            CallstackEntry entry(context, &addressIt->m_busId);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), addressIt->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResultReverse(Results& results, const IdType& id, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, &id, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end() && addressIt->m_interface)
                        {
                            CallstackEntry entry(context, &addressIt->m_busId);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), addressIt->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void Event(const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, false);

                        if (busPtr->m_interface)
                        {
                            CallstackEntry entry(context, &busPtr->m_busId);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), busPtr->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResult(Results& results, const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, false);

                        if (busPtr->m_interface)
                        {
                            CallstackEntry entry(context, &busPtr->m_busId);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), busPtr->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void EventReverse(const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, true);

                        if (busPtr->m_interface)
                        {
                            CallstackEntry entry(context, &busPtr->m_busId);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), busPtr->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void EventResultReverse(Results& results, const BusPtr& busPtr, Function&& func, ArgsT&&... args)
                {
                    if (busPtr)
                    {
                        auto* context = Bus::GetContext();
                        EBUS_ASSERT(context, "Internal error: context deleted with bind ptr outstanding.");
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        EBUS_DO_ROUTING(*context, &busPtr->m_busId, false, true);

                        if (busPtr->m_interface)
                        {
                            CallstackEntry entry(context, &busPtr->m_busId);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), busPtr->m_interface, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }

                // Broadcast family
                template <typename Function, typename... ArgsT>
                static void Broadcast(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        auto addressesEnd = addresses.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, nullptr,
                            [&addressIt, &addressesEnd](Interface* handler)
                            {
                                if (addressIt != addressesEnd && addressIt->m_handler->m_interface == handler)
                                {
                                    ++addressIt;
                                }
                            },
                            [&addresses, &addressesEnd]()
                            {
                                addressesEnd = addresses.end();
                            }
                        );

                        while (addressIt != addressesEnd)
                        {
                            fixer.m_busId = &addressIt->m_busId;
                            if (Interface* inst = (addressIt++)->m_interface)
                            {
                                // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                                // due to potential of multiple addresses of this EBus container invoking the function multiple times
                                Traits::EventProcessingPolicy::Call(func, inst, args...);
                            }
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResult(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        auto addressesEnd = addresses.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, nullptr,
                            [&addressIt, &addressesEnd](Interface* handler)
                            {
                                if (addressIt != addressesEnd && addressIt->m_handler->m_interface == handler)
                                {
                                    ++addressIt;
                                }
                            },
                            [&addresses, &addressesEnd]()
                            {
                                addressesEnd = addresses.end();
                            }
                        );

                        while (addressIt != addressesEnd)
                        {
                            fixer.m_busId = &addressIt->m_busId;
                            if (Interface* inst = (addressIt++)->m_interface)
                            {
                                // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                                // due to potential of multiple addresses of this EBus container invoking the function multiple times
                                Traits::EventProcessingPolicy::CallResult(results, func, inst, args...);
                            }
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void BroadcastReverse(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.rbegin();
                        while (addressIt != addresses.rend())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            HandlerHolder* nextHolder = nullptr;
                            if (addressIt != addresses.rbegin())
                            {
                                // The reverse iterator (addressIt) internally points to the previous element (the next forward iterator).
                                // It's important to make sure that this iterator stays valid, so we hold a lock on that element until we
                                // have moved to the next one. Moving to the next element will happen after processing current bus. While
                                // processing, we can change the bus container (add/remove listeners).
                                nextHolder = &*AZStd::prev(addressIt);
                                nextHolder->add_ref();
                            }

                            if (Interface* inst = holder.m_interface)
                            {
                                CallstackEntry entry(context, &holder.m_busId);
                                // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                                // due to potential of multiple addresses of this EBus container invoking the function multiple times
                                Traits::EventProcessingPolicy::Call(func, inst, args...);
                            }
                            holder.release();

                            // Since rend() internally points to begin(), it might have changed during the message (e.g., bus was removed).
                            // Make sure that we are not at the end before moving to the next element, which would be an invalid operation.
                            if (addressIt != addresses.rend())
                            {
                                // During message processing we could have removed/added buses. Since the reverse iterator points
                                // to the next forward iterator, the elements it points to can change. In this case, we don't need
                                // to move the reverse iterator.
                                if (&*addressIt == &holder)
                                {
                                    ++addressIt; // If we did not remove the bus we just processed, move to the next one.
                                }
                            }

                            if (nextHolder)
                            {
                                // We are done moving the ebCurrentBus iterator. It's safe to release the lock on the element the iterator was pointing to.
                                nextHolder->release();
                            }
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResultReverse(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.rbegin();
                        while (addressIt != addresses.rend())
                        {
                            HandlerHolder& holder = *addressIt;
                            holder.add_ref();

                            HandlerHolder* nextHolder = nullptr;
                            if (addressIt != addresses.rbegin())
                            {
                                // The reverse iterator (addressIt) internally points to the previous element (the next forward iterator).
                                // It's important to make sure that this iterator stays valid, so we hold a lock on that element until we
                                // have moved to the next one. Moving to the next element will happen after processing current bus. While
                                // processing, we can change the bus container (add/remove listeners).
                                nextHolder = &*AZStd::prev(addressIt);
                                nextHolder->add_ref();
                            }

                            if (Interface* inst = holder.m_interface)
                            {
                                CallstackEntry entry(context, &holder.m_busId);
                                // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                                // due to potential of multiple addresses of this EBus container invoking the function multiple times
                                Traits::EventProcessingPolicy::CallResult(results, func, inst, args...);
                            }
                            holder.release();

                            // Since rend() internally points to begin(), it might have changed during the message (e.g., bus was removed).
                            // Make sure that we are not at the end before moving to the next element, which would be an invalid operation.
                            if (addressIt != addresses.rend())
                            {
                                // During message processing we could have removed/added buses. Since the reverse iterator points
                                // to the next forward iterator, the elements it points to can change. In this case, we don't need
                                // to move the reverse iterator.
                                if (&*addressIt == &holder)
                                {
                                    ++addressIt; // If we did not remove the bus we just processed, move to the next one.
                                }
                            }

                            if (nextHolder)
                            {
                                // We are done moving the ebCurrentBus iterator. It's safe to release the lock on the element the iterator was pointing to.
                                nextHolder->release();
                            }
                        }
                    }
                }

                // Enumerate family
                template <class Callback>
                static void EnumerateHandlers(Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.begin();
                        auto addressesEnd = addresses.end();

                        auto fixer = MakeDisconnectFixer<Bus>(Bus::GetContext(), nullptr,
                            [&addressIt, &addressesEnd](Interface* handler)
                            {
                                if (addressIt != addressesEnd && addressIt->m_handler->m_interface == handler)
                                {
                                    ++addressIt;
                                }
                            },
                            [&addresses, &addressesEnd]()
                            {
                                addressesEnd = addresses.end();
                            }
                        );

                        while (addressIt != addressesEnd)
                        {
                            fixer.m_busId = &addressIt->m_busId;
                            if (Interface* inst = (addressIt++)->m_interface)
                            {
                                bool result = false;
                                Traits::EventProcessingPolicy::CallResult(result, callback, inst);
                                if (!result)
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
                template <class Callback>
                static void EnumerateHandlersId(const IdType& id, Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto& addresses = context->m_buses.m_addresses;
                        auto addressIt = addresses.find(id);
                        if (addressIt != addresses.end())
                        {
                            if (Interface* inst = addressIt->m_interface)
                            {
                                CallstackEntry entry(context, &id);
                                bool result = false;
                                Traits::EventProcessingPolicy::CallResult(result, callback, inst);
                                if (!result)
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
                template <class Callback>
                static void EnumerateHandlersPtr(const BusPtr& ptr, Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        if (ptr)
                        {
                            if (Interface* inst = ptr->m_interface)
                            {
                                CallstackEntry entry(context, &ptr->m_busId);
                                bool result = false;
                                Traits::EventProcessingPolicy::CallResult(result, callback, inst);
                                if (!result)
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
            };

            void Bind(BusPtr& busPtr, const IdType& id)
            {
                busPtr = &FindOrCreateHandlerHolder(id);
            }

            struct HandlerHolder
            {
                ContainerType& m_busContainer;
                IdType m_busId;
                HandlerNode* m_handler = nullptr;
                // Cache of the interface to save an indirection to m_handler
                Interface* m_interface = nullptr;
                AZStd::atomic_uint m_refCount{ 0 };

                HandlerHolder(ContainerType& storage, const IdType& id)
                    : m_busContainer(storage)
                    , m_busId(id)
                { }

                HandlerHolder(HandlerHolder&& rhs)
                    : m_busContainer(rhs.m_busContainer)
                    , m_busId(rhs.m_busId)
                    , m_handler(AZStd::move(rhs.m_handler))
                    , m_interface(AZStd::move(rhs.m_interface))
                {
                    m_refCount.store(rhs.m_refCount.load());
                    rhs.m_refCount.store(0);
                }

                HandlerHolder(const HandlerHolder&) = delete;
                HandlerHolder& operator=(const HandlerHolder&) = delete;
                HandlerHolder& operator=(HandlerHolder&&) = delete;

                ~HandlerHolder() = default;

                // Returns true if this HandlerHolder actually has handers.
                // This will return false when this id is Bind'ed to, but there are no handlers.
                bool HasHandlers() const
                {
                    return m_interface != nullptr;
                }

                void add_ref()
                {
                    m_refCount.fetch_add(1);
                }
                void release()
                {
                    // Must check against 1 because fetch_sub returns the value before decrementing
                    if (m_refCount.fetch_sub(1) == 1)
                    {
                        m_busContainer.m_addresses.erase(m_busId);
                    }
                }
            };

            HandlerHolder& FindOrCreateHandlerHolder(const IdType& id)
            {
                auto busIt = m_addresses.find(id);
                if (busIt == m_addresses.end())
                {
                    busIt = m_addresses.emplace(*this, id);
                }

                return *busIt;
            }

            void Connect(HandlerNode& handler, const IdType& id)
            {
                EBUS_ASSERT(!handler.m_holder,
                    "Internal error: Connect() may not be called by a handler that is already connected."
                    "BusConnect() should already have handled this case.");

                HandlerHolder& holder = FindOrCreateHandlerHolder(id);
                holder.m_handler = &handler;
                holder.m_interface = handler.m_interface;
                handler.m_holder = &holder;
            }

            void Disconnect(HandlerNode& handler)
            {
                EBUS_ASSERT(handler.m_holder, "Internal error: disconnecting handler that is incompletely connected");

                handler.m_holder->m_handler = nullptr;
                handler.m_holder->m_interface = nullptr;

                // Must reset handler after removing it from the list, otherwise m_holder could have been destroyed already (and handlerList would be invalid)
                handler.m_holder.reset();
            }

            typename AddressStorage::StorageType m_addresses;
        };

        // Specialization for single address, multi handler
        template <typename Interface, typename Traits, EBusHandlerPolicy handlerPolicy>
        struct EBusContainer<Interface, Traits, EBusAddressPolicy::Single, handlerPolicy>
        {
        public:
            using ContainerType = EBusContainer;
            using IdType = typename Traits::BusIdType;

            using CallstackEntry = AZ::Internal::CallstackEntry<Interface, Traits>;

            // This struct will hold the handlers per address
            struct HandlerHolder;
            // This struct will hold each handler
            using HandlerNode = HandlerNode<Interface, Traits, HandlerHolder>;
            // Defines how handlers are stored per address (will be some sort of list)
            using HandlerStorage = HandlerStoragePolicy<Interface, Traits, HandlerNode>;
            // No need for AddressStorage, there's only 1

            struct BusPtr { };
            using Handler = NonIdHandler<Interface, Traits, ContainerType>;

            EBusContainer() = default;

            // EBus will extend this class to gain the Event*/Broadcast* functions
            template <typename Bus>
            struct Dispatcher
            {
                // Broadcast family
                template <typename Function, typename... ArgsT>
                static void Broadcast(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& handlers = context->m_buses.m_handlers;
                        auto handlerIt = handlers.begin();
                        auto handlersEnd = handlers.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, nullptr,
                            [&handlerIt, &handlersEnd](Interface* handler)
                            {
                                if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                {
                                    ++handlerIt;
                                }
                            },
                            [&handlers, &handlersEnd]()
                            {
                                handlersEnd = handlers.end();
                            }
                        );

                        while (handlerIt != handlersEnd)
                        {
                            // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                            // due to potential of multiple handlers of this EBus container invoking the function multiple times
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::Call(func, *itr, args...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResult(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto& handlers = context->m_buses.m_handlers;
                        auto handlerIt = handlers.begin();
                        auto handlersEnd = handlers.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, nullptr,
                            [&handlerIt, &handlersEnd](Interface* handler)
                            {
                                if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                {
                                    ++handlerIt;
                                }
                            },
                            [&handlers, &handlersEnd]()
                            {
                                handlersEnd = handlers.end();
                            }
                        );

                        while (handlerIt != handlersEnd)
                        {
                            // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                            // due to potential of multiple handlers of this EBus container invoking the function multiple times
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void BroadcastReverse(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& handlers = context->m_buses.m_handlers;
                        auto handlerIt = handlers.rbegin();

                        CallstackEntry entry(context, nullptr);
                        while (handlerIt != handlers.rend())
                        {
                            // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                            // due to potential of multiple handlers of this EBus container invoking the function multiple times
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::Call(func, *itr, args...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResultReverse(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, true);

                        auto& handlers = context->m_buses.m_handlers;
                        auto handlerIt = handlers.rbegin();

                        CallstackEntry entry(context, nullptr);
                        while (handlerIt != handlers.rend())
                        {
                            // @func and @args cannot be forwarded here as rvalue arguments need to bind to const lvalue arguments
                            // due to potential of multiple handlers of this EBus container invoking the function multiple times
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::CallResult(results, func, *itr, args...);
                        }
                    }
                }

                // Enumerate family
                template <class Callback>
                static void EnumerateHandlers(Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto& handlers = context->m_buses.m_handlers;
                        auto handlerIt = handlers.begin();
                        auto handlersEnd = handlers.end();

                        auto fixer = MakeDisconnectFixer<Bus>(context, nullptr,
                            [&handlerIt, &handlersEnd](Interface* handler)
                            {
                                if (handlerIt != handlersEnd && handlerIt->m_interface == handler)
                                {
                                    ++handlerIt;
                                }
                            },
                            [&handlers, &handlersEnd]()
                            {
                                handlersEnd = handlers.end();
                            }
                        );

                        while (handlerIt != handlersEnd)
                        {
                            bool result = false;
                            auto itr = handlerIt++;
                            Traits::EventProcessingPolicy::CallResult(result, callback, itr->m_interface);
                            if (!result)
                            {
                                return;
                            }
                        }
                    }
                }
            };

            void Connect(HandlerNode& handler, const IdType&)
            {
                // Don't need to check for duplicates here, because BusConnect would have caught it already
                m_handlers.insert(handler);
            }

            void Disconnect(HandlerNode& handler)
            {
                // Don't need to check that handler is already connected here, because BusDisconnect would have caught it already
                m_handlers.erase(handler);
            }

            typename HandlerStorage::StorageType m_handlers;
        };

        // Specialization for single address, single handler
        template <typename Interface, typename Traits>
        struct EBusContainer<Interface, Traits, EBusAddressPolicy::Single, EBusHandlerPolicy::Single>
        {
        public:
            using ContainerType = EBusContainer;
            using IdType = typename Traits::BusIdType;

            using CallstackEntry = AZ::Internal::CallstackEntry<Interface, Traits>;

            // Handlers can just be stored as raw pointer
            // Needs to be public for ConnectionPolicy
            using HandlerNode = Interface*;
            // No need for AddressStorage, there's only 1
            // No need for HandlerStorage, there's only 1

            using Handler = NonIdHandler<Interface, Traits, ContainerType>;
            struct BusPtr { };

            EBusContainer() = default;

            // EBus will extend this class to gain the Event*/Broadcast* functions
            template <typename Bus>
            struct Dispatcher
            {
                // Broadcast family
                template <typename Function, typename... ArgsT>
                static void Broadcast(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto handler = context->m_buses.m_handler;
                        if (handler)
                        {
                            CallstackEntry entry(context, nullptr);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), handler, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResult(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto handler = context->m_buses.m_handler;
                        if (handler)
                        {
                            CallstackEntry entry(context, nullptr);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), handler, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Function, typename... ArgsT>
                static void BroadcastReverse(Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto handler = context->m_buses.m_handler;
                        if (handler)
                        {
                            CallstackEntry entry(context, nullptr);
                            Traits::EventProcessingPolicy::Call(AZStd::forward<Function>(func), handler, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }
                template <typename Results, typename Function, typename... ArgsT>
                static void BroadcastResultReverse(Results& results, Function&& func, ArgsT&&... args)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);
                        EBUS_DO_ROUTING(*context, nullptr, false, false);

                        auto handler = context->m_buses.m_handler;
                        if (handler)
                        {
                            CallstackEntry entry(context, nullptr);
                            Traits::EventProcessingPolicy::CallResult(results, AZStd::forward<Function>(func), handler, AZStd::forward<ArgsT>(args)...);
                        }
                    }
                }

                // Enumerate family
                template <class Callback>
                static void EnumerateHandlers(Callback&& callback)
                {
                    if (auto* context = Bus::GetContext())
                    {
                        typename Bus::Context::DispatchLockGuard lock(context->m_contextMutex);

                        auto handler = context->m_buses.m_handler;
                        if (handler)
                        {
                            CallstackEntry entry(context, nullptr);
                            Traits::EventProcessingPolicy::Call(callback, handler);
                        }
                    }
                }
            };

            void Connect(HandlerNode& handler, const IdType&)
            {
                AZ_Assert(!m_handler, "Bus already connected to!");
                m_handler = handler;
            }

            void Disconnect(HandlerNode& handler)
            {
                if (m_handler == handler)
                {
                    m_handler = nullptr;
                }
            }

            HandlerNode m_handler = nullptr;
        };
    }
}

AZ_POP_DISABLE_WARNING
