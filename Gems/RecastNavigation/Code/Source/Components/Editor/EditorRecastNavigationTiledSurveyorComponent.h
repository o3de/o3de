/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

namespace RecastNavigation
{
    //! This component requires a box shape component that defines a world space to collect geometry from
    //! static physical colliders present within the bounds of a shape component on the same entity.
    //!
    //! @note You can provide your implementation of collecting geometry instead of this component.
    //!       If you do, in @GetProvidedServices specify AZ_CRC_CE("RecastNavigationSurveyorService"),
    //!       which is needed by @RecastNavigationMeshComponent.
    class EditorRecastNavigationTiledSurveyorComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , public RecastNavigationSurveyorRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationTiledSurveyorComponent,
            "{F1E57D0B-11A1-46C2-876D-720DD40CB14D}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // EditorComponentBase interface implementation
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // RecastNavigationSurveyorRequestBus interface implementation
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        AZ::Aabb GetWorldBounds() const override;
        bool IsTiled() const override { return true; }
        int GetNumberOfTiles(float tileSize) const override;

    private:
        // Append the geometry within a volume
        void AppendColliderGeometry(TileGeometry& geometry, const AzPhysics::SceneQueryHits& overlapHits);

        void CollectGeometryWithinVolume(const AZ::Aabb& volume, AzPhysics::SceneQueryHits& overlapHits);

        AZStd::vector<AZStd::string> m_tagSelectionList;
        AZStd::vector<AZ::u32> m_tags;
    };
} // namespace RecastNavigation
