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
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/MeshColliderComponentBus.h>

#include <Editor/DebugDraw.h>

namespace AzPhysics
{
    class SceneInterface;
    struct SimulatedBody;
}

namespace PhysX
{
    struct EditorProxyPhysicsAsset
    {
        AZ_CLASS_ALLOCATOR(EditorProxyPhysicsAsset, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorProxyPhysicsAsset, "{1F69C480-CC88-4C2D-B126-B13694E6192B}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<Pipeline::MeshAsset> m_pxAsset{ AZ::Data::AssetLoadBehavior::QueueLoad };
        Physics::PhysicsAssetShapeConfiguration m_configuration;
    };

    //! Edit context wrapper for the physics asset and asset specific parameters in the shape configuration.
    struct EditorProxyAssetShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyAssetShapeConfig, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorProxyAssetShapeConfig, "{6427B76E-22F0-4DED-BB1B-AC1D4CBD45FB}");
        static void Reflect(AZ::ReflectContext* context);

        EditorProxyAssetShapeConfig() = default;
        EditorProxyAssetShapeConfig(const Physics::PhysicsAssetShapeConfiguration& assetShapeConfiguration);

        EditorProxyPhysicsAsset m_physicsAsset;

        bool m_hasNonUniformScale = false; //!< Whether there is a non-uniform scale component on this entity.
        AZ::u8 m_subdivisionLevel = 4; //!< The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.

    private:
        enum class ShapeType
        {
            Invalid,
            Primitive,
            Convex,
            TriangleMesh
        };
        AZStd::vector<ShapeType> GetShapeTypesInsideAsset() const;
        AZStd::string PhysXMeshAssetShapeTypeName() const;
        bool ShowingSubdivisionLevel() const;
        AZ::u32 OnConfigurationChanged();
    };

    class EditorMeshColliderComponentDescriptor;

    //! Editor PhysX Mesh Collider Component.
    class EditorMeshColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , protected DebugDraw::DisplayCallback
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AZ::Data::AssetBus::Handler
        , private PhysX::MeshColliderComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , private AZ::Render::MeshComponentNotificationBus::Handler
        , private PhysX::EditorColliderComponentRequestBus::Handler
        , private PhysX::EditorMeshColliderComponentRequestBus::Handler
        , private PhysX::EditorMeshColliderValidationRequestBus::Handler
        , private AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_RTTI(EditorMeshColliderComponent, "{20382794-0E74-4860-9C35-A19F22DC80D4}", AzToolsFramework::Components::EditorComponentBase);
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorMeshColliderComponent);
        AZ_CLASS_ALLOCATOR(EditorMeshColliderComponent, AZ::SystemAllocator);

        friend class EditorMeshColliderComponentDescriptor;
        static AZ::ComponentDescriptor* CreateDescriptor();

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        EditorMeshColliderComponent() = default;
        EditorMeshColliderComponent(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const EditorProxyAssetShapeConfig& proxyAssetShapeConfig,
            bool debugDrawDisplayFlagEnabled = true);
        EditorMeshColliderComponent(
            const Physics::ColliderConfiguration& colliderConfiguration, const Physics::PhysicsAssetShapeConfiguration& assetShapeConfig);

        const EditorProxyAssetShapeConfig& GetShapeConfiguration() const;
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
        AZ_DISABLE_COPY_MOVE(EditorMeshColliderComponent);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        void UpdateShapeConfiguration();

        // AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback overrides ...
        void Display(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) const override;

        void DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // AZ::Data::AssetBus::Handler overrides ...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // PhysXMeshColliderComponentRequestBus overrides ...
        AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        void UpdateMaterialSlotsFromMeshAsset();

        // TransformBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // non-uniform scale handling
        void OnNonUniformScaleChanged(const AZ::Vector3& nonUniformScale);

        // AZ::Render::MeshComponentNotificationBus overrides ...
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
            const AZ::Data::Instance<AZ::RPI::Model>& model) override;

        // PhysX::ColliderShapeBus overrides ...
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // PhysX::EditorColliderComponentRequestBus overrides ...
        void SetColliderOffset(const AZ::Vector3& offset) override;
        AZ::Vector3 GetColliderOffset() const override;
        void SetColliderRotation(const AZ::Quaternion& rotation) override;
        AZ::Quaternion GetColliderRotation() const override;
        AZ::Transform GetColliderWorldTransform() const override;
        Physics::ShapeType GetShapeType() const override { return Physics::ShapeType::PhysicsAsset; }

        // PhysX::EditorMeshColliderComponentRequestBus overrides ...
        void SetAssetScale(const AZ::Vector3& scale) override;
        AZ::Vector3 GetAssetScale() const override;

        // PhysX::EditorMeshColliderValidationRequestBus overrides ...
        void ValidateRigidBodyMeshGeometryType() override;

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

        // Mesh collider
        void UpdateMeshAsset();

        void UpdateCollider();
        void CreateStaticEditorCollider();

        void BuildDebugDrawMesh() const;

        AZ::ComponentDescriptor::StringWarningArray GetComponentWarnings() const { return m_componentWarnings; };

        // Auto-assigning collision mesh utility functions
        bool ShouldUpdateCollisionMeshFromRender() const;
        void SetCollisionMeshFromRender();
        AZ::Data::Asset<AZ::Data::AssetData> GetRenderMeshAsset() const;

        AZ::Data::AssetId FindMatchingPhysicsAsset(
            const AZ::Data::Asset<AZ::Data::AssetData>& renderMeshAsset, const AZStd::vector<AZ::Data::AssetId>& physicsAssets);

        void ValidateAssetMaterials();

        EditorProxyAssetShapeConfig m_proxyShapeConfiguration;
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
        AZ::Aabb m_cachedAabb = AZ::Aabb::CreateNull(); //!< Cache the Aabb to avoid recalculating it.
        bool m_cachedAabbDirty = true; //!< Track whether the cached Aabb needs to be recomputed.

        AZ::ComponentDescriptor::StringWarningArray m_componentWarnings;
    };

    class EditorMeshColliderComponentDescriptor :
        public AZ::ComponentDescriptorHelper<EditorMeshColliderComponent>
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorMeshColliderComponentDescriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorMeshColliderComponentDescriptor, "{FFE6E6D5-6DB0-49C8-AD1F-67FB5337842B}");

        EditorMeshColliderComponentDescriptor() = default;

        void Reflect(AZ::ReflectContext* reflection) const override;

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override;
        void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override;
        void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override;
        void GetWarnings(AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const override;
    };

} // namespace PhysX
