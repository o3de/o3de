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
        bool IsVisibleEntitySelectableInViewport(size_t index) const;

        AZStd::optional<size_t> GetVisibleEntityIndexFromId(AZ::EntityId entityId) const;

        void AddEntityIds(const EntityIdList& entityIds);

    private:
        // ToolsApplicationNotificationBus
        void AfterUndoRedo() override;

        // EditorEntityVisibilityNotificationBus
        void OnEntityVisibilityChanged(bool visibility) override;

        // EditorEntityLockComponentNotificationBus
        void OnEntityLockChanged(bool locked) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorComponentSelectionNotificationsBus
        void OnAccentTypeChanged(EntityAccentType accent) override;

        // EntitySelectionEvents::Bus
        void OnSelected() override;
        void OnDeselected() override;

        // EditorEntityIconComponentNotificationBus
        void OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId) override;

        class EditorVisibleEntityDataCacheImpl;
        AZStd::unique_ptr<EditorVisibleEntityDataCacheImpl> m_impl; //!< Internal representation of entity data cache.
    };
} // namespace AzToolsFramework
