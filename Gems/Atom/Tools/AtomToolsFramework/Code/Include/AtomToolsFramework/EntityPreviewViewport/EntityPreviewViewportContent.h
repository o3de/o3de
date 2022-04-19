/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#endif

namespace AtomToolsFramework
{
    //! EntityPreviewViewportContent and derived classes are responsible for populating the scene with entities. Some overrideable functions
    //! provide access to certain entities that should be standard in most viewports but can be constructed differently based on the
    //! content.
    class EntityPreviewViewportContent : public EntityPreviewViewportSettingsNotificationBus::Handler
    {
    public:
        EntityPreviewViewportContent(
            const AZ::Crc32& toolId, RenderViewportWidget* widget, AZStd::shared_ptr<AzFramework::EntityContext> entityContext);
        virtual ~EntityPreviewViewportContent();

        //! Local bounds of the primary object displayed in the viewport
        virtual AZ::Aabb GetObjectLocalBounds() const;
        //! World bounds of the primary object displayed in the viewport
        virtual AZ::Aabb GetObjectWorldBounds() const;
        //! Entity ID of the primary object displayed in the viewport
        virtual AZ::EntityId GetObjectEntityId() const;
        //! Entity ID of the camera used control perspective and view in the viewport
        virtual AZ::EntityId GetCameraEntityId() const;
        //! Entity ID of the environment or stage surrounding the primary object
        virtual AZ::EntityId GetEnvironmentEntityId() const;
        //! Entity ID of the entity containing or controlling post processing effect components
        virtual AZ::EntityId GetPostFxEntityId() const;

        //! Create and activate a single entity with the listed components
        AZ::Entity* CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds);
        //! Deactivate and destroy a single entity
        void DestroyEntity(AZ::Entity*& entity);

    protected:
        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override{};

        const AZ::Crc32 m_toolId = {};

    private:
        AZStd::shared_ptr<AzFramework::EntityContext> m_entityContext;
        AZStd::vector<AZ::Entity*> m_entities;
        AZ::Entity* m_cameraEntity = {};
    };
} // namespace AtomToolsFramework
