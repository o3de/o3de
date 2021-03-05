/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        : public EntityBoundsUnionRequestBus::Handler
        , private AZ::EntitySystemBus::Handler
        , private AZ::TransformNotificationBus::Router
        , private AZ::TickBus::Handler
    {
    public:
        void Connect();
        void Disconnect();

        // EntityBoundsUnionRequestBus overrides ...
        void RefreshEntityLocalBoundsUnion(AZ::EntityId entityId) override;
        AZ::Aabb GetEntityLocalBoundsUnion(AZ::EntityId entityId) const override;
        void ProcessEntityBoundsUnionRequests() override;

    private:
        struct EntityVisibilityBoundsUnionInstance
        {
            AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity(); //!< The world transform of the Entity.
            AZ::Aabb m_localEntityBoundsUnion =
                AZ::Aabb::CreateNull(); //!< Entity union bounding volume in local space.
            VisibilityEntry m_visibilityEntry; //!< Hook into the IVisibilitySystem interface.
        };

        using UniqueEntityIds = AZStd::unordered_set<AZ::EntityId>;
        using EntityVisibilityBoundsUnionInstanceMapping =
            AZStd::unordered_map<AZ::EntityId, EntityVisibilityBoundsUnionInstance>;

        // TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // EntitySystemBus overrides ...
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void UpdateVisibilitySystem(EntityVisibilityBoundsUnionInstance& instance);

        EntityVisibilityBoundsUnionInstanceMapping m_entityVisibilityBoundsUnionInstanceMapping;
        UniqueEntityIds m_entityIdsBoundsDirty;
        UniqueEntityIds m_entityIdsTransformDirty;
    };
} // namespace AzFramework
