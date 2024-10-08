/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <ToolsComponents/EditorComponentBase.h>
#if defined(CARBONATED)
#include <RecastNavigation/DetourNavigationBus.h>
#endif

namespace RecastNavigation
{
    //! Editor version of a path finding component, @DetourNavigationComponent.
    class EditorDetourNavigationComponent final
        : public AzToolsFramework::Components::EditorComponentBase
#if defined(CARBONATED)
        , public DetourNavigationRequestBus::Handler
#endif
    {
    public:
        AZ_EDITOR_COMPONENT(EditorDetourNavigationComponent, "{A8D728AB-FC42-42AE-A904-3CF5F1C83D16}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! EditorComponentBase overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

#if defined(CARBONATED)
        //! DetourNavigationBus overrides ...
        //! @{
        void SetNavigationMeshEntity(AZ::EntityId navMeshEntity) override;
        AZ::EntityId GetNavigationMeshEntity() const override;
        AZStd::vector<AZ::Vector3> FindPathBetweenEntities(
            AZ::EntityId fromEntity, AZ::EntityId toEntity, bool addCrossings, bool& partial) override;
        AZStd::vector<AZ::Vector3> FindPathBetweenPositions(
            const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition, bool addCrossings, bool& partial) override;        
        //! @}
#endif

    private:
        //! Entity with Recast Navigation Mesh component.
        AZ::EntityId m_navQueryEntityId;
        //! If FindPath APIs are given points that are outside the navigation mesh, then
        //! look for the nearest point on the navigation mesh within this distance from the specified positions.
        float m_nearestDistance = 3.f;
    };
} // namespace RecastNavigation
