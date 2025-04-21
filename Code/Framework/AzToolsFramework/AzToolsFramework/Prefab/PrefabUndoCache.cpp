/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabUndoCache.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Interface/Interface.h>

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

            AZ::Interface<UndoSystem::UndoCacheInterface>::Register(this);
        }

        void PrefabUndoCache::Destroy()
        {
            AZ::Interface<UndoSystem::UndoCacheInterface>::Unregister(this);
        }

        void PrefabUndoCache::Validate(const AZ::EntityId& entityId)
        {
            (void)entityId;

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
            if (entityId == AZ::SystemEntityId)
            {
                return;
            }

            AZ::EntityId oldParentId;
            Retrieve(entityId, oldParentId);

            UpdateCache(entityId);

            AZ::EntityId newParentId;
            Retrieve(entityId, newParentId);

            if (oldParentId != newParentId)
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
            m_parentEntityCache[entityId] = oldParentId;

#endif // ENABLE_UNDOCACHE_CONSISTENCY_CHECKS
        }

        void PrefabUndoCache::UpdateCache(const AZ::EntityId& entityId)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

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
            m_parentEntityCache[entityId] = parentId;

            AZLOG("Prefab Undo", "Correctly cached parent id for entity id %llu (%s)", static_cast<AZ::u64>(entityId), entity->GetName().c_str());

            return;
        }

        void PrefabUndoCache::PurgeCache(const AZ::EntityId& entityId)
        {
            m_parentEntityCache.erase(entityId);
        }

        bool PrefabUndoCache::Retrieve(const AZ::EntityId& entityId, AZ::EntityId& parentId)
        {
            auto it = m_parentEntityCache.find(entityId);

            if (it == m_parentEntityCache.end())
            {
                return false;
            }

            parentId = m_parentEntityCache[entityId];
            m_parentEntityCache.erase(entityId);
            return true;
        }

        void PrefabUndoCache::Store(const AZ::EntityId& entityId, const AZ::EntityId& parentId)
        {
            m_parentEntityCache[entityId] = parentId;
        }

        void PrefabUndoCache::Clear()
        {
            m_parentEntityCache.clear();
        }

    } // namespace Prefab
} // namespace AzToolsFramework
