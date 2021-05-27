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

    const AZ::Entity* const* SpawnableEntityContainerView::cbegin()
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableEntityContainerView::cend()
    {
        return m_end;
    }

    size_t SpawnableEntityContainerView::size()
    {
        return AZStd::distance(m_begin, m_end);
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

    const AZ::Entity* const* SpawnableConstEntityContainerView::cbegin()
    {
        return m_begin;
    }

    const AZ::Entity* const* SpawnableConstEntityContainerView::cend()
    {
        return m_end;
    }

    size_t SpawnableConstEntityContainerView::size()
    {
        return AZStd::distance(m_begin, m_end);
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
    {
        rhs.m_payload = nullptr;
    }

    EntitySpawnTicket::EntitySpawnTicket(AZ::Data::Asset<Spawnable> spawnable)
    {
        auto manager = SpawnableEntitiesInterface::Get();
        AZ_Assert(manager, "Attempting to create an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
        m_payload = manager->CreateTicket(AZStd::move(spawnable));
    }

    EntitySpawnTicket::~EntitySpawnTicket()
    {
        if (m_payload)
        {
            auto manager = SpawnableEntitiesInterface::Get();
            AZ_Assert(manager, "Attempting to destroy an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
            manager->DestroyTicket(m_payload);
            m_payload = nullptr;
        }
    }

    EntitySpawnTicket& EntitySpawnTicket::operator=(EntitySpawnTicket&& rhs)
    {
        if (this != &rhs)
        {
            if (m_payload)
            {
                auto manager = SpawnableEntitiesInterface::Get();
                AZ_Assert(manager, "Attempting to destroy an entity spawn ticket while the SpawnableEntitiesInterface has no implementation.");
                manager->DestroyTicket(m_payload);
            }
            m_payload = rhs.m_payload;
            rhs.m_payload = nullptr;
        }
        return *this;
    }

    bool EntitySpawnTicket::IsValid() const
    {
        return m_payload != nullptr;
    }
} // namespace AzFramework
