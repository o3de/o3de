/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <PhysX/ColliderShapeBus.h>
#include <Editor/DebugDraw.h>
#include <Editor/PolygonPrismMeshUtils.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

namespace AzPhysics
{
    class SceneInterface;
    struct SimulatedBody;
}

namespace PhysX
{
    class StaticRigidBody;

    enum class ShapeType
    {
        None,
        Box,
        Capsule,
        Sphere,
        PolygonPrism,
        Cylinder,
        Unsupported
    };

    //! Cached data for generating sample points inside the attached shape.
    struct GeometryCache
    {
        float m_height = 1.0f; //!< Caches height for capsule, cylinder and polygon prism shapes.
        float m_radius = 1.0f; //!< Caches radius for capsule, cylinder and sphere shapes.
        AZ::Vector3 m_boxDimensions = AZ::Vector3::CreateOne(); //!< Caches dimensions for box shapes.
        AZStd::vector<AZ::Vector3> m_cachedSamplePoints; //!< Stores a cache of points sampled from the shape interior.
        bool m_cachedSamplePointsDirty = true; //!< Marks whether the cached sample points need to be recalculated.
    };
    //! Editor PhysX Shape Collider Component.
    //! This component is used together with a shape component, and uses the shape information contained in that
    //! component to create geometry in the PhysX simulation.
    class EditorShapeColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AZ::TransformNotificationBus::Handler
        , protected DebugDraw::DisplayCallback
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , protected AzPhysics::SimulatedBodyComponentRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorShapeColliderComponent, "{2389DDC7-871B-42C6-9C95-2A679DDA0158}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        EditorShapeColliderComponent();

        const AZStd::vector<AZ::Vector3>& GetSamplePoints() const;

        // These functions are made virtual because we call them from other modules
        virtual const Physics::ColliderConfiguration& GetColliderConfiguration() const;
        virtual const AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>>& GetShapeConfigurations() const;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    private:
        void UpdateCachedSamplePoints() const;
        void CreateStaticEditorCollider();
        AZ::u32 OnConfigurationChanged();
        void UpdateShapeConfigs();
        void UpdateBoxConfig(const AZ::Vector3& scale);
        void UpdateCapsuleConfig(const AZ::Vector3& scale);
        void UpdateSphereConfig(const AZ::Vector3& scale);
        void UpdateCylinderConfig(const AZ::Vector3& scale);
        void UpdatePolygonPrismDecomposition();
        void UpdatePolygonPrismDecomposition(const AZ::PolygonPrismPtr polygonPrismPtr);

        // Helper function to set a specific shape configuration
        template<typename ConfigType>
        void SetShapeConfig(ShapeType shapeType, const ConfigType& shapeConfig);

        void RefreshUiProperties();

        AZ::u32 OnSubdivisionCountChange();
        AZ::Crc32 SubdivisionCountVisibility();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // handling for non-uniform scale
        void OnNonUniformScaleChanged(const AZ::Vector3& scale);

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // LmbrCentral::ShapeComponentNotificationBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason) override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const override;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        Physics::ColliderConfiguration m_colliderConfig; //!< Stores collision layers, whether the collider is a trigger, etc.
        DebugDraw::Collider m_colliderDebugDraw; //!< Handles drawing the collider based on global and local
        AzPhysics::SceneInterface* m_sceneInterface = nullptr;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SimulatedBodyHandle m_editorBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the body in the editor physics scene if there is no rigid body component.
        bool m_shapeTypeWarningIssued = false; //!< Records whether a warning about unsupported shapes has been previously issued.
        PolygonPrismMeshUtils::Mesh2D m_mesh; //!< Used for storing decompositions of the polygon prism.
        AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>> m_shapeConfigs; //!< Stores the physics shape configuration(s).
        bool m_simplePolygonErrorIssued = false; //!< Records whether an error about invalid polygon prisms has been previously raised.
        ShapeType m_shapeType = ShapeType::None; //!< Caches the current type of shape.
        //! Default number of subdivisions in the PhysX geometry representation.
        //! @note 16 is the number of subdivisions in the debug cylinder that is loaded as a mesh (not generated procedurally)
        AZ::u8 m_subdivisionCount = 16; 
        mutable GeometryCache m_geometryCache; //!< Cached data for generating sample points inside the attached shape.

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AzPhysics::SystemEvents::OnMaterialLibraryChangedEvent::Handler m_onMaterialLibraryChangedEventHandler;
        AZ::Transform m_cachedWorldTransform;
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; //!< Responds to changes in non-uniform scale.
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); //!< Caches the current non-uniform scale.
    };

    template<typename ConfigType>
    void EditorShapeColliderComponent::SetShapeConfig(ShapeType shapeType, const ConfigType& shapeConfig)
    {
        if (m_shapeType != shapeType)
        {
            m_shapeConfigs.clear();
            m_shapeType = shapeType;
        }

        if (m_shapeConfigs.empty())
        {
            m_shapeConfigs.emplace_back(AZStd::make_shared<ConfigType>(shapeConfig));
        }
        else
        {
            AZ_Assert(m_shapeConfigs.back()->GetShapeType() == shapeConfig.GetShapeType(),
                "Expected Physics shape configuration with shape type %d but found one with shape type %d.",
                static_cast<int>(shapeConfig.GetShapeType()), static_cast<int>(m_shapeConfigs.back()->GetShapeType()));
            ConfigType& configuration =
                static_cast<ConfigType&>(*m_shapeConfigs.back());
            configuration = shapeConfig;
        }
    }
} // namespace PhysX
