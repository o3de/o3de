/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Math/Quaternion.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/MeshColliderComponentBus.h>
#include <System/PhysXSystem.h>

#include <Editor/DebugDraw.h>

namespace AzPhysics
{
    class SceneInterface;
    struct SimulatedBody;
}

namespace PhysX
{
    //! Edit context wrapper for the physics asset and asset specific parameters in the shape configuration.
    struct EditorProxyAssetShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyAssetShapeConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorProxyAssetShapeConfig, "{C1B46450-C2A3-4115-A2FB-E5FF3BAAAD15}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<Pipeline::MeshAsset> m_pxAsset{ AZ::Data::AssetLoadBehavior::QueueLoad };
        Physics::PhysicsAssetShapeConfiguration m_configuration;
    };

    //! Edit context wrapper for cylinder specific parameters and cached geometry.
    struct EditorProxyCylinderShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyCylinderShapeConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorProxyCylinderShapeConfig, "{2394B3D0-E7A1-4B66-8C42-0FFDC1FCAA26}");
        static void Reflect(AZ::ReflectContext* context);

        //! Cylinder specific parameters.
        AZ::u8 m_subdivisionCount = 16;
        float m_height = 1.0f;
        float m_radius = 1.0f;

        //! Configuration stores the convex geometry for the cylinder and shape scale.
        Physics::CookedMeshShapeConfiguration m_configuration;
    };

    //! Proxy container for only displaying a specific shape configuration depending on the shapeType selected.
    struct EditorProxyShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyShapeConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorProxyShapeConfig, "{531FB42A-42A9-4234-89BA-FD349EF83D0C}");
        static void Reflect(AZ::ReflectContext* context);

        EditorProxyShapeConfig() = default;
        EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration);

        Physics::ShapeType m_shapeType = Physics::ShapeType::PhysicsAsset;
        Physics::SphereShapeConfiguration m_sphere;
        Physics::BoxShapeConfiguration m_box;
        Physics::CapsuleShapeConfiguration m_capsule;
        EditorProxyCylinderShapeConfig m_cylinder;
        EditorProxyAssetShapeConfig m_physicsAsset;
        bool m_hasNonUniformScale = false; //!< Whether there is a non-uniform scale component on this entity.
        AZ::u8 m_subdivisionLevel = 4; //!< The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.
        Physics::CookedMeshShapeConfiguration m_cookedMesh;

        bool IsSphereConfig() const;
        bool IsBoxConfig() const;
        bool IsCapsuleConfig() const;
        bool IsCylinderConfig() const;
        bool IsAssetConfig() const;
        Physics::ShapeConfiguration& GetCurrent();
        const Physics::ShapeConfiguration& GetCurrent() const;

        AZStd::shared_ptr<Physics::ShapeConfiguration> CloneCurrent() const;

    private:
        bool ShowingSubdivisionLevel() const;
        AZ::u32 OnShapeTypeChanged();
        AZ::u32 OnConfigurationChanged();

        Physics::ShapeType m_lastShapeType = Physics::ShapeType::PhysicsAsset;
    };

    class EditorColliderComponentDescriptor;
    //! Editor PhysX Collider Component.
    //!
    class EditorColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected DebugDraw::DisplayCallback
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::BoxManipulatorRequestBus::Handler
        , private AZ::Data::AssetBus::Handler
        , private PhysX::MeshColliderComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , private AZ::Render::MeshComponentNotificationBus::Handler
        , private PhysX::EditorColliderComponentRequestBus::Handler
        , private PhysX::EditorColliderValidationRequestBus::Handler
        , private AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_RTTI(EditorColliderComponent, "{FD429282-A075-4966-857F-D0BBF186CFE6}", AzToolsFramework::Components::EditorComponentBase);
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorColliderComponent);

        AZ_CLASS_ALLOCATOR(EditorColliderComponent, AZ::SystemAllocator, 0);
        friend class EditorColliderComponentDescriptor;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static AZ::ComponentDescriptor* CreateDescriptor();

        EditorColliderComponent() = default;
        EditorColliderComponent(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration);

        // these functions are made virtual because we call them from other modules
        virtual const EditorProxyShapeConfig& GetShapeConfiguration() const;
        virtual const Physics::ColliderConfiguration& GetColliderConfiguration() const;
        virtual Physics::ColliderConfiguration GetColliderConfigurationScaled() const;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorColliderComponent)
        // AZ::Component
        void Activate() override;

        void UpdateShapeConfiguration();

        void Deactivate() override;

        //! AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback overrides...
        void Display(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) const override;

        void DisplayCylinderCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayUnscaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayScaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // PhysXMeshColliderComponentRequestBus
        AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        void UpdateMaterialSlotsFromMeshAsset();

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // non-uniform scale handling
        void OnNonUniformScaleChanged(const AZ::Vector3& nonUniformScale);

        // AzToolsFramework::BoxManipulatorRequestBus
        AZ::Vector3 GetDimensions() override;
        void SetDimensions(const AZ::Vector3& dimensions) override;
        AZ::Transform GetCurrentTransform() override;
        AZ::Transform GetCurrentLocalTransform() override;
        AZ::Vector3 GetBoxScale() override;

        // AZ::Render::MeshComponentNotificationBus
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
            const AZ::Data::Instance<AZ::RPI::Model>& model) override;

        // PhysX::ColliderShapeBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // PhysX::EditorColliderComponentRequestBus
        void SetColliderOffset(const AZ::Vector3& offset) override;
        AZ::Vector3 GetColliderOffset() const override;
        void SetColliderRotation(const AZ::Quaternion& rotation) override;
        AZ::Quaternion GetColliderRotation() const override;
        AZ::Transform GetColliderWorldTransform() const override;
        void SetShapeType(Physics::ShapeType shapeType) override;
        Physics::ShapeType GetShapeType() const override;
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
        void SetAssetScale(const AZ::Vector3& scale) override;
        AZ::Vector3 GetAssetScale() const override;

        // PhysX::EditorColliderValidationRequestBus overrides ...
        void ValidateRigidBodyMeshGeometryType() override;

        AZ::Transform GetColliderLocalTransform() const;

        EditorProxyShapeConfig m_shapeConfiguration;
        Physics::ColliderConfiguration m_configuration;

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

        // Mesh collider
        void UpdateMeshAsset();
        bool IsAssetConfig() const;

        // Cylinder collider
        void UpdateCylinderCookedMesh();

        void CreateStaticEditorCollider();
        void ClearStaticEditorCollider();

        void BuildDebugDrawMesh() const;

        AZ::ComponentDescriptor::StringWarningArray GetComponentWarnings() const { return m_componentWarnings; };

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        AzPhysics::SceneInterface* m_sceneInterface = nullptr;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SimulatedBodyHandle m_editorBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;

        // Auto-assigning collision mesh utility functions
        bool ShouldUpdateCollisionMeshFromRender() const;
        void SetCollisionMeshFromRender();
        AZ::Data::Asset<AZ::Data::AssetData> GetRenderMeshAsset() const;

        AZ::Data::AssetId FindMatchingPhysicsAsset(const AZ::Data::Asset<AZ::Data::AssetData>& renderMeshAsset,
            const AZStd::vector<AZ::Data::AssetId>& physicsAssets);

        void ValidateAssetMaterials();

        void InitEventHandlers();
        DebugDraw::Collider m_colliderDebugDraw;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AZ::Transform m_cachedWorldTransform;

        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; //!< Responds to changes in non-uniform scale.
        bool m_hasNonUniformScale = false; //!< Whether there is a non-uniform scale component on this entity.
        AZ::Vector3 m_cachedNonUniformScale = AZ::Vector3::CreateOne(); //!< Caches the current non-uniform scale.
        mutable AZStd::optional<Physics::CookedMeshShapeConfiguration> m_scaledPrimitive; //!< Approximation for non-uniformly scaled primitive.
        AZ::Aabb m_cachedAabb = AZ::Aabb::CreateNull(); //!< Cache the Aabb to avoid recalculating it.
        bool m_cachedAabbDirty = true; //!< Track whether the cached Aabb needs to be recomputed.

        AZ::ComponentDescriptor::StringWarningArray m_componentWarnings;
    };

    class EditorColliderComponentDescriptor :
        public AZ::ComponentDescriptorHelper<EditorColliderComponent>
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorColliderComponentDescriptor, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorColliderComponentDescriptor, "{E099B5D6-B03F-436C-AB8E-7ADE4DAD74A0}");

        EditorColliderComponentDescriptor() = default;

        void Reflect(AZ::ReflectContext* reflection) const override;

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override;
        void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override;
        void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override;
        void GetWarnings(AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const override;

    };

} // namespace PhysX
