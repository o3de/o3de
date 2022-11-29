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
        QuadDoubleSided,
        QuadSingleSided,
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

        //! Returns a collider configuration with the entity scale applied to the collider position.
        //! Non-uniform scale is not applied here, because it is already stored in the collider position.
        Physics::ColliderConfiguration GetColliderConfigurationScaled() const;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    private:
        void UpdateCachedSamplePoints() const;
        void CreateStaticEditorCollider();
        AZ::u32 OnConfigurationChanged();
        void UpdateShapeConfigs();
        void UpdateBoxConfig(const AZ::Vector3& scale);
        void UpdateQuadConfig(const AZ::Vector3& scale);
        void UpdateCapsuleConfig(const float scale);
        void UpdateSphereConfig(const float scale);
        void UpdateCylinderConfig(const float scale);
        void UpdatePolygonPrismDecomposition();
        void UpdatePolygonPrismDecomposition(const AZ::PolygonPrismPtr polygonPrismPtr);

        // Helper function to set a specific shape configuration
        template<typename ConfigType>
        void SetShapeConfig(ShapeType shapeType, const ConfigType& shapeConfig);
        void RefreshUiProperties();

        AZ::u32 OnSubdivisionCountChange();
        AZ::Crc32 SubdivisionCountVisibility() const;
        void OnSingleSidedChange();
        AZ::Crc32 SingleSidedVisibility() const;

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
        void Display(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) const override;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        void UpdateTriggerSettings();
        void UpdateSingleSidedSettings();
        void UpdateTranslationOffset();

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
        AZStd::optional<bool> m_previousIsTrigger; //!< Stores the previous trigger setting if the shape is changed to one which does not support triggers.
        bool m_singleSided = false; //!< Used for 2d shapes like quad which may be treated as either single or doubled sided.
        AZStd::optional<bool> m_previousSingleSided; //!< Stores the previous single sided setting when unable to support single-sided shapes (such as when used with a dynamic rigid body).

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
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
