/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorVisibleEntityDataCache.h"

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityModel.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    //! Cached Entity data required by the selection.
    struct EntityData final
    {
        using ComponentEntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;

        EntityData() = default;
        EntityData(
            AZ::EntityId entityId,
            const AZ::Transform& worldFromLocal,
            bool locked,
            bool visible,
            bool inFocus,
            bool descendantOfClosedContainer,
            bool selected,
            bool iconHidden);

        AZ::Transform m_worldFromLocal;
        AZ::EntityId m_entityId;
        ComponentEntityAccentType m_accent = ComponentEntityAccentType::None;
        bool m_locked = false;
        bool m_visible = true;
        bool m_inFocus = true;
        bool m_descendantOfClosedContainer = false;
        bool m_selected = false;
        bool m_iconHidden = false;
    };

    using EntityDatas = AZStd::vector<EntityData>; //!< Alias for vector of EntityDatas.

    // Predicate to sort EntityIds with EntityDatas interchangeably.
    struct EntityDataComparer
    {
        bool operator()(AZ::EntityId lhs, const EntityData& rhs) const;
        bool operator()(const EntityData& lhs, AZ::EntityId rhs) const;
        bool operator()(AZ::EntityId lhs, AZ::EntityId rhs) const;
        bool operator()(const EntityData& lhs, const EntityData& rhs) const;
    };

    class EditorVisibleEntityDataCache::EditorVisibleEntityDataCacheImpl
    {
    public:
        struct ViewportCache
        {
            EntityIdList m_visibleEntityIds; //!< The EntityIds that are visible this frame.
            EntityIdList m_prevVisibleEntityIds; //!< The EntityIds that were visible the previous frame (unsorted).
            EntityDatas m_visibleEntityDatas; //!< Cached EntityData required by EditorTransformComponentSelection.
        };

        template<typename UpdateEntityDataFn>
        void ForEntityData(const AZ::EntityId entityId, UpdateEntityDataFn&& updateEntityDataFn)
        {
            for (auto& viewportCache : m_viewportCaches)
            {
                const auto entityIdIt = std::equal_range(
                    viewportCache.second.m_visibleEntityDatas.begin(), viewportCache.second.m_visibleEntityDatas.end(), entityId,
                    EntityDataComparer());
                if (entityIdIt.first != entityIdIt.second)
                {
                    updateEntityDataFn(*entityIdIt.first);
                }
            }
        }

        AZStd::unordered_map<AzFramework::ViewportId, ViewportCache> m_viewportCaches;
        ViewportCache* m_current = nullptr;
    };

    // constructor for EntityData to support emplace_back in vector
    EntityData::EntityData(
        const AZ::EntityId entityId,
        const AZ::Transform& worldFromLocal,
        const bool locked,
        const bool visible,
        const bool inFocus,
        const bool descendantOfClosedContainer,
        const bool selected,
        const bool iconHidden)
        : m_worldFromLocal(worldFromLocal)
        , m_entityId(entityId)
        , m_locked(locked)
        , m_visible(visible)
        , m_inFocus(inFocus)
        , m_descendantOfClosedContainer(descendantOfClosedContainer)
        , m_selected(selected)
        , m_iconHidden(iconHidden)
    {
    }

    bool EntityDataComparer::operator()(const AZ::EntityId lhs, const EntityData& rhs) const
    {
        return lhs < rhs.m_entityId;
    }

    bool EntityDataComparer::operator()(const EntityData& lhs, const AZ::EntityId rhs) const
    {
        return lhs.m_entityId < rhs;
    }

    bool EntityDataComparer::operator()(const AZ::EntityId lhs, const AZ::EntityId rhs) const
    {
        return lhs < rhs;
    }

    bool EntityDataComparer::operator()(const EntityData& lhs, const EntityData& rhs) const
    {
        return lhs.m_entityId < rhs.m_entityId;
    }

    bool operator<(const EntityData& lhs, const EntityData& rhs)
    {
        return lhs.m_entityId < rhs.m_entityId;
    }

    static bool EntityIdListsEqual(const EntityIdList& lhs, const EntityIdList& rhs)
    {
        return lhs.size() == rhs.size() && AZStd::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    static EntityData EntityDataFromEntityId(const AZ::EntityId entityId)
    {
        bool visible = false;
        bool locked = false;
        bool iconHidden = false;

        // Check to see if the given entity is an Editor entity or a Runtime entity.
        bool isEditorEntity = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            isEditorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, entityId);

        if (isEditorEntity)
        {
            EditorEntityInfoRequestBus::EventResult(visible, entityId, &EditorEntityInfoRequestBus::Events::IsVisible);
            EditorEntityInfoRequestBus::EventResult(locked, entityId, &EditorEntityInfoRequestBus::Events::IsLocked);
            EditorEntityIconComponentRequestBus::EventResult(
                iconHidden, entityId, &EditorEntityIconComponentRequests::IsEntityIconHiddenInViewport);
        }
        else
        {
            // If this is a runtime entity, default visible / locked / iconHidden to true. This will cause the entity to be displayed
            // but not selectable assuming it is listening to the debug display bus.

            visible = true;
            locked = true;
            iconHidden = true;
        }

        bool inFocus = false;
        if (auto focusModeInterface = AZ::Interface<FocusModeInterface>::Get())
        {
            inFocus = focusModeInterface->IsInFocusSubTree(entityId);
        }

        bool descendantOfClosedContainer = false;
        if (ContainerEntityInterface* containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get())
        {
            descendantOfClosedContainer = containerEntityInterface->IsUnderClosedContainerEntity(entityId);
        }

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        return { entityId, worldFromLocal, locked, visible, inFocus, descendantOfClosedContainer, IsSelected(entityId), iconHidden };
    }

    EditorVisibleEntityDataCache::EditorVisibleEntityDataCache()
        : m_impl(AZStd::make_unique<EditorVisibleEntityDataCacheImpl>())
    {
        m_impl->m_current = &m_impl->m_viewportCaches[AzFramework::InvalidViewportId];

        EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
        AZ::TransformNotificationBus::Router::BusRouterConnect();
        EditorComponentSelectionNotificationsBus::Router::BusRouterConnect();
        EntitySelectionEvents::Bus::Router::BusRouterConnect();
        EditorEntityIconComponentNotificationBus::Router::BusRouterConnect();
        ToolsApplicationNotificationBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzToolsFramework::GetEntityContextId();

        ContainerEntityNotificationBus::Handler::BusConnect(editorEntityContextId);
        FocusModeNotificationBus::Handler::BusConnect(editorEntityContextId);
    }

    EditorVisibleEntityDataCache::~EditorVisibleEntityDataCache()
    {
        FocusModeNotificationBus::Handler::BusDisconnect();
        ContainerEntityNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorEntityIconComponentNotificationBus::Router::BusRouterDisconnect();
        EntitySelectionEvents::Bus::Router::BusRouterDisconnect();
        EditorComponentSelectionNotificationsBus::Router::BusRouterDisconnect();
        AZ::TransformNotificationBus::Router::BusRouterDisconnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
    }

    EditorVisibleEntityDataCache::EditorVisibleEntityDataCache(EditorVisibleEntityDataCache&&) = default;
    EditorVisibleEntityDataCache& EditorVisibleEntityDataCache::operator=(EditorVisibleEntityDataCache&&) = default;

    void EditorVisibleEntityDataCache::AddEntityIds(const EntityIdList& entityIds)
    {
        m_impl->m_current->m_visibleEntityIds.insert(m_impl->m_current->m_visibleEntityIds.end(), entityIds.begin(), entityIds.end());

        m_impl->m_current->m_visibleEntityDatas.reserve(m_impl->m_current->m_visibleEntityDatas.size() + entityIds.size());
        for (AZ::EntityId entityId : m_impl->m_current->m_visibleEntityIds)
        {
            m_impl->m_current->m_visibleEntityDatas.push_back(EntityDataFromEntityId(entityId));
        }

        AZStd::sort(m_impl->m_current->m_visibleEntityDatas.begin(), m_impl->m_current->m_visibleEntityDatas.end());
    }

    void EditorVisibleEntityDataCache::CalculateVisibleEntityDatas(const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorVisibleEntityDataCacheImpl::ViewportCache& cache = m_impl->m_viewportCaches[viewportInfo.m_viewportId];
        m_impl->m_current = &cache;

        // request list of visible entities from authoritative system
        EntityIdList nextVisibleEntityIds;
        ViewportInteraction::EditorEntityViewportInteractionRequestBus::Event(
            viewportInfo.m_viewportId, &ViewportInteraction::EditorEntityViewportInteractionRequestBus::Events::FindVisibleEntities,
            nextVisibleEntityIds);

        // only bother resorting if we know the lists have changed
        if (!EntityIdListsEqual(cache.m_prevVisibleEntityIds, nextVisibleEntityIds))
        {
            // make a copy of the list, this will be sorted in-place
            cache.m_visibleEntityIds = nextVisibleEntityIds;
            // move/steal the nextVisibleEntityIds we requested for faster equality check next frame
            cache.m_prevVisibleEntityIds = AZStd::move(nextVisibleEntityIds);

            // sort copied incoming m_visibleEntityIds - expensive but does not happen often
            AZStd::sort(cache.m_visibleEntityIds.begin(), cache.m_visibleEntityIds.end());

            // find entities that are visible this frame but weren't last frame
            AZStd::vector<AZ::EntityId> added;
            std::set_difference(
                cache.m_visibleEntityIds.begin(), cache.m_visibleEntityIds.end(), cache.m_visibleEntityDatas.begin(),
                cache.m_visibleEntityDatas.end(), std::back_inserter(added), EntityDataComparer());

            // find entities that are not visible this frame but were last frame
            AZStd::vector<EntityData> removed;
            std::set_difference(
                cache.m_visibleEntityDatas.begin(), cache.m_visibleEntityDatas.end(), cache.m_visibleEntityIds.begin(),
                cache.m_visibleEntityIds.end(), std::back_inserter(removed), EntityDataComparer());

            // search for entityData in removed list, return true if it is found
            const auto removePredicate = [&removed](const EntityData& entityData)
            {
                const auto removeIt = std::equal_range(removed.begin(), removed.end(), entityData);

                return removeIt.first != removeIt.second;
            };

            // erase-remove idiom - bubble entities to be removed to the end, then erase them in one go
            cache.m_visibleEntityDatas.erase(
                AZStd::remove_if(cache.m_visibleEntityDatas.begin(), cache.m_visibleEntityDatas.end(), removePredicate),
                cache.m_visibleEntityDatas.end());

            // for newly added entities, request their initial state when first cached
            // and add them to our tracked entity data
            for (AZ::EntityId entityId : added)
            {
                cache.m_visibleEntityDatas.push_back(EntityDataFromEntityId(entityId));
            }

            // after inserting added elements, ensure we keep the visible entity data in sorted order
            AZStd::sort(cache.m_visibleEntityDatas.begin(), cache.m_visibleEntityDatas.end());
        }
    }

    size_t EditorVisibleEntityDataCache::VisibleEntityDataCount() const
    {
        return m_impl->m_current->m_visibleEntityDatas.size();
    }

    AZ::Vector3 EditorVisibleEntityDataCache::GetVisibleEntityPosition(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_worldFromLocal.GetTranslation();
    }

    const AZ::Transform& EditorVisibleEntityDataCache::GetVisibleEntityTransform(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_worldFromLocal;
    }

    AZ::EntityId EditorVisibleEntityDataCache::GetVisibleEntityId(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_entityId;
    }

    EditorVisibleEntityDataCache::ComponentEntityAccentType EditorVisibleEntityDataCache::GetVisibleEntityAccent(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_accent;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntityLocked(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_locked;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntityVisible(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_visible;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntitySelected(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_selected;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntityIconHidden(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_iconHidden;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntityIndividuallySelectableInViewport(const size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_visible && !m_impl->m_current->m_visibleEntityDatas[index].m_locked &&
            m_impl->m_current->m_visibleEntityDatas[index].m_inFocus && !m_impl->m_current->m_visibleEntityDatas[index].m_descendantOfClosedContainer;
    }

    bool EditorVisibleEntityDataCache::IsVisibleEntityInFocusSubTree(size_t index) const
    {
        return m_impl->m_current->m_visibleEntityDatas[index].m_inFocus;
    }

    AZStd::optional<size_t> EditorVisibleEntityDataCache::GetVisibleEntityIndexFromId(const AZ::EntityId entityId) const
    {
        const auto entityIdIt =
            std::equal_range(m_impl->m_current->m_visibleEntityDatas.begin(), m_impl->m_current->m_visibleEntityDatas.end(), entityId, EntityDataComparer());

        if (entityIdIt.first != entityIdIt.second)
        {
            return AZStd::optional<size_t>(static_cast<size_t>(entityIdIt.first - m_impl->m_current->m_visibleEntityDatas.begin()));
        }

        return {};
    }

    void EditorVisibleEntityDataCache::AfterUndoRedo()
    {
        // ensure we refresh all EntityData after an undo/redo action as
        // the notification buses will not be called
        for (auto& viewportCache : m_impl->m_viewportCaches)
        {
            for (EntityData& entityData : viewportCache.second.m_visibleEntityDatas)
            {
                entityData = EntityDataFromEntityId(entityData.m_entityId);
            }
        }
    }

    void EditorVisibleEntityDataCache::OnEntityVisibilityChanged(const bool visibility)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_impl->ForEntityData(
            *EditorEntityVisibilityNotificationBus::GetCurrentBusId(),
            [visibility](EntityData& entityData)
            {
                entityData.m_visible = visibility;
            });
    }

    void EditorVisibleEntityDataCache::OnEntityLockChanged(const bool locked)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_impl->ForEntityData(
            *EditorEntityLockComponentNotificationBus::GetCurrentBusId(),
            [locked](EntityData& entityData)
            {
                entityData.m_locked = locked;
            });
    }

    void EditorVisibleEntityDataCache::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_impl->ForEntityData(
            *AZ::TransformNotificationBus::GetCurrentBusId(),
            [&world](EntityData& entityData)
            {
                entityData.m_worldFromLocal = world;
            });
    }

    void EditorVisibleEntityDataCache::OnAccentTypeChanged(const EntityAccentType accent)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_impl->ForEntityData(
            *EditorComponentSelectionNotificationsBus::GetCurrentBusId(),
            [accent](EntityData& entityData)
            {
                entityData.m_accent = accent;
            });
    }

    void EditorVisibleEntityDataCache::OnSelected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_impl->ForEntityData(
            *EntitySelectionEvents::Bus::GetCurrentBusId(),
            [](EntityData& entityData)
            {
                entityData.m_selected = true;
            });
    }

    void EditorVisibleEntityDataCache::OnDeselected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_impl->ForEntityData(
            *EntitySelectionEvents::Bus::GetCurrentBusId(),
            [](EntityData& entityData)
            {
                entityData.m_selected = false;
            });
    }

    void EditorVisibleEntityDataCache::OnEntityIconChanged(const AZ::Data::AssetId& /*entityIconAssetId*/)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const AZ::EntityId entityId = *EditorEntityIconComponentNotificationBus::GetCurrentBusId();

        bool iconHidden = false;
        EditorEntityIconComponentRequestBus::EventResult(
            iconHidden, entityId, &EditorEntityIconComponentRequests::IsEntityIconHiddenInViewport);

        m_impl->ForEntityData(
            entityId,
            [iconHidden](EntityData& entityData)
            {
                entityData.m_iconHidden = iconHidden;
            });
    }

    void EditorVisibleEntityDataCache::OnContainerEntityStatusChanged(AZ::EntityId entityId, [[maybe_unused]] bool open)
    {
        // Get container descendants
        AzToolsFramework::EntityIdList descendantIds;
        AZ::TransformBus::EventResult(descendantIds, entityId, &AZ::TransformBus::Events::GetAllDescendants);

        // Update cached values
        if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get())
        {
            for (AZ::EntityId descendantId : descendantIds)
            {
                m_impl->ForEntityData(
                    descendantId,
                    [containerEntityInterface, descendantId](EntityData& entityData)
                    {
                        entityData.m_descendantOfClosedContainer = containerEntityInterface->IsUnderClosedContainerEntity(descendantId);
                    });
            }
        }
    }

    void EditorVisibleEntityDataCache::OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId)
    {
        if (previousFocusEntityId.IsValid() && newFocusEntityId.IsValid())
        {
            // Get previous focus root descendants
            AzToolsFramework::EntityIdList previousDescendantIds;
            AZ::TransformBus::EventResult(previousDescendantIds, previousFocusEntityId, &AZ::TransformBus::Events::GetAllDescendants);

            // Get new focus root descendants
            AzToolsFramework::EntityIdList newDescendantIds;
            AZ::TransformBus::EventResult(newDescendantIds, newFocusEntityId, &AZ::TransformBus::Events::GetAllDescendants);

            // Merge EntityId Lists to avoid refreshing values twice
            AzToolsFramework::EntityIdSet descendantsSet;
            descendantsSet.insert(previousFocusEntityId);
            descendantsSet.insert(newFocusEntityId);
            descendantsSet.insert(previousDescendantIds.begin(), previousDescendantIds.end());
            descendantsSet.insert(newDescendantIds.begin(), newDescendantIds.end());

            // Update cached values
            if (auto focusModeInterface = AZ::Interface<FocusModeInterface>::Get())
            {
                for (const AZ::EntityId& descendantId : descendantsSet)
                {
                    m_impl->ForEntityData(
                        descendantId,
                        [focusModeInterface, &descendantId](EntityData& entityData)
                        {
                            entityData.m_inFocus = focusModeInterface->IsInFocusSubTree(descendantId);
                        });
                }
            }
        }
        else
        {
            // If either focus was the invalid entity, refresh all entities.
            if (auto focusModeInterface = AZ::Interface<FocusModeInterface>::Get())
            {
                for (auto& viewportCache : m_impl->m_viewportCaches)
                {
                    for (EntityData& entityData : viewportCache.second.m_visibleEntityDatas)
                    {
                        if (entityData.m_entityId.IsValid())
                        {
                            entityData.m_inFocus = focusModeInterface->IsInFocusSubTree(entityData.m_entityId);
                        }
                    }
                }
            }
        }
    }

} // namespace AzToolsFramework
