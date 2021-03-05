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

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/WorldBodyBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>

#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/MeshColliderComponentBus.h>
#include <System/PhysXSystem.h>

#include <Editor/DebugDraw.h>

namespace PhysX
{
    struct EditorProxyAssetShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyAssetShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorProxyAssetShapeConfig, "{C1B46450-C2A3-4115-A2FB-E5FF3BAAAD15}");
        static void Reflect(AZ::ReflectContext* context);
        virtual ~EditorProxyAssetShapeConfig() = default;

        AZ::Data::Asset<Pipeline::MeshAsset> m_pxAsset{ AZ::Data::AssetLoadBehavior::QueueLoad };
        Physics::PhysicsAssetShapeConfiguration m_configuration;
    };

    /// Proxy container for only displaying a specific shape configuration depending on the shapeType selected.
    struct EditorProxyShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorProxyShapeConfig, "{531FB42A-42A9-4234-89BA-FD349EF83D0C}");
        static void Reflect(AZ::ReflectContext* context);

        EditorProxyShapeConfig() = default;
        EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration);
        virtual ~EditorProxyShapeConfig() = default;

        Physics::ShapeType m_shapeType = Physics::ShapeType::PhysicsAsset;
        Physics::SphereShapeConfiguration m_sphere;
        Physics::BoxShapeConfiguration m_box;
        Physics::CapsuleShapeConfiguration m_capsule;
        EditorProxyAssetShapeConfig m_physicsAsset;
        Physics::CookedMeshShapeConfiguration m_cookedMesh;

        bool IsSphereConfig() const;
        bool IsBoxConfig() const;
        bool IsCapsuleConfig() const;
        bool IsAssetConfig() const;
        Physics::ShapeConfiguration& GetCurrent();
        const Physics::ShapeConfiguration& GetCurrent() const;

        AZ::u32 OnConfigurationChanged();
    };

    class EditorColliderComponentDescriptor;
    /// Editor PhysX Collider Component.
    ///
    class EditorColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected DebugDraw::DisplayCallback
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::BoxManipulatorRequestBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private PhysX::MeshColliderComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , private LmbrCentral::MeshComponentNotificationBus::Handler
        , private AZ::Render::MeshComponentNotificationBus::Handler
        , private PhysX::EditorColliderComponentRequestBus::Handler
        , private Physics::WorldBodyRequestBus::Handler
    {
    public:
        AZ_RTTI(EditorColliderComponent, "{FD429282-A075-4966-857F-D0BBF186CFE6}", AzToolsFramework::Components::EditorComponentBase);
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorColliderComponent);

        AZ_CLASS_ALLOCATOR(EditorColliderComponent, AZ::SystemAllocator, 0);
        friend class EditorColliderComponentDescriptor;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
            provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
            provided.push_back(AZ_CRC("PhysXTriggerService", 0x3a117d7b));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Not compatible with Legacy Cry Physics services
            incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
            incompatible.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

        static AZ::ComponentDescriptor* CreateDescriptor();

        EditorColliderComponent() = default;
        EditorColliderComponent(
            const Physics::ColliderConfiguration& colliderConfiguration,
            const Physics::ShapeConfiguration& shapeConfiguration);

        // these functions are made virtual because we call them from other modules
        virtual const EditorProxyShapeConfig& GetShapeConfiguration() const;
        virtual const Physics::ColliderConfiguration& GetColliderConfiguration() const;
        virtual Physics::ColliderConfiguration GetColliderConfigurationScaled() const;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorColliderComponent)
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        /// AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DisplayPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // PhysXMeshColliderComponentRequestBus
        AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const override;
        void GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const override;
        Physics::MaterialId GetMaterialId() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialId(const Physics::MaterialId& id) override;
        void UpdateMaterialSlotsFromMeshAsset();

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AzToolsFramework::BoxManipulatorRequestBus
        AZ::Vector3 GetDimensions() override;
        void SetDimensions(const AZ::Vector3& dimensions) override;
        AZ::Transform GetCurrentTransform() override;
        AZ::Vector3 GetBoxScale() override;

        // LmbrCentral::MeshComponentNotificationBus
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;

        // AZ::Render::MeshComponentNotificationBus
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
            const AZ::Data::Instance<AZ::RPI::Model>& model) override;

        // PhysX::ColliderShapeBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // PhysX::EditorColliderComponentBus
        void SetColliderOffset(const AZ::Vector3& offset) override;
        AZ::Vector3 GetColliderOffset() override;
        void SetColliderRotation(const AZ::Quaternion& rotation) override;
        AZ::Quaternion GetColliderRotation() override;
        AZ::Transform GetColliderWorldTransform() override;
        void SetShapeType(Physics::ShapeType shapeType) override;
        Physics::ShapeType GetShapeType() override;
        void SetSphereRadius(float radius) override;
        float GetSphereRadius() override;
        void SetCapsuleRadius(float radius) override;
        float GetCapsuleRadius() override;
        void SetCapsuleHeight(float height) override;
        float GetCapsuleHeight() override;
        void SetAssetScale(const AZ::Vector3& scale) override;
        AZ::Vector3 GetAssetScale() override;

        AZ::Transform GetColliderLocalTransform() const;

        EditorProxyShapeConfig m_shapeConfiguration;
        Physics::ColliderConfiguration m_configuration;

        AZ::u32 OnConfigurationChanged();
        void UpdateShapeConfigurationScale();

        // WorldBodyRequestBus
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        Physics::WorldBody* GetWorldBody() override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

        // Mesh collider
        void UpdateMeshAsset();
        bool IsAssetConfig() const;

        void CreateStaticEditorCollider();
        void ClearStaticEditorCollider();

        void BuildDebugDrawMesh() const;

        void ValidateRigidBodyMeshGeometryType();

        AZ::ComponentDescriptor::StringWarningArray GetComponentWarnings() const { return m_componentWarnings; };

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode.

        AZStd::unique_ptr<Physics::RigidBodyStatic> m_editorBody;

        // Auto-assigning collision mesh utility functions
        bool ShouldUpdateCollisionMeshFromRender() const;
        void SetCollisionMeshFromRender();
        AZ::Data::Asset<AZ::Data::AssetData> GetRenderMeshAsset() const;

        AZ::Data::AssetId FindMatchingPhysicsAsset(const AZ::Data::Asset<AZ::Data::AssetData>& renderMeshAsset,
            const AZStd::vector<AZ::Data::AssetId>& physicsAssets);

        void ValidateMaterialSurfaces();

        void InitEventHandlers();
        DebugDraw::Collider m_colliderDebugDraw;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AzPhysics::SystemEvents::OnDefaultMaterialLibraryChangedEvent::Handler m_onDefaultMaterialLibraryChangedEventHandler;
        AZ::Transform m_cachedWorldTransform;


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
        void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible, const AZ::Component* instance) const override;
        void GetWarnings(AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const override;

    };

} // namespace PhysX
