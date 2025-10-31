/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <RecastNavigation/DetourNavigationBus.h>

namespace RecastNavigation
{
    //! Calculates paths over the associated navigation mesh.
    //! Provides APIs to find a path between two entities or two world positions.
    class DetourNavigationComponent final
        : public AZ::Component
        , public DetourNavigationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(DetourNavigationComponent, "{B9A8F260-2772-4C94-8DE4-850C94A8F2AC}");
        DetourNavigationComponent() = default;
        //! Constructor to be used by Editor variant to pass the configuration in.
        //! @param navQueryEntityId entity id of the entity with a navigation mesh component.
        //! @param nearestDistance distance to use when finding nearest point on the navigation
        //!            mesh when points provided to FindPath are outside of the navigation mesh.
        DetourNavigationComponent(AZ::EntityId navQueryEntityId, float nearestDistance);

        static void Reflect(AZ::ReflectContext* context);

        //! DetourNavigationRequestBus overrides ...
        //! @{
        AZStd::vector<AZ::Vector3> FindPathBetweenEntities(AZ::EntityId fromEntity, AZ::EntityId toEntity) override;
        AZStd::vector<AZ::Vector3> FindPathBetweenPositions(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition) override;
        void SetNavigationMeshEntity(AZ::EntityId navMeshEntity) override;
        AZ::EntityId GetNavigationMeshEntity() const override;
        //! @}

        //! AZ::Component overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

    private:
        //! Entity id of the entity with a navigation mesh component.
        AZ::EntityId m_navQueryEntityId;
        //! Distance to use when finding nearest point on the navigation mesh when points provided to FindPath are outside of the navigation mesh.
        float m_nearestDistance = 3.f;
    };
} // namespace RecastNavigation
