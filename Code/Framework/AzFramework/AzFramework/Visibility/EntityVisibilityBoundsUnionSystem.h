/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>

namespace AzFramework
{
    //! Provide a unified hook between entities and the visibility system.
    class EntityVisibilityBoundsUnionSystem
        : public IEntityBoundsUnionRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        EntityVisibilityBoundsUnionSystem();

        void Connect();
        void Disconnect();

        // EntityBoundsUnionRequestBus overrides ...
        void RefreshEntityLocalBoundsUnion(AZ::EntityId entityId) override;
        AZ::Aabb GetEntityLocalBoundsUnion(AZ::EntityId entityId) const override;
        AZ::Aabb GetEntityWorldBoundsUnion(AZ::EntityId entityId) const override;
        void ProcessEntityBoundsUnionRequests() override;
        void OnTransformUpdated(AZ::Entity* entity) override;

    private:
        struct EntityVisibilityBoundsUnionInstance
        {
            AZ::Aabb m_localEntityBoundsUnion = AZ::Aabb::CreateNull(); //!< Entity union bounding volume in local space.
            VisibilityEntry m_visibilityEntry; //!< Hook into the IVisibilitySystem interface.
        };

        using UniqueEntities = AZStd::set<AZ::Entity*>;
        using EntityVisibilityBoundsUnionInstanceMapping =
            AZStd::unordered_map<AZ::Entity*, EntityVisibilityBoundsUnionInstance>;

        void OnEntityActivated(AZ::Entity* entity);
        void OnEntityDeactivated(AZ::Entity* entity);

        // TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void UpdateVisibilitySystem(AZ::Entity* entity, EntityVisibilityBoundsUnionInstance& instance);

        EntityVisibilityBoundsUnionInstanceMapping m_entityVisibilityBoundsUnionInstanceMapping;
        UniqueEntities m_entityBoundsDirty;

        AZ::EntityActivatedEvent::Handler m_entityActivatedEventHandler;
        AZ::EntityDeactivatedEvent::Handler m_entityDeactivatedEventHandler;
    };
} // namespace AzFramework
