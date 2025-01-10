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

            NonIdHandler();
            NonIdHandler(const NonIdHandler& rhs);
            NonIdHandler& operator=(const NonIdHandler& rhs);
            NonIdHandler(NonIdHandler&& rhs);
            NonIdHandler& operator=(NonIdHandler&& rhs);
            virtual ~NonIdHandler();

            void BusConnect();
            void BusDisconnect();
            bool BusIsConnected() const;

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

            IdHandler();
            IdHandler(const IdHandler& rhs);
            IdHandler& operator=(const IdHandler& rhs);
            IdHandler(IdHandler&& rhs);
            IdHandler& operator=(IdHandler&& rhs);
            virtual ~IdHandler();

            void BusConnect(const IdType& id);
            void BusDisconnect(const IdType& id);
            void BusDisconnect();
            bool BusIsConnectedId(const IdType& id) const;
            bool BusIsConnected() const;

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

            MultiHandler();
            MultiHandler(const MultiHandler& rhs);
            MultiHandler& operator=(const MultiHandler& rhs);
            MultiHandler(MultiHandler&& rhs);
            MultiHandler& operator=(MultiHandler&& rhs);
            virtual ~MultiHandler();

            void BusConnect(const IdType& id);
            void BusDisconnect(const IdType& id);
            void BusDisconnect();
            bool BusIsConnectedId(const IdType& id) const;
            bool BusIsConnected() const;

        private:
            AZStd::unordered_map<IdType, HandlerNode*, AZStd::hash<IdType>, AZStd::equal_to<IdType>, typename Traits::AllocatorType> m_handlerNodes;
        };
    } // namespace Internal
} // namespace AZ
