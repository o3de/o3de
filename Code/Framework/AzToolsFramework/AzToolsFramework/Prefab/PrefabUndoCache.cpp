/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabUndoCache.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabUndoCache::Initialize()
        {
            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "PrefabUndoCache - Could not retrieve instance of InstanceToTemplateInterface");

            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "PrefabUndoCache - Could not retrieve instance of InstanceEntityMapperInterface");

            bool prefabSystemEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(
                prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

            if (prefabSystemEnabled)
            {
                // By default, ToolsApplication will register the regular PreemptiveCache as the handler of this interface.
                // Since the SettingsRegistry isn't active when ToolsApplication is constructed, and Start and StartCommon
                // aren't called during tests, we have to resort to unregistering the Preemptive cache here, and registering
                // the PrefabCache in its place. Both caches check if they're registered before unregistering on destroy.
                auto preemptiveCache = AZ::Interface<UndoSystem::UndoCacheInterface>::Get();
                if (preemptiveCache)
                {
                    AZ::Interface<UndoSystem::UndoCacheInterface>::Unregister(preemptiveCache);
                }
                AZ::Interface<UndoSystem::UndoCacheInterface>::Register(this);
            }
        }

        void PrefabUndoCache::Destroy()
        {
            if (AZ::Interface<UndoSystem::UndoCacheInterface>::Get() == this)
            {
                AZ::Interface<UndoSystem::UndoCacheInterface>::Unregister(this);
            }
        }

        void PrefabUndoCache::Validate(const AZ::EntityId& entityId)
        {
            (void)entityId;

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
            if (entityId == AZ::SystemEntityId)
            {
                return;
            }

            PrefabDom oldData;
            AZ::EntityId oldParentId;
            Retrieve(entityId, oldData, oldParentId);

            UpdateCache(entityId);

            PrefabDom newData;
            AZ::EntityId newParentId;
            Retrieve(entityId, newData, newParentId);

            if (newData != oldData || oldParentId != newParentId)
            {
                // display a useful message
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                if (!entity)
                {
                    AZ_Warning(
                        "Undo", false,
                        "Undo system wasn't informed about the deletion of entity %p - make sure you call DeleteEntity, instead of "
                        "directly deleting it.\n",
                        entityId);
                    return;
                }

                AZ_Warning(
                    "Undo", false,
                    "Undo system has inconsistent data for entity %p (%s)\n  Ensure that SetDirty is "
                    "being called (or WorldEditor::WorldEditorMessages::Bus, AddDirtyEntity...) for modified entities.",
                    entityId, entity->GetName().c_str());
            }

            // Clear out newly generated data and
            // replace with original data to ensure debug mode has the same data as profile/release
            // in the event of the consistency check failing.
            m_entitySavedStates[entityId] = {AZStd::move(oldData), oldParentId};

#endif // ENABLE_UNDOCACHE_CONSISTENCY_CHECKS
        }

        void PrefabUndoCache::UpdateCache(const AZ::EntityId& entityId)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

            if (!entity)
            {
                AZ_Warning(
                    "Undo", false,
                    "PrefabUndoCache was told to update the cache for entity of id %llu, but that entity is not available in "
                    "FindEntity",
                    static_cast<AZ::u64>(entityId)
                );
                return;
            }

            InstanceOptionalReference instanceOptionalReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

            if (!instanceOptionalReference.has_value())
            {
                // This is not an error, we just don't handle this entity.
                return;
            }

            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

            // Capture it
            PrefabDom entityDom;
            m_instanceToTemplateInterface->GenerateDomForEntity(entityDom, *entity);
            m_entitySavedStates[entityId] = {AZStd::move(entityDom), parentId};

            AZLOG("Prefab Undo", "Correctly updated cache for entity of id %llu (%s)", static_cast<AZ::u64>(entityId), entity->GetName().c_str());

            return;
        }

        void PrefabUndoCache::PurgeCache(const AZ::EntityId& entityId)
        {
            m_entitySavedStates.erase(entityId);
        }

        bool PrefabUndoCache::Retrieve(const AZ::EntityId& entityId, PrefabDom& outDom, AZ::EntityId& parentId)
        {
            auto it = m_entitySavedStates.find(entityId);

            if (it == m_entitySavedStates.end())
            {
                return false;
            }

            outDom = AZStd::move(m_entitySavedStates[entityId].dom);
            parentId = m_entitySavedStates[entityId].parentId;
            m_entitySavedStates.erase(entityId);
            return true;
        }

        void PrefabUndoCache::Store(const AZ::EntityId& entityId, PrefabDom&& dom, const AZ::EntityId& parentId)
        {
            m_entitySavedStates[entityId] = {AZStd::move(dom), parentId};
        }

        void PrefabUndoCache::Clear()
        {
            m_entitySavedStates.clear();
        }

    } // namespace Prefab
} // namespace AzToolsFramework
