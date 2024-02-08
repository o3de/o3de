/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Math/Quaternion.h>

#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>

#include <Editor/DebugDraw.h>

namespace AzPhysics
{
    class SceneInterface;
    struct SimulatedBody;
}

namespace PhysX
{
    //! Legacy edit context wrapper for the physics asset and asset specific parameters in the shape configuration.
    //!
    //! O3DE_DEPRECATION_NOTICE(GHI-14718)
    //! Physics asset shape is now handled by EditorMeshColliderComponent.
    //! This class is only used to keep the serialization data intact inside
    //! EditorColliderComponent so it can be converted to EditorMeshColliderComponent
    //! when running the console command 'ed_physxUpdatePrefabsWithColliderComponents'.
    struct LegacyEditorProxyAssetShapeConfig
    {
        AZ_CLASS_ALLOCATOR(LegacyEditorProxyAssetShapeConfig, AZ::SystemAllocator);
        AZ_TYPE_INFO(LegacyEditorProxyAssetShapeConfig, "{C1B46450-C2A3-4115-A2FB-E5FF3BAAAD15}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<Pipeline::MeshAsset> m_pxAsset{ AZ::Data::AssetLoadBehavior::QueueLoad };
        Physics::PhysicsAssetShapeConfiguration m_configuration;
    };

    //! Edit context wrapper for cylinder specific parameters and cached geometry.
    struct EditorProxyCylinderShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyCylinderShapeConfig, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorProxyCylinderShapeConfig, "{2394B3D0-E7A1-4B66-8C42-0FFDC1FCAA26}");
        static void Reflect(AZ::ReflectContext* context);

        //! Cylinder specific parameters.
        AZ::u8 m_subdivisionCount = Physics::ShapeConstants::DefaultCylinderSubdivisionCount;
        float m_height = Physics::ShapeConstants::DefaultCylinderHeight;
        float m_radius = Physics::ShapeConstants::DefaultCylinderRadius;

        //! Configuration stores the convex geometry for the cylinder and shape scale.
        Physics::CookedMeshShapeConfiguration m_configuration;
    };

    //! Proxy container for only displaying a specific shape configuration depending on the shapeType selected.
    struct EditorProxyShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyShapeConfig, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorProxyShapeConfig, "{531FB42A-42A9-4234-89BA-FD349EF83D0C}");
        static void Reflect(AZ::ReflectContext* context);

        EditorProxyShapeConfig() = default;
        EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration);

        // O3DE_DEPRECATION_NOTICE(GHI-14718)
        // Initial value for m_shapeType needs to remain PhysicsAsset
        // to support command to convert EditorColliderComponent to
        // EditorMeshColliderComponent. This is because prefabs do not
        // store in JSON the default values and therefore the converter
        // would lose the ability to know if the type was PhysX Asset
        // before to convert the component to a Editor Mesh Collider.
        // The initial value can be changed to Box when GHI-14718 deprecation
        // task is done.
        Physics::ShapeType m_shapeType = Physics::ShapeType::PhysicsAsset;
        Physics::SphereShapeConfiguration m_sphere;
        Physics::BoxShapeConfiguration m_box;
        Physics::CapsuleShapeConfiguration m_capsule;
        EditorProxyCylinderShapeConfig m_cylinder;
        LegacyEditorProxyAssetShapeConfig m_legacyPhysicsAsset; // O3DE_DEPRECATION_NOTICE(GHI-14718)
        bool m_hasNonUniformScale = false; //!< Whether there is a non-uniform scale component on this entity.
        AZ::u8 m_subdivisionLevel = 4; //!< The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.
        Physics::CookedMeshShapeConfiguration m_cookedMesh;

        bool IsSphereConfig() const;
        bool IsBoxConfig() const;
        bool IsCapsuleConfig() const;
        bool IsCylinderConfig() const;
        Physics::ShapeConfiguration& GetCurrent();
        const Physics::ShapeConfiguration& GetCurrent() const;

        AZStd::shared_ptr<Physics::ShapeConfiguration> CloneCurrent() const;

        bool IsNonUniformlyScaledPrimitive() const;

    private:
        bool ShowingSubdivisionLevel() const;
        AZ::u32 OnShapeTypeChanged();
        AZ::u32 OnConfigurationChanged();

        Physics::ShapeType m_lastShapeType = Physics::ShapeType::Box;
    };

    //! Editor PhysX Primitive Collider Component.
    class EditorColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , protected DebugDraw::DisplayCallback
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::BoxManipulatorRequestBus::Handler
        , private AzToolsFramework::ShapeManipulatorRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , private PhysX::EditorColliderComponentRequestBus::Handler
        , private PhysX::EditorPrimitiveColliderComponentRequestBus::Handler
        , private AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorColliderComponent, "{FD429282-A075-4966-857F-D0BBF186CFE6}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        EditorColliderComponent() = default;
        EditorColliderComponent(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration);

        const EditorProxyShapeConfig& GetShapeConfiguration() const;
        const Physics::ColliderConfiguration& GetColliderConfiguration() const;
        Physics::ColliderConfiguration GetColliderConfigurationScaled() const;
        Physics::ColliderConfiguration GetColliderConfigurationNoOffset() const;

        bool IsDebugDrawDisplayFlagEnabled() const;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorColliderComponent);

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void UpdateShapeConfiguration();

        // AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback overrides ...
        void Display(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) const override;

        void DisplayCylinderCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayUnscaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayScaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // TransformBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // non-uniform scale handling
        void OnNonUniformScaleChanged(const AZ::Vector3& nonUniformScale);

        // AzToolsFramework::BoxManipulatorRequestBus overrides ...
        AZ::Vector3 GetDimensions() const override;
        void SetDimensions(const AZ::Vector3& dimensions) override;
        AZ::Transform GetCurrentLocalTransform() const override;

        // AzToolsFramework::ShapeManipulatorRequestBus overrides ...
        AZ::Vector3 GetTranslationOffset() const override;
        void SetTranslationOffset(const AZ::Vector3& translationOffset) override;
        AZ::Transform GetManipulatorSpace() const override;
        AZ::Quaternion GetRotationOffset() const override;

        // PhysX::ColliderShapeBus overrides ...
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // PhysX::EditorColliderComponentRequestBus overrides ...
        void SetColliderOffset(const AZ::Vector3& offset) override;
        AZ::Vector3 GetColliderOffset() const override;
        void SetColliderRotation(const AZ::Quaternion& rotation) override;
        AZ::Quaternion GetColliderRotation() const override;
        AZ::Transform GetColliderWorldTransform() const override;
        Physics::ShapeType GetShapeType() const override;
        
        // PhysX::EditorPrimitiveColliderComponentRequestBus overrides ...
        void SetShapeType(Physics::ShapeType shapeType) override;
        void SetBoxDimensions(const AZ::Vector3& dimensions) override;
        AZ::Vector3 GetBoxDimensions() const override;
        void SetSphereRadius(float radius) override;
        float GetSphereRadius() const override;
        void SetCapsuleRadius(float radius) override;
        float GetCapsuleRadius() const override;
        void SetCapsuleHeight(float height) override;
        float GetCapsuleHeight() const override;
        void SetCylinderRadius(float radius) override;
        float GetCylinderRadius() const override;
        void SetCylinderHeight(float height) override;
        float GetCylinderHeight() const override;
        void SetCylinderSubdivisionCount(AZ::u8 subdivisionCount) override;
        AZ::u8 GetCylinderSubdivisionCount() const override;

        AZ::Transform GetColliderLocalTransform() const;

        AZ::u32 OnConfigurationChanged();
        void UpdateShapeConfigurationScale();

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // Cylinder collider
        void UpdateCylinderCookedMesh();

        void UpdateCollider();
        void CreateStaticEditorCollider();

        void BuildDebugDrawMesh() const;

        EditorProxyShapeConfig m_proxyShapeConfiguration;
        Physics::ColliderConfiguration m_configuration;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        AzPhysics::SceneInterface* m_sceneInterface = nullptr;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SimulatedBodyHandle m_editorBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;

        DebugDraw::Collider m_colliderDebugDraw;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AZ::Transform m_cachedWorldTransform;

        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; //!< Responds to changes in non-uniform scale.
        bool m_hasNonUniformScale = false; //!< Whether there is a non-uniform scale component on this entity.
        AZ::Vector3 m_cachedNonUniformScale = AZ::Vector3::CreateOne(); //!< Caches the current non-uniform scale.
        mutable AZStd::optional<Physics::CookedMeshShapeConfiguration> m_scaledPrimitive; //!< Approximation for non-uniformly scaled primitive.
        AZ::Aabb m_cachedAabb = AZ::Aabb::CreateNull(); //!< Cache the Aabb to avoid recalculating it.
        bool m_cachedAabbDirty = true; //!< Track whether the cached Aabb needs to be recomputed.
    };

} // namespace PhysX
