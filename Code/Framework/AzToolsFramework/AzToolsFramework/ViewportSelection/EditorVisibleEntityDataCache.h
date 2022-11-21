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
    //! Read-only interface for EditorVisibleEntityDataCache to be used by systems that want to efficiently
    //! query the state of visible entities in the viewport.
    class EditorVisibleEntityDataCacheInterface
    {
        using ComponentEntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;

    public:
        virtual ~EditorVisibleEntityDataCacheInterface() = default;

        virtual size_t VisibleEntityDataCount() const = 0;
        virtual AZ::Vector3 GetVisibleEntityPosition(size_t index) const = 0;
        virtual const AZ::Transform& GetVisibleEntityTransform(size_t index) const = 0;
        virtual AZ::EntityId GetVisibleEntityId(size_t index) const = 0;
        virtual ComponentEntityAccentType GetVisibleEntityAccent(size_t index) const = 0;
        virtual bool IsVisibleEntityLocked(size_t index) const = 0;
        virtual bool IsVisibleEntityVisible(size_t index) const = 0;
        virtual bool IsVisibleEntitySelected(size_t index) const = 0;
        virtual bool IsVisibleEntityIconHidden(size_t index) const = 0;
        //! Returns true if the entity is individually selectable (none of its ancestors are a closed container entity).
        //! @note It may still be desirable to be able to 'click' an entity that is a descendant of a closed container
        //! to select the container itself, not the individual entity.
        virtual bool IsVisibleEntityIndividuallySelectableInViewport(size_t index) const = 0;
        virtual bool IsVisibleEntityInFocusSubTree(size_t index) const = 0;
        virtual AZStd::optional<size_t> GetVisibleEntityIndexFromId(AZ::EntityId entityId) const = 0;
    };

    //! A cache of packed EntityData that can be iterated over efficiently without
    //! the need to make individual EBus calls
    class EditorVisibleEntityDataCache
        : public EditorVisibleEntityDataCacheInterface
        , private EditorEntityVisibilityNotificationBus::Router
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

        //! EditorVisibleEntityDataCacheInterface overrides ...
        size_t VisibleEntityDataCount() const override;
        AZ::Vector3 GetVisibleEntityPosition(size_t index) const override;
        const AZ::Transform& GetVisibleEntityTransform(size_t index) const override;
        AZ::EntityId GetVisibleEntityId(size_t index) const override;
        ComponentEntityAccentType GetVisibleEntityAccent(size_t index) const override;
        bool IsVisibleEntityLocked(size_t index) const override;
        bool IsVisibleEntityVisible(size_t index) const override;
        bool IsVisibleEntitySelected(size_t index) const override;
        bool IsVisibleEntityIconHidden(size_t index) const override;
        bool IsVisibleEntityIndividuallySelectableInViewport(size_t index) const override;
        bool IsVisibleEntityInFocusSubTree(size_t index) const override;
        AZStd::optional<size_t> GetVisibleEntityIndexFromId(AZ::EntityId entityId) const override;

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
