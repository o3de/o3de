/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include "PreemptiveUndoCache.h"

namespace AzToolsFramework
{
    namespace
    {
        // configuration options here.
        const AZ::DataStream::StreamType s_streamType = AZ::DataStream::ST_BINARY;

        struct PreemptiveUndoCacheInstance
        {
            PreemptiveUndoCacheInstance()
                : m_instance(nullptr) {}
            AzToolsFramework::PreemptiveUndoCache* m_instance;
        };
        AZ::EnvironmentVariable<PreemptiveUndoCacheInstance> s_cache;
    }

    static const char* s_environmentVarName = "PreemptiveUndoCache";

    PreemptiveUndoCache::PreemptiveUndoCache()
    {
        if (!AZ::Environment::FindVariable<PreemptiveUndoCacheInstance>(s_environmentVarName))
        {
            s_cache = AZ::Environment::CreateVariable<PreemptiveUndoCacheInstance>(s_environmentVarName);
        }

        AZ_Assert(s_cache, "Failed to create environment variable for PreemptiveUndoCache singleton.");
        s_cache->m_instance = this;
    }

    void PreemptiveUndoCache::RegisterToUndoCacheInterface()
    {
        AZ::Interface<UndoSystem::UndoCacheInterface>::Register(this);
    }

    PreemptiveUndoCache::~PreemptiveUndoCache()
    {
        if (s_cache)
        {
            s_cache->m_instance = nullptr;
            s_cache.Reset();
        }

        if (AZ::Interface<UndoSystem::UndoCacheInterface>::Get() == this)
        {
            AZ::Interface<UndoSystem::UndoCacheInterface>::Unregister(this);
        }
    }

    PreemptiveUndoCache* PreemptiveUndoCache::Get()
    {
        auto var = AZ::Environment::FindVariable<PreemptiveUndoCacheInstance>(s_environmentVarName);
        AZ_Assert(var, "Attempt to get PreemptiveUndoCache when an instance has not been created.");
        return var->m_instance;
    }

    void PreemptiveUndoCache::Clear()
    {
        m_EntityStateMap.clear();
    }

    void PreemptiveUndoCache::PurgeCache(const AZ::EntityId& entityId)
    {
        m_EntityStateMap.erase(entityId);
    }

    void PreemptiveUndoCache::UpdateCache(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // capture it

        AZ::Entity* pEnt = nullptr;
        EBUS_EVENT_RESULT(pEnt, AZ::ComponentApplicationBus, FindEntity, entityId);
        if (!pEnt)
        {
            AZ_Warning("Undo", false, "Preemptive Undo Cache was told to update the cache for a particular entity, but that entity is not available in FindEntity");
            return;
        }

        // entity found, snap it!
        CacheLineType& newData = m_EntityStateMap[entityId];
        newData.clear();
        AZ::IO::ByteContainerStream<CacheLineType> ms(&newData);

        AZ::SerializeContext* sc = nullptr;
        EBUS_EVENT_RESULT(sc, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(sc, "Serialization context not found!");

        // capture state.
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&ms, *sc, s_streamType);
        if (!objStream->WriteClass(pEnt))
        {
            AZ_Assert(false, "Unable to serialize entity for undo/redo. ObjectStream::WriteClass() returned an error.");
            return;
        }

        if (!objStream->Finalize())
        {
            AZ_Assert(false, "Unable to serialize entity for undo/redo. ObjectStream::Finalize() returned an error.");
            return;
        }
    }

    void PreemptiveUndoCache::Validate(const AZ::EntityId& entityId)
    {
        (void)entityId;

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
        if (entityId == AZ::SystemEntityId)
        {
            return;
        }

        CacheLineType oldData = Retrieve(entityId);

        if (oldData.empty())
        {
            // this is ok.  It means the entity is not being tracked in the cache.
            // for example, its not an actual undoable entity.
            return;
        }

        UpdateCache(entityId);
        const CacheLineType& newData = Retrieve(entityId);
        if (newData != oldData)
        {
            // display a useful message
            AZ::Entity* pEnt = nullptr;
            EBUS_EVENT_RESULT(pEnt, AZ::ComponentApplicationBus, FindEntity, entityId);
            if (!pEnt)
            {
                AZ_Warning("Undo", false, "Undo system wasn't informed about the deletion of entity %p - make sure you call DeleteEntity, instead of directly deleting it.\n", entityId);
                return;
            }

            AZ_Warning("Undo", false, "Undo system has inconsistent data for entity %p (%s) - Old Size: %llu - New Size: %llu\n  Ensure that SetDirty is being called (or WorldEditor::WorldEditorMessages::Bus, AddDirtyEntity...) for modified entities.",
                entityId, pEnt->GetName().c_str(),
                oldData.size(), newData.size());
        }

        // Clear out newly generated data and
        // replace with original data to ensure debug mode has the same data as profile/release in the event of the consistency check failing.
        m_EntityStateMap[entityId].clear();
        m_EntityStateMap[entityId] = oldData;
#endif // ENABLE_UNDOCACHE_CONSISTENCY_CHECKS
    }

    const PreemptiveUndoCache::CacheLineType& PreemptiveUndoCache::Retrieve(const AZ::EntityId& entityId)
    {
        auto it = m_EntityStateMap.find(entityId);

        if (it == m_EntityStateMap.end())
        {
            return m_Empty;
        }

        return it->second;
    }

} // namespace AzToolsFramework
