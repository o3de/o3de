/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Visibility/BoundsBus.h>

namespace AtomToolsFramework
{
    EntityPreviewViewportContent::EntityPreviewViewportContent(
        const AZ::Crc32& toolId,
        RenderViewportWidget* widget,
        AZStd::shared_ptr<AzFramework::EntityContext> entityContext)
        : m_toolId(toolId)
        , m_entityContext(entityContext)
    {
        AZ::RPI::ViewportContextPtr viewportContext = widget->GetViewportContext();
        AZ::RPI::WindowContextSharedPtr windowContext = viewportContext->GetWindowContext();

        // Configure camera
        m_cameraEntity =
            CreateEntity("CameraEntity", { azrtti_typeid<AzFramework::TransformComponent>(), azrtti_typeid<AZ::Debug::CameraComponent>() });

        AZ::Debug::CameraComponentConfig cameraConfig(windowContext);
        cameraConfig.m_fovY = AZ::Constants::HalfPi;
        cameraConfig.m_depthNear = 0.01f;
        m_cameraEntity->Deactivate();
        m_cameraEntity->FindComponent(azrtti_typeid<AZ::Debug::CameraComponent>())->SetConfiguration(cameraConfig);
        m_cameraEntity->Activate();

        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportContent::~EntityPreviewViewportContent()
    {
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();

        while (!m_entities.empty())
        {
            DestroyEntity(m_entities.back());
        }
    }

    AZ::Aabb EntityPreviewViewportContent::GetObjectLocalBounds() const
    {
        AZ::Aabb objectBounds = AZ::Aabb::CreateNull();
        AzFramework::BoundsRequestBus::EventResult(
            objectBounds, GetObjectEntityId(), &AzFramework::BoundsRequestBus::Events::GetLocalBounds);
        if (!objectBounds.IsValid() || !objectBounds.IsFinite())
        {
            objectBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        }
        return objectBounds;
    }

    AZ::Aabb EntityPreviewViewportContent::GetObjectWorldBounds() const
    {
        AZ::Aabb objectBounds = AZ::Aabb::CreateNull();
        AzFramework::BoundsRequestBus::EventResult(
            objectBounds, GetObjectEntityId(), &AzFramework::BoundsRequestBus::Events::GetWorldBounds);
        if (!objectBounds.IsValid() || !objectBounds.IsFinite())
        {
            objectBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        }
        return objectBounds;
    }

    AZ::EntityId EntityPreviewViewportContent::GetObjectEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportContent::GetCameraEntityId() const
    {
        return m_cameraEntity ? m_cameraEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportContent::GetEnvironmentEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportContent::GetPostFxEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::Entity* EntityPreviewViewportContent::CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds)
    {
        AZ::Entity* entity = m_entityContext->CreateEntity(name.c_str());
        AZ_Assert(entity != nullptr, "Failed to create post process entity: %s.", name.c_str());

        if (entity)
        {
            for (const auto& componentTypeId : componentTypeIds)
            {
                entity->CreateComponent(componentTypeId);
            }
            entity->Init();
            entity->Activate();
            m_entities.push_back(entity);
        }

        return entity;
    }

    void EntityPreviewViewportContent::DestroyEntity(AZ::Entity*& entity)
    {
        if (entity)
        {
            entity->Deactivate();
            m_entityContext->DestroyEntity(entity);
            m_entities.erase(AZStd::remove(m_entities.begin(), m_entities.end(), entity), m_entities.end());
            entity = nullptr;
        }
    }
} // namespace AtomToolsFramework
