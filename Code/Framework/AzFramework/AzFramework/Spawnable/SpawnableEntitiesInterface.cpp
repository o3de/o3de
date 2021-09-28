/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework
{
    //
    // SpawnableEntityContainerView
    //

    SpawnableEntityContainerView::SpawnableEntityContainerView(AZ::Entity** begin, size_t length)
        : m_begin(begin)
        , m_end(begin + length)
    {}

    SpawnableEntityContainerView::SpawnableEntityContainerView(AZ::Entity** begin, AZ::Entity** end)
        : m_begin(begin)
        , m_end(end)
    {
        AZ_Assert(m_begin <= m_end, "SpawnableEntityContainerView created with a begin that's past the end.");
    }

    AZ::Entity** SpawnableEntityContainerView::begin()
    {
        return m_begin;
    }

    AZ::Entity** SpawnableEntityContainerView::end()
    {
        return m_end;
    }

    const AZ::Entity* const* SpawnableEntityContainerView::begin() const
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableEntityContainerView::end() const
    {
        return m_end;
    }

    const AZ::Entity* const* SpawnableEntityContainerView::cbegin()
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableEntityContainerView::cend()
    {
        return m_end;
    }

    AZ::Entity* SpawnableEntityContainerView::operator[](size_t n)
    {
        AZ_Assert(n < size(), "Index %zu is out of bounds (size: %llu) for Spawnable Entity Container View", n, size());
        return *(m_begin + n);
    }

    const AZ::Entity* SpawnableEntityContainerView::operator[](size_t n) const
    {
        AZ_Assert(n < size(), "Index %zu is out of bounds (size: %llu) for Spawnable Entity Container View", n, size());
        return *(m_begin + n);
    }

    size_t SpawnableEntityContainerView::size() const
    {
        return AZStd::distance(m_begin, m_end);
    }

    bool SpawnableEntityContainerView::empty() const
    {
        return m_begin == m_end;
    }


    //
    // SpawnableConstEntityContainerView
    //

    SpawnableConstEntityContainerView::SpawnableConstEntityContainerView(AZ::Entity** begin, size_t length)
        : m_begin(begin)
        , m_end(begin + length)
    {}

    SpawnableConstEntityContainerView::SpawnableConstEntityContainerView(AZ::Entity** begin, AZ::Entity** end)
        : m_begin(begin)
        , m_end(end)
    {
        AZ_Assert(m_begin <= m_end, "SpawnableConstEntityContainerView created with a begin that's past the end.");
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::begin()
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::end()
    {
        return m_end;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::begin() const
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::end() const
    {
        return m_end;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::cbegin()
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::cend()
    {
        return m_end;
    }

    const AZ::Entity* SpawnableConstEntityContainerView::operator[](size_t n)
    {
        AZ_Assert(n < size(), "Index %zu is out of bounds (size: %llu) for Spawnable Const Entity Container View", n, size());
        return *(m_begin + n);
    }

    const AZ::Entity* SpawnableConstEntityContainerView::operator[](size_t n) const
    {
        AZ_Assert(n < size(), "Index %zu is out of bounds (size: %llu) for Spawnable Entity Container View", n, size());
        return *(m_begin + n);
    }

    size_t SpawnableConstEntityContainerView::size() const
    {
        return AZStd::distance(m_begin, m_end);
    }

    bool SpawnableConstEntityContainerView::empty() const
    {
        return m_begin == m_end;
    }


    //
    // SpawnableIndexEntityPair
    //

    SpawnableIndexEntityPair::SpawnableIndexEntityPair(AZ::Entity** entityIterator, size_t* indexIterator)
        : m_entity(entityIterator)
        , m_index(indexIterator)
    {
    }

    AZ::Entity* SpawnableIndexEntityPair::GetEntity()
    {
        return *m_entity;
    }

    const AZ::Entity* SpawnableIndexEntityPair::GetEntity() const
    {
        return *m_entity;
    }

    size_t SpawnableIndexEntityPair::GetIndex() const
    {
        return *m_index;
    }

    //
    // SpawnableIndexEntityIterator
    //

    SpawnableIndexEntityIterator::SpawnableIndexEntityIterator(AZ::Entity** entityIterator, size_t* indexIterator)
        : m_value(entityIterator, indexIterator)
    {
    }

    SpawnableIndexEntityIterator& SpawnableIndexEntityIterator::operator++()
    {
        ++m_value.m_entity;
        ++m_value.m_index;
        return *this;
    }

    SpawnableIndexEntityIterator SpawnableIndexEntityIterator::operator++(int)
    {
        SpawnableIndexEntityIterator result = *this;
        ++m_value.m_entity;
        ++m_value.m_index;
        return result;
    }

    SpawnableIndexEntityIterator& SpawnableIndexEntityIterator::operator--()
    {
        --m_value.m_entity;
        --m_value.m_index;
        return *this;
    }

    SpawnableIndexEntityIterator SpawnableIndexEntityIterator::operator--(int)
    {
        SpawnableIndexEntityIterator result = *this;
        --m_value.m_entity;
        --m_value.m_index;
        return result;
    }

    bool SpawnableIndexEntityIterator::operator==(const SpawnableIndexEntityIterator& rhs)
    {
        return m_value.m_entity == rhs.m_value.m_entity && m_value.m_index == rhs.m_value.m_index;
    }

    bool SpawnableIndexEntityIterator::operator!=(const SpawnableIndexEntityIterator& rhs)
    {
        return m_value.m_entity != rhs.m_value.m_entity || m_value.m_index != rhs.m_value.m_index;
    }

    SpawnableIndexEntityPair& SpawnableIndexEntityIterator::operator*()
    {
        return m_value;
    }

    const SpawnableIndexEntityPair& SpawnableIndexEntityIterator::operator*() const
    {
        return m_value;
    }

    SpawnableIndexEntityPair* SpawnableIndexEntityIterator::operator->()
    {
        return &m_value;
    }

    const SpawnableIndexEntityPair* SpawnableIndexEntityIterator::operator->() const
    {
        return &m_value;
    }


    //
    // SpawnableConstIndexEntityContainerView
    //

    SpawnableConstIndexEntityContainerView::SpawnableConstIndexEntityContainerView(
        AZ::Entity** beginEntity, size_t* beginIndices, size_t length)
        : m_begin(beginEntity, beginIndices)
        , m_end(beginEntity + length, beginIndices + length)
    {
    }

    const SpawnableIndexEntityIterator& SpawnableConstIndexEntityContainerView::begin()
    {
        return m_begin;
    }

    const SpawnableIndexEntityIterator& SpawnableConstIndexEntityContainerView::end()
    {
        return m_end;
    }

    const SpawnableIndexEntityIterator& SpawnableConstIndexEntityContainerView::cbegin()
    {
        return m_begin;
    }

    const SpawnableIndexEntityIterator& SpawnableConstIndexEntityContainerView::cend()
    {
        return m_end;
    }


    //
    // EntitySpawnTicket
    //

    EntitySpawnTicket::EntitySpawnTicket(EntitySpawnTicket&& rhs)
        : m_payload(rhs.m_payload)
        , m_id(rhs.m_id)
    {
        auto manager = SpawnableEntitiesInterface::Get();
        AZ_Assert(manager, "SpawnableEntitiesInterface has no implementation.");
        rhs.m_payload = nullptr;
        rhs.m_id = 0;
        AZStd::scoped_lock lock(manager->m_entitySpawnTicketMapMutex);
        manager->m_entitySpawnTicketMap.insert_or_assign(rhs.m_id, this);
    }

    EntitySpawnTicket::EntitySpawnTicket(AZ::Data::Asset<Spawnable> spawnable)
    {
        auto manager = SpawnableEntitiesInterface::Get();
        AZ_Assert(manager, "Attempting to create an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
        AZStd::pair<EntitySpawnTicket::Id, void*> result = manager->CreateTicket(AZStd::move(spawnable));
        m_id = result.first;
        m_payload = result.second;
        AZStd::scoped_lock lock(manager->m_entitySpawnTicketMapMutex);
        manager->m_entitySpawnTicketMap.insert_or_assign(m_id, this);
    }

    EntitySpawnTicket::~EntitySpawnTicket()
    {
        if (m_payload)
        {
            auto manager = SpawnableEntitiesInterface::Get();
            AZ_Assert(manager, "Attempting to destroy an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
            manager->DestroyTicket(m_payload);
            m_payload = nullptr;
            AZStd::scoped_lock lock(manager->m_entitySpawnTicketMapMutex);
            manager->m_entitySpawnTicketMap.erase(m_id);
            m_id = 0;
        }
    }

    EntitySpawnTicket& EntitySpawnTicket::operator=(EntitySpawnTicket&& rhs)
    {
        if (this != &rhs)
        {
            auto manager = SpawnableEntitiesInterface::Get();
            AZ_Assert(manager, "Attempting to destroy an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
            if (m_payload)
            {
                manager->DestroyTicket(m_payload);
            }

            Id previousId = m_id;
            m_id = rhs.m_id;
            rhs.m_id = 0;

            m_payload = rhs.m_payload;
            rhs.m_payload = nullptr;

            AZStd::scoped_lock lock(manager->m_entitySpawnTicketMapMutex);
            manager->m_entitySpawnTicketMap.erase(previousId);
            manager->m_entitySpawnTicketMap.insert_or_assign(m_id, this);
        }
        return *this;
    }

    auto EntitySpawnTicket::GetId() const -> Id
    {
        return m_id;
    }

    bool EntitySpawnTicket::IsValid() const
    {
        return m_payload != nullptr;
    }
} // namespace AzFramework
