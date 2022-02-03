/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/typetraits/typetraits.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    //
    // EntityAlias
    //

    bool Spawnable::EntityAlias::HasLowerIndex(const EntityAlias& other) const
    {
        return m_sourceIndex == other.m_sourceIndex ?
            m_aliasType < other.m_aliasType :
            m_sourceIndex < other.m_sourceIndex;
    }


    //
    // EntityAliasVisitorBase
    //

    bool Spawnable::EntityAliasVisitorBase::IsValid(const EntityAliasList* aliases) const
    {
        return aliases != nullptr;
    }

    bool Spawnable::EntityAliasVisitorBase::HasAliases(const EntityAliasList* aliases) const
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        return !aliases->empty();
    }

    bool Spawnable::EntityAliasVisitorBase::AreAllSpawnablesReady(const EntityAliasList* aliases) const
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        for (const EntityAlias& alias : *aliases)
        {
            if (!alias.m_queueLoad ||
                alias.m_aliasType == Spawnable::EntityAliasType::Original ||
                alias.m_aliasType == Spawnable::EntityAliasType::Disable)
            {
                continue;
            }
            if (!alias.m_spawnable.IsReady() && !alias.m_spawnable.IsError())
            {
                return false;
            }
        }
        return true;
    }

    auto Spawnable::EntityAliasVisitorBase::begin(const EntityAliasList* aliases) const -> EntityAliasList::const_iterator
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        return aliases->cbegin();
    }

    auto Spawnable::EntityAliasVisitorBase::end(const EntityAliasList* aliases) const -> EntityAliasList::const_iterator
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        return aliases->cend();
    }

    auto Spawnable::EntityAliasVisitorBase::cbegin(const EntityAliasList* aliases) const -> EntityAliasList::const_iterator
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        return aliases->cbegin();
    }

    auto Spawnable::EntityAliasVisitorBase::cend(const EntityAliasList* aliases) const -> EntityAliasList::const_iterator
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        return aliases->cend();
    }

    void Spawnable::EntityAliasVisitorBase::ListTargetSpawnables(
        const EntityAliasList* aliases, const ListTargetSpawanblesCallback& callback) const
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        AZStd::unordered_set<AZ::Data::AssetId> spawnableIds;
        for (const Spawnable::EntityAlias& alias : *aliases)
        {
            // If the spawnable id is not valid it means that the alias is referencing the spawnable it's stored on.
            if (alias.m_spawnable.GetId().IsValid())
            {
                auto it = spawnableIds.find(alias.m_spawnable.GetId());
                if (it == spawnableIds.end())
                {
                    callback(alias.m_spawnable);
                    spawnableIds.emplace(alias.m_spawnable.GetId());
                }
            }
        }
    }

    void Spawnable::EntityAliasVisitorBase::ListTargetSpawnables(
        const EntityAliasList* aliases, AZ::Crc32 tag, const ListTargetSpawanblesCallback& callback) const
    {
        AZ_Assert(aliases, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        AZStd::unordered_set<AZ::Data::AssetId> spawnableIds;
        for (const Spawnable::EntityAlias& alias : *aliases)
        {
            // If the spawnable id is not valid it means that the alias is referencing the spawnable it's stored on.
            if (alias.m_tag == tag && alias.m_spawnable.GetId().IsValid())
            {
                auto it = spawnableIds.find(alias.m_spawnable.GetId());
                if (it == spawnableIds.end())
                {
                    callback(alias.m_spawnable);
                    spawnableIds.emplace(alias.m_spawnable.GetId());
                }
            }
        }
    }


    //
    // EntityAliasVisitor
    //

    Spawnable::EntityAliasVisitor::EntityAliasVisitor(Spawnable& owner, EntityAliasList* entityAliasList)
        : m_owner(owner)
        , m_entityAliasList(entityAliasList)
    {
    }

    Spawnable::EntityAliasVisitor::~EntityAliasVisitor()
    {
        if (IsValid())
        {
            Optimize();

            AZ_Assert(
                m_owner.m_shareState == ShareState::ReadWrite, "Attempting to unlock a spawnable that's not in the locked state (%i).",
                m_owner.m_shareState.load());
            m_owner.m_shareState = ShareState::NotShared;
        }
    }

    Spawnable::EntityAliasVisitor::EntityAliasVisitor(EntityAliasVisitor&& rhs)
        : m_owner(rhs.m_owner)
        , m_entityAliasList(rhs.m_entityAliasList)
    {
        m_dirty = rhs.m_dirty;

        rhs.m_entityAliasList = nullptr;
        rhs.m_dirty = false;
    }

    auto Spawnable::EntityAliasVisitor::operator=(EntityAliasVisitor&& rhs) -> EntityAliasVisitor&
    {
        if (this != &rhs)
        {
            this->~EntityAliasVisitor();
            new(this) EntityAliasVisitor(AZStd::move(rhs));
        }
        return *this;
    }

    bool Spawnable::EntityAliasVisitor::IsValid() const
    {
        return EntityAliasVisitorBase::IsValid(m_entityAliasList);
    }

    bool Spawnable::EntityAliasVisitor::HasAliases() const
    {
        return EntityAliasVisitorBase::HasAliases(m_entityAliasList);
    }

    bool Spawnable::EntityAliasVisitor::AreAllSpawnablesReady() const
    {
        return EntityAliasVisitorBase::AreAllSpawnablesReady(m_entityAliasList);
    }

    auto Spawnable::EntityAliasVisitor::begin() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::begin(m_entityAliasList);
    }

    auto Spawnable::EntityAliasVisitor::end() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::end(m_entityAliasList);
    }

    auto Spawnable::EntityAliasVisitor::cbegin() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::cbegin(m_entityAliasList);
    }

    auto Spawnable::EntityAliasVisitor::cend() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::cend(m_entityAliasList);
    }

    void Spawnable::EntityAliasVisitor::ListTargetSpawnables(const ListTargetSpawanblesCallback& callback) const
    {
        EntityAliasVisitorBase::ListTargetSpawnables(m_entityAliasList, callback);
    }

    void Spawnable::EntityAliasVisitor::ListTargetSpawnables(AZ::Crc32 tag, const ListTargetSpawanblesCallback& callback) const
    {
        EntityAliasVisitorBase::ListTargetSpawnables(m_entityAliasList, tag, callback);
    }

    void Spawnable::EntityAliasVisitor::AddAlias(
        AZ::Data::Asset<Spawnable> targetSpawnable,
        AZ::Crc32 tag,
        uint32_t sourceIndex,
        uint32_t targetIndex,
        Spawnable::EntityAliasType aliasType,
        bool queueLoad)
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        AZ_Assert(sourceIndex < m_owner.GetEntities().size(), "Invalid source index (%i) for entity alias", sourceIndex);
        if (targetSpawnable.IsReady())
        {
            AZ_Assert(
                targetIndex < targetSpawnable->GetEntities().size(), "Invalid target index (%i) for entity alias '%s'", targetIndex,
                targetSpawnable.GetHint().c_str());
        }

        m_entityAliasList->push_back(Spawnable::EntityAlias{ targetSpawnable, tag, sourceIndex, targetIndex, aliasType, queueLoad });
        m_dirty = true;
    }

    void Spawnable::EntityAliasVisitor::ListSpawnablesRequiringLoad(const ListSpawnablesRequiringLoadCallback& callback)
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        for (Spawnable::EntityAlias& alias : *m_entityAliasList)
        {
            if (alias.m_queueLoad &&
                alias.m_aliasType != Spawnable::EntityAliasType::Original &&
                alias.m_aliasType != Spawnable::EntityAliasType::Disable &&
                !alias.m_spawnable.IsLoading() &&
                !alias.m_spawnable.IsReady() &&
                !alias.m_spawnable.IsError())
            {
                callback(alias.m_spawnable);
            }
        }
    }

    void Spawnable::EntityAliasVisitor::UpdateAliases(const UpdateCallback& callback)
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        for (Spawnable::EntityAlias& alias : *m_entityAliasList)
        {
            AZ::Data::Asset<Spawnable> targetSpawnable(alias.m_spawnable);
            callback(alias.m_aliasType, alias.m_queueLoad, targetSpawnable, alias.m_tag, alias.m_sourceIndex, alias.m_targetIndex);
        }
        m_dirty = true;
    }

    void Spawnable::EntityAliasVisitor::UpdateAliases(AZ::Crc32 tag, const UpdateCallback& callback)
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        for (Spawnable::EntityAlias& alias : *m_entityAliasList)
        {
            if (alias.m_tag == tag)
            {
                AZ::Data::Asset<Spawnable> targetSpawnable(alias.m_spawnable);
                callback(alias.m_aliasType, alias.m_queueLoad, targetSpawnable, alias.m_tag, alias.m_sourceIndex, alias.m_targetIndex);
                m_dirty = true;
            }
        }
    }

    void Spawnable::EntityAliasVisitor::UpdateAliasType(uint32_t index, Spawnable::EntityAliasType newType)
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        AZ_Assert(
            index < m_entityAliasList->size(), "Unable to update entity alias at index %i as there are only %zu aliases in spawnable.",
            index, m_entityAliasList->size());
        (*m_entityAliasList)[index].m_aliasType = newType;
        m_dirty = true;
    }

    void Spawnable::EntityAliasVisitor::Optimize()
    {
        AZ_Assert(m_entityAliasList, "Attempting to visit entity aliases on a spawnable that wasn't locked.");
        if (m_dirty)
        {
            AZStd::stable_sort(
                m_entityAliasList->begin(), m_entityAliasList->end(),
                [](const Spawnable::EntityAlias& lhs, const Spawnable::EntityAlias& rhs)
                {
                    // Sort by source index from smallest to largest so during spawning the entities can be iterated linearly over.
                    // If the source index is the same then sort by alias type so the next steps can optimize away superfluous steps.
                    return lhs.HasLowerIndex(rhs);
                });

            // Remove aliases that are not going to have any practical effect and insert aliases where needed to simplify the spawning.
            // This is done at runtime rather than at build time because the above ebus allows other systems to make adjustments to the
            // aliases, for instance Networking can decide to disable certain aliases when running on a client. This in turn also requires
            // the aliases to be in their recorded order during building as the ebus handlers may depend on that order to determine what
            // entities need to be updated.
            uint32_t previousIndex = AZStd::numeric_limits<uint32_t>::max();
            Spawnable::EntityAliasType previousType =
                static_cast<Spawnable::EntityAliasType>(AZStd::numeric_limits<AZStd::underlying_type_t<Spawnable::EntityAliasType>>::max());
            Spawnable::EntityAlias* it = m_entityAliasList->begin();
            Spawnable::EntityAlias* end = m_entityAliasList->end();
            while (it < end)
            {
                // If there's a switch to a new source index and the previous index only had an original it can
                // be removed.
                if (previousType == Spawnable::EntityAliasType::Original && previousIndex != it->m_sourceIndex)
                {
                    it = m_entityAliasList->erase(it - 1);
                    end = m_entityAliasList->end();
                    if (it == end)
                    {
                        break;
                    }
                }

                switch (it->m_aliasType)
                {
                case Spawnable::EntityAliasType::Original:
                    [[fallthrough]];
                case Spawnable::EntityAliasType::Disable:
                    [[fallthrough]];
                case Spawnable::EntityAliasType::Replace:
                    // If the previous entry was a disabled, original or replace alias then remove it as it will be overwritten by the
                    // current entry.
                    if (previousIndex == it->m_sourceIndex &&
                        (previousType == Spawnable::EntityAliasType::Original ||
                         previousType == Spawnable::EntityAliasType::Disable ||
                         previousType == Spawnable::EntityAliasType::Replace))
                    {
                        previousIndex = it->m_sourceIndex;
                        previousType = it->m_aliasType;
                        // Erase instead of a swap-and-pop in order to preserver the order.
                        it = m_entityAliasList->erase(it - 1) + 1;
                        end = m_entityAliasList->end();
                    }
                    else
                    {
                        previousIndex = it->m_sourceIndex;
                        previousType = it->m_aliasType;
                        ++it;
                    }
                    break;
                case Spawnable::EntityAliasType::Additional:
                    [[fallthrough]];
                case Spawnable::EntityAliasType::Merge:
                    // If this is the first entry for this index then insert an original in front of it so the spawnable entity manager
                    // doesn't have to check for the case there's a merge and/or addition without an entity to extend.
                    if (previousIndex != it->m_sourceIndex)
                    {
                        Spawnable::EntityAlias insert;
                        // No load, as the asset is already loaded.
                        insert.m_spawnable = AZ::Data::Asset<Spawnable>({}, azrtti_typeid<Spawnable>());
                        insert.m_sourceIndex = it->m_sourceIndex;
                        insert.m_targetIndex = it->m_sourceIndex; // Source index as the original entry for this slot is added.
                        insert.m_aliasType = Spawnable::EntityAliasType::Original;

                        previousIndex = it->m_sourceIndex;
                        previousType = it->m_aliasType;

                        // Insert to maintain the order.
                        it = m_entityAliasList->insert(it, AZStd::move(insert));
                        it += 2;
                        end = m_entityAliasList->end();
                    }
                    else
                    {
                        previousType = it->m_aliasType;
                        ++it;
                    }
                    break;
                default:
                    AZ_Assert(false, "Invalid Spawnable entity alias type found during asset loading: %i", it->m_aliasType);
                    break;
                }
            }

            // Check if the last entry is an "Original" in which case it can be removed.
            if (!m_entityAliasList->empty() && m_entityAliasList->back().m_aliasType == Spawnable::EntityAliasType::Original)
            {
                m_entityAliasList->pop_back();
            }

            // Reclaim memory because after this point the aliases will not change anymore.
            m_entityAliasList->shrink_to_fit();
            m_dirty = false;
        }
    }



    //
    // EntityAliasConstVisitor
    //

    Spawnable::EntityAliasConstVisitor::EntityAliasConstVisitor(const Spawnable& owner, const EntityAliasList* entityAliasList)
        : m_owner(owner)
        , m_entityAliasList(entityAliasList)
    {
    }

    Spawnable::EntityAliasConstVisitor::~EntityAliasConstVisitor()
    {
        if (IsValid())
        {
            AZ_Assert(
                m_owner.m_shareState <= ShareState::Read, "Attempting to unlock a read shared spawnable that was not in a read shared mode (%i).",
                m_owner.m_shareState.load());
            m_owner.m_shareState++;
        }
    }

    bool Spawnable::EntityAliasConstVisitor::IsValid() const
    {
        return EntityAliasVisitorBase::IsValid(m_entityAliasList);
    }

    bool Spawnable::EntityAliasConstVisitor::HasAliases() const
    {
        return EntityAliasVisitorBase::HasAliases(m_entityAliasList);
    }

    bool Spawnable::EntityAliasConstVisitor::AreAllSpawnablesReady() const
    {
        return EntityAliasVisitorBase::AreAllSpawnablesReady(m_entityAliasList);
    }

    auto Spawnable::EntityAliasConstVisitor::begin() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::begin(m_entityAliasList);
    }

    auto Spawnable::EntityAliasConstVisitor::end() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::end(m_entityAliasList);
    }

    auto Spawnable::EntityAliasConstVisitor::cbegin() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::cbegin(m_entityAliasList);
    }

    auto Spawnable::EntityAliasConstVisitor::cend() const -> EntityAliasList::const_iterator
    {
        return EntityAliasVisitorBase::cend(m_entityAliasList);
    }

    void Spawnable::EntityAliasConstVisitor::ListTargetSpawnables(const ListTargetSpawanblesCallback& callback) const
    {
        EntityAliasVisitorBase::ListTargetSpawnables(m_entityAliasList, callback);
    }

    void Spawnable::EntityAliasConstVisitor::ListTargetSpawnables(AZ::Crc32 tag, const ListTargetSpawanblesCallback& callback) const
    {
        EntityAliasVisitorBase::ListTargetSpawnables(m_entityAliasList, tag, callback);
    }



    //
    // Spawnable
    //

    Spawnable::Spawnable(const AZ::Data::AssetId& id, AssetStatus status)
        : AZ::Data::AssetData(id, status)
    {
    }

    const Spawnable::EntityList& Spawnable::GetEntities() const
    {
        return m_entities;
    }

    Spawnable::EntityList& Spawnable::GetEntities()
    {
        return m_entities;
    }

    auto Spawnable::TryGetAliasesConst() const -> EntityAliasConstVisitor
    {
        int32_t expected = ShareState::NotShared;
        do
        {
            // Try to set the lock to a negative number to indicate a shared read.
            if (m_shareState.compare_exchange_strong(expected, expected - 1))
            {
                return EntityAliasConstVisitor(*this, &m_entityAliases);
            }
        // as long as the value is negative or not shared then keep trying to get a shared read lock.
        } while (expected <= 0);
        return EntityAliasConstVisitor(*this, nullptr);
    }

    auto Spawnable::TryGetAliases() const -> EntityAliasConstVisitor
    {
        return TryGetAliasesConst();
    }

    auto Spawnable::TryGetAliases() -> EntityAliasVisitor
    {
        int32_t expected = ShareState::NotShared;
        return m_shareState.compare_exchange_strong(expected, ShareState::ReadWrite) ? EntityAliasVisitor(*this, &m_entityAliases)
                                                                                     : EntityAliasVisitor(*this, nullptr);
    }

    bool Spawnable::IsEmpty() const
    {
        return m_entities.empty();
    }

    SpawnableMetaData& Spawnable::GetMetaData()
    {
        return m_metaData;
    }

    const SpawnableMetaData& Spawnable::GetMetaData() const
    {
        return m_metaData;
    }

    void Spawnable::Reflect(AZ::ReflectContext* context)
    {
        EntityAlias::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<Spawnable, AZ::Data::AssetData>()->Version(2)
                ->Field("Meta data", &Spawnable::m_metaData)
                ->Field("Entity aliases", &Spawnable::m_entityAliases)
                ->Field("Entities", &Spawnable::m_entities);
        }
    }

    void Spawnable::EntityAlias::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<Spawnable::EntityAlias>()
                ->Version(1)
                ->Field("Spawnable", &EntityAlias::m_spawnable)
                ->Field("Tag", &EntityAlias::m_tag)
                ->Field("Source Index", &EntityAlias::m_sourceIndex)
                ->Field("Target Index", &EntityAlias::m_targetIndex)
                ->Field("Alias Type", &EntityAlias::m_aliasType)
                ->Field("Queue Load", &EntityAlias::m_queueLoad);
        }
    }
} // namespace AzFramework
