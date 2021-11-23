/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>
#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

namespace AzToolsFramework
{
    //! A cache of packed EntityData that can be iterated over efficiently without
    //! the need to make individual EBus calls
    class EditorVisibleEntityDataCache
        : private EditorEntityVisibilityNotificationBus::Router
        , private EditorEntityLockComponentNotificationBus::Router
        , private AZ::TransformNotificationBus::Router
        , private EditorComponentSelectionNotificationsBus::Router
        , private EntitySelectionEvents::Bus::Router
        , private EditorEntityIconComponentNotificationBus::Router
        , private ToolsApplicationNotificationBus::Handler
        , private ContainerEntityNotificationBus::Handler
        , private FocusModeNotificationBus::Handler
    {
    public:
        EditorVisibleEntityDataCache();
        ~EditorVisibleEntityDataCache();
        EditorVisibleEntityDataCache(const EditorVisibleEntityDataCache&) = delete;
        EditorVisibleEntityDataCache& operator=(const EditorVisibleEntityDataCache&) = delete;
        EditorVisibleEntityDataCache(EditorVisibleEntityDataCache&&);
        EditorVisibleEntityDataCache& operator=(EditorVisibleEntityDataCache&&);

        using ComponentEntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;

        void CalculateVisibleEntityDatas(const AzFramework::ViewportInfo& viewportInfo);

        //! EditorVisibleEntityDataCache interface
        size_t VisibleEntityDataCount() const;
        AZ::Vector3 GetVisibleEntityPosition(size_t index) const;
        const AZ::Transform& GetVisibleEntityTransform(size_t index) const;
        AZ::EntityId GetVisibleEntityId(size_t index) const;
        ComponentEntityAccentType GetVisibleEntityAccent(size_t index) const;
        bool IsVisibleEntityLocked(size_t index) const;
        bool IsVisibleEntityVisible(size_t index) const;
        bool IsVisibleEntitySelected(size_t index) const;
        bool IsVisibleEntityIconHidden(size_t index) const;
        //! Returns true if the entity is individually selectable (none of its ancestors are a closed container entity).
        //! @note It may still be desirable to be able to 'click' an entity that is a descendant of a closed container
        //! to select the container itself, not the individual entity.
        bool IsVisibleEntityIndividuallySelectableInViewport(size_t index) const;

        AZStd::optional<size_t> GetVisibleEntityIndexFromId(AZ::EntityId entityId) const;

        void AddEntityIds(const EntityIdList& entityIds);

    private:
        // ToolsApplicationNotificationBus overrides ...
        void AfterUndoRedo() override;

        // EditorEntityVisibilityNotificationBus overrides ...
        void OnEntityVisibilityChanged(bool visibility) override;

        // EditorEntityLockComponentNotificationBus overrides ...
        void OnEntityLockChanged(bool locked) override;

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorComponentSelectionNotificationsBus overrides ...
        void OnAccentTypeChanged(EntityAccentType accent) override;

        // EntitySelectionEvents::Bus overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        // EditorEntityIconComponentNotificationBus overrides ...
        void OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId) override;

        // ContainerEntityNotificationBus overrides ...
        void OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open) override;

        // FocusModeNotificationBus overrides ...
        void OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId) override;

        class EditorVisibleEntityDataCacheImpl;
        AZStd::unique_ptr<EditorVisibleEntityDataCacheImpl> m_impl; //!< Internal representation of entity data cache.
    };
} // namespace AzToolsFramework
