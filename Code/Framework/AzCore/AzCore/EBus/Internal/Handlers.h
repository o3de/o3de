/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Internal/StoragePolicies.h>
#include <AzCore/EBus/Internal/Debug.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/typetraits/is_polymorphic.h>

namespace AZ
{
    template <typename Interface, typename Traits>
    class EBus;

    struct NullMutex;

    namespace Internal
    {
        /**
         * HandlerNodes, stored by Handlers, point to the interface.
         * This class should be able to act as an Interface*.
         */
        template <typename Interface, typename Traits, typename HandlerHolder, bool /*hasId*/ = Traits::AddressPolicy != AZ::EBusAddressPolicy::Single>
        class HandlerNode
            : public HandlerStorageNode<HandlerNode<Interface, Traits, HandlerHolder, true>, Traits::HandlerPolicy>
        {
        public:
            HandlerNode(Interface* inst)
                : m_interface(inst)
            {
            }

            HandlerNode(const HandlerNode& rhs)
            {
                *this = rhs;
            }
            HandlerNode(HandlerNode&& rhs)
            {
                *this = AZStd::move(rhs);
            }
            HandlerNode& operator=(const HandlerNode& rhs)
            {
                m_interface = rhs.m_interface;
                m_holder = rhs.m_holder;
                return *this;
            }
            HandlerNode& operator=(HandlerNode&& rhs)
            {
                m_interface = AZStd::move(rhs.m_interface);
                m_holder = AZStd::move(rhs.m_holder);
                return *this;
            }

            typename Traits::BusIdType GetBusId() const
            {
                return m_holder->m_busId;
            }

            operator Interface*() const
            {
                return m_interface;
            }
            Interface* operator->() const
            {
                return m_interface;
            }
            // Allows resetting of the pointer
            HandlerNode& operator=(Interface* inst)
            {
                m_interface = inst;
                // If assigning to nullptr, do a full reset
                if (inst == nullptr)
                {
                    m_holder.reset();
                }
                return *this;
            }

            Interface* m_interface = nullptr;
            // This is stored as an intrusive_ptr so that the holder doesn't get destroyed while handlers are still inside it.
            AZStd::intrusive_ptr<HandlerHolder> m_holder;
        };
        template <typename Interface, typename Traits, typename HandlerHolder>
        class HandlerNode<Interface, Traits, HandlerHolder, false>
            : public HandlerStorageNode<HandlerNode<Interface, Traits, HandlerHolder, false>, Traits::HandlerPolicy>
        {
        public:
            HandlerNode(Interface* inst)
                : m_interface(inst)
            {
            }

            HandlerNode(const HandlerNode& rhs)
            {
                *this = rhs;
            }
            HandlerNode(HandlerNode&& rhs)
            {
                *this = AZStd::move(rhs);
            }
            HandlerNode& operator=(const HandlerNode& rhs)
            {
                m_interface = rhs.m_interface;
                return *this;
            }
            HandlerNode& operator=(HandlerNode&& rhs)
            {
                m_interface = AZStd::move(rhs.m_interface);
                return *this;
            }

            operator Interface*() const
            {
                return m_interface;
            }
            Interface* operator->() const
            {
                return m_interface;
            }
            // Allows resetting of the pointer
            HandlerNode& operator=(Interface* inst)
            {
                m_interface = inst;
                return *this;
            }

            Interface* m_interface = nullptr;
        };

        /**
         * Handler class used for buses with only a single address (no id type).
         */
        template <typename Interface, typename Traits, typename ContainerType>
        class NonIdHandler
            : public Interface
        {
        public:
            using BusType = AZ::EBus<Interface, Traits>;

            NonIdHandler()
                : m_node(nullptr)
            { }

            // Copy
            NonIdHandler(const NonIdHandler& rhs)
                : m_node(nullptr)
            {
                *this = rhs;
            }
            NonIdHandler& operator=(const NonIdHandler& rhs)
            {
                BusDisconnect();
                if (rhs.BusIsConnected())
                {
                    BusConnect();
                }
                return *this;
            }

            // Move
            NonIdHandler(NonIdHandler&& rhs)
                : m_node(nullptr)
            {
                *this = AZStd::move(rhs);
            }
            NonIdHandler& operator=(NonIdHandler&& rhs)
            {
                BusDisconnect();
                if (rhs.BusIsConnected())
                {
                    rhs.BusDisconnect();
                    BusConnect();
                }
                return *this;
            }

            virtual ~NonIdHandler()
            {
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant (for Traits::LocklessDispatch in asserts)
                AZ_Assert((!AZStd::is_polymorphic<typename BusType::InterfaceType>::value || AZStd::is_same<typename BusType::MutexType, AZ::NullMutex>::value || !BusIsConnected()), "EBus handlers must be disconnected prior to destruction on multi-threaded buses with virtual functions");
AZ_POP_DISABLE_WARNING

                if (BusIsConnected())
                {
                    BusDisconnect();
                }
                EBUS_ASSERT(!BusIsConnected(), "Internal error: Bus was not properly disconnected!");
            }

            void BusConnect();
            void BusDisconnect();

            bool BusIsConnected() const
            {
                return static_cast<Interface*>(m_node) != nullptr;
            }

        private:
            // Must be a member and not a base type so that Interface may be an incomplete type.
            typename ContainerType::HandlerNode m_node;
        };

        /**
         * Handler class used for buses with only multiple addresses (provides an id type).
         */
        template <typename Interface, typename Traits, typename ContainerType>
        class IdHandler
            : public Interface
        {
        private:
            using IdType = typename Traits::BusIdType;

        public:
            using BusType = AZ::EBus<Interface, Traits>;

            IdHandler()
                : m_node(nullptr)
            { }

            // Copy
            IdHandler(const IdHandler& rhs)
                : m_node(nullptr)
            {
                *this = rhs;
            }
            IdHandler& operator=(const IdHandler& rhs)
            {
                BusDisconnect();
                if (rhs.BusIsConnected())
                {
                    BusConnect(rhs.m_node.GetBusId());
                }
                return *this;
            }

            // Move
            IdHandler(IdHandler&& rhs)
                : m_node(nullptr)
            {
                *this = AZStd::move(rhs);
            }
            IdHandler& operator=(IdHandler&& rhs)
            {
                BusDisconnect();
                if (rhs.BusIsConnected())
                {
                    IdType id = rhs.m_node.GetBusId();
                    rhs.BusDisconnect(id);
                    BusConnect(id);
                }
                return *this;
            }

            virtual ~IdHandler()
            {
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant (for Traits::LocklessDispatch in asserts)
                AZ_Assert((!AZStd::is_polymorphic<typename BusType::InterfaceType>::value || AZStd::is_same_v<typename BusType::MutexType, AZ::NullMutex> || !BusIsConnected()), "EBus handlers must be disconnected prior to destruction on multi-threaded buses with virtual functions");
AZ_POP_DISABLE_WARNING

                if (BusIsConnected())
                {
                    BusDisconnect();
                }
                EBUS_ASSERT(!BusIsConnected(), "Internal error: Bus was not properly disconnected!");
            }

            void BusConnect(const IdType& id);
            void BusDisconnect(const IdType& id);
            void BusDisconnect();

            bool BusIsConnectedId(const IdType& id) const
            {
                return BusIsConnected() && m_node.GetBusId() == id;
            }

            bool BusIsConnected() const
            {
                return m_node.m_holder != nullptr;
            }

        private:
            // Must be a member and not a base type so that Interface may be an incomplete type.
            typename ContainerType::HandlerNode m_node;
        };

        /**
         * Allows handlers of a bus with multiple addresses (provides an id type) to connect to multiple addresses at the same time.
         */
        template <typename Interface, typename Traits, typename ContainerType>
        class MultiHandler
            : public Interface
        {
        private:
            using IdType = typename Traits::BusIdType;
            using HandlerNode = typename ContainerType::HandlerNode;

        public:
            using BusType = AZ::EBus<Interface, Traits>;

            MultiHandler() = default;

            // Copy
            MultiHandler(const MultiHandler& rhs)
            {
                *this = rhs;
            }
            MultiHandler& operator=(const MultiHandler& rhs)
            {
                BusDisconnect();
                for (const auto& nodePair : rhs.m_handlerNodes)
                {
                    BusConnect(nodePair.first);
                }
                return *this;
            }

            // Move
            MultiHandler(MultiHandler&& rhs)
            {
                *this = AZStd::move(rhs);
            }
            MultiHandler& operator=(MultiHandler&& rhs)
            {
                BusDisconnect();
                for (const auto& nodePair : rhs.m_handlerNodes)
                {
                    BusConnect(nodePair.first);
                }
                rhs.BusDisconnect();
                return *this;
            }

            virtual ~MultiHandler()
            {
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant (for Traits::LocklessDispatch in asserts)
                AZ_Assert((!AZStd::is_polymorphic<typename BusType::InterfaceType>::value || AZStd::is_same<typename BusType::MutexType, AZ::NullMutex>::value || !BusIsConnected()), "EBus handlers must be disconnected prior to destruction on multi-threaded buses with virtual functions");
AZ_POP_DISABLE_WARNING

                if (BusIsConnected())
                {
                    BusDisconnect();
                }
                EBUS_ASSERT(!BusIsConnected(), "Internal error: Bus was not properly disconnected!");
            }

            void BusConnect(const IdType& id);
            void BusDisconnect(const IdType& id);
            void BusDisconnect();

            bool BusIsConnectedId(const IdType& id) const
            {
                return m_handlerNodes.end() != m_handlerNodes.find(id);
            }

            bool BusIsConnected() const
            {
                return !m_handlerNodes.empty();
            }

        private:
            AZStd::unordered_map<IdType, HandlerNode*, AZStd::hash<IdType>, AZStd::equal_to<IdType>, typename Traits::AllocatorType> m_handlerNodes;
        };
    } // namespace Internal
} // namespace AZ
