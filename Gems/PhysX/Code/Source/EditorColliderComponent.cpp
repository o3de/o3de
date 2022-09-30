/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Editor/EditorClassConverters.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Editor/Source/Components/EditorSystemComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/Utils.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <LyViewPaneNames.h>
#include <Editor/ConfigurationWindowBus.h>
#include <Editor/ColliderComponentMode.h>

namespace PhysX
{
    void EditorProxyCylinderShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyCylinderShapeConfig>()
                ->Version(1)
                ->Field("Configuration", &EditorProxyCylinderShapeConfig::m_configuration)
                ->Field("Subdivision", &EditorProxyCylinderShapeConfig::m_subdivisionCount)
                ->Field("Height", &EditorProxyCylinderShapeConfig::m_height)
                ->Field("Radius", &EditorProxyCylinderShapeConfig::m_radius)
            ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyCylinderShapeConfig>("EditorProxyCylinderShapeConfig", "Proxy structure to wrap cylinder data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyCylinderShapeConfig::m_configuration,
                        "Configuration", "PhysX cylinder collider configuration.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyCylinderShapeConfig::m_subdivisionCount,
                        "Subdivision", "Cylinder subdivision count.")
                        ->Attribute(AZ::Edit::Attributes::Min, Utils::MinFrustumSubdivisions)
                        ->Attribute(AZ::Edit::Attributes::Max, Utils::MaxFrustumSubdivisions)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyCylinderShapeConfig::m_height, "Height", "Cylinder height.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyCylinderShapeConfig::m_radius, "Radius", "Cylinder radius.")
                    ;
            }
        }
    }

    void EditorProxyAssetShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyAssetShapeConfig>()
                ->Version(1)
                ->Field("Asset", &EditorProxyAssetShapeConfig::m_pxAsset)
                ->Field("Configuration", &EditorProxyAssetShapeConfig::m_configuration)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyAssetShapeConfig>("EditorProxyShapeConfig", "PhysX Base collider.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyAssetShapeConfig::m_pxAsset, "PhysX Mesh",
                        "Specifies the PhysX mesh collider asset for this PhysX collider component.")
                        ->Attribute(AZ_CRC_CE("EditButton"), "")
                        ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Scene Settings")
                        ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyAssetShapeConfig::m_configuration, "Configuration",
                        "PhysX mesh asset collider configuration.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorProxyShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyAssetShapeConfig::Reflect(context);
        EditorProxyCylinderShapeConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyShapeConfig>()
                ->Version(2, &PhysX::ClassConverters::EditorProxyShapeConfigVersionConverter)
                ->Field("ShapeType", &EditorProxyShapeConfig::m_shapeType)
                ->Field("Sphere", &EditorProxyShapeConfig::m_sphere)
                ->Field("Box", &EditorProxyShapeConfig::m_box)
                ->Field("Capsule", &EditorProxyShapeConfig::m_capsule)
                ->Field("Cylinder", &EditorProxyShapeConfig::m_cylinder)
                ->Field("PhysicsAsset", &EditorProxyShapeConfig::m_physicsAsset)
                ->Field("HasNonUniformScale", &EditorProxyShapeConfig::m_hasNonUniformScale)
                ->Field("SubdivisionLevel", &EditorProxyShapeConfig::m_subdivisionLevel)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyShapeConfig>(
                    "EditorProxyShapeConfig", "PhysX Base shape collider")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorProxyShapeConfig::m_shapeType, "Shape", "The shape of the collider.")
                        ->EnumAttribute(Physics::ShapeType::Sphere, "Sphere")
                        ->EnumAttribute(Physics::ShapeType::Box, "Box")
                        ->EnumAttribute(Physics::ShapeType::Capsule, "Capsule")
                        ->EnumAttribute(Physics::ShapeType::Cylinder, "Cylinder")
                        ->EnumAttribute(Physics::ShapeType::PhysicsAsset, "PhysicsAsset")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnShapeTypeChanged)
                        // note: we do not want the user to be able to change shape types while in ComponentMode (there will
                        // potentially be different ComponentModes for different shape types)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &AzToolsFramework::ComponentModeFramework::InComponentMode)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_sphere, "Sphere", "Configuration of sphere shape.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsSphereConfig)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_box, "Box", "Configuration of box shape.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsBoxConfig)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_capsule, "Capsule", "Configuration of capsule shape.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsCapsuleConfig)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_cylinder, "Cylinder", "Configuration of cylinder shape.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsCylinderConfig)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_physicsAsset, "Asset", "Configuration of asset shape.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsAssetConfig)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_subdivisionLevel, "Subdivision level",
                        "The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.")
                        ->Attribute(AZ::Edit::Attributes::Min, Utils::MinCapsuleSubdivisionLevel)
                        ->Attribute(AZ::Edit::Attributes::Max, Utils::MaxCapsuleSubdivisionLevel)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::ShowingSubdivisionLevel)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ;
            }
        }
    }

    AZ::u32 EditorProxyShapeConfig::OnShapeTypeChanged()
    {
        // reset the physics asset if the shape type was Physics Asset
        if (m_shapeType != Physics::ShapeType::PhysicsAsset && m_lastShapeType == Physics::ShapeType::PhysicsAsset)
        {
            // clean up any reference to a physics assets, and re-initialize to an empty Pipeline::MeshAsset asset.
            m_physicsAsset.m_pxAsset.Reset();
            m_physicsAsset.m_pxAsset = AZ::Data::Asset<Pipeline::MeshAsset>(AZ::Data::AssetLoadBehavior::QueueLoad);

            m_physicsAsset.m_configuration = Physics::PhysicsAssetShapeConfiguration();
        }
        m_lastShapeType = m_shapeType;
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorProxyShapeConfig::OnConfigurationChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsTriggerService"));
    }

    void EditorColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyShapeConfig::Reflect(context);
        DebugDraw::Collider::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate old separate components
            serializeContext->ClassDeprecate(
                "EditorCapsuleColliderComponent",
                AZ::Uuid("{0BD5AF3A-35C0-4386-9930-54A2A3E97432}"),
                &ClassConverters::DeprecateEditorCapsuleColliderComponent)
                ;

            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                AZ::Uuid("{FAECF2BE-625B-469D-BBFF-E345BBB12D66}"),
                &ClassConverters::DeprecateEditorBoxColliderComponent)
                ;

            serializeContext->ClassDeprecate(
                "EditorSphereColliderComponent",
                AZ::Uuid("{D11C1624-4AE9-4B66-A6F6-40EDB9CDCE99}"),
                &ClassConverters::DeprecateEditorSphereColliderComponent)
                ;

            serializeContext->ClassDeprecate(
                "EditorMeshColliderComponent",
                AZ::Uuid("{214185DA-ABD9-4410-9819-7C177801CF7A}"),
                &ClassConverters::DeprecateEditorMeshColliderComponent)
                ;

            serializeContext->Class<EditorColliderComponent, EditorComponentBase>()
                ->Version(9, &PhysX::ClassConverters::UpgradeEditorColliderComponent)
                ->Field("ColliderConfiguration", &EditorColliderComponent::m_configuration)
                ->Field("ShapeConfiguration", &EditorColliderComponent::m_shapeConfiguration)
                ->Field("DebugDrawSettings", &EditorColliderComponent::m_colliderDebugDraw)
                ->Field("ComponentMode", &EditorColliderComponent::m_componentModeDelegate)
                ->Field("HasNonUniformScale", &EditorColliderComponent::m_hasNonUniformScale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorColliderComponent>(
                    "PhysX Collider", "Creates geometry in the PhysX simulation, using either a primitive shape or geometry from an asset.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/collider/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_configuration, "Collider Configuration", "Configuration of the collider.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_shapeConfiguration, "Shape Configuration", "Configuration of the shape.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::RemoveNotify, &EditorColliderComponent::ValidateRigidBodyMeshGeometryType)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_componentModeDelegate, "Component Mode", "Collider Component Mode.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    AZ::ComponentDescriptor* EditorColliderComponent::CreateDescriptor()
    {
        return aznew EditorColliderComponentDescriptor();
    }

    EditorColliderComponent::EditorColliderComponent(
        const Physics::ColliderConfiguration& colliderConfiguration,
        const Physics::ShapeConfiguration& shapeConfiguration)
        : m_configuration(colliderConfiguration)
        , m_shapeConfiguration(shapeConfiguration)
    {

    }

    const EditorProxyShapeConfig& EditorColliderComponent::GetShapeConfiguration() const
    {
        return m_shapeConfiguration;
    }

    const Physics::ColliderConfiguration& EditorColliderComponent::GetColliderConfiguration() const
    {
        return m_configuration;
    }

    Physics::ColliderConfiguration EditorColliderComponent::GetColliderConfigurationScaled() const
    {
        // Scale the collider offset
        Physics::ColliderConfiguration colliderConfiguration = m_configuration;
        colliderConfiguration.m_position *= Utils::GetTransformScale(GetEntityId()) * m_cachedNonUniformScale;
        return colliderConfiguration;
    }

    EditorProxyShapeConfig::EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration)
    {
        m_shapeType = shapeConfiguration.GetShapeType();
        switch (m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            m_sphere = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::Box:
            m_box = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::Capsule:
            m_capsule = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::PhysicsAsset:
            m_physicsAsset.m_configuration = static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::CookedMesh:
            m_cookedMesh = static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfiguration);
            break;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Invalid shape type!");
        }
    }

    bool EditorProxyShapeConfig::IsSphereConfig() const
    {
        return m_shapeType == Physics::ShapeType::Sphere;
    }

    bool EditorProxyShapeConfig::IsBoxConfig() const
    {
        return m_shapeType == Physics::ShapeType::Box;
    }

    bool EditorProxyShapeConfig::IsCapsuleConfig() const
    {
        return m_shapeType == Physics::ShapeType::Capsule;
    }

    bool EditorProxyShapeConfig::IsCylinderConfig() const
    {
        return m_shapeType == Physics::ShapeType::Cylinder;
    }

    bool EditorProxyShapeConfig::IsAssetConfig() const
    {
        return m_shapeType == Physics::ShapeType::PhysicsAsset;
    }

    Physics::ShapeConfiguration& EditorProxyShapeConfig::GetCurrent()
    {
        return const_cast<Physics::ShapeConfiguration&>(static_cast<const EditorProxyShapeConfig&>(*this).GetCurrent());
    }

    const Physics::ShapeConfiguration& EditorProxyShapeConfig::GetCurrent() const
    {
        switch (m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            return m_sphere;
        case Physics::ShapeType::Box:
            return m_box;
        case Physics::ShapeType::Capsule:
            return m_capsule;
        case Physics::ShapeType::Cylinder:
            return m_cylinder.m_configuration;
        case Physics::ShapeType::PhysicsAsset:
            return m_physicsAsset.m_configuration;
        case Physics::ShapeType::CookedMesh:
            return m_cookedMesh;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Unsupported shape type");
            return m_box;
        }
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> EditorProxyShapeConfig::CloneCurrent() const
    {
        switch (m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            return AZStd::make_shared<Physics::SphereShapeConfiguration>(m_sphere);
        case Physics::ShapeType::Capsule:
            return AZStd::make_shared<Physics::CapsuleShapeConfiguration>(m_capsule);
        case Physics::ShapeType::Cylinder:
            return AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(m_cylinder.m_configuration);
        case Physics::ShapeType::PhysicsAsset:
            return AZStd::make_shared<Physics::PhysicsAssetShapeConfiguration>(m_physicsAsset.m_configuration);
        case Physics::ShapeType::CookedMesh:
            return AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(m_cookedMesh);
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Unsupported shape type, defaulting to Box.");
            [[fallthrough]];
        case Physics::ShapeType::Box:
            return AZStd::make_shared<Physics::BoxShapeConfiguration>(m_box);
        }
    }

    bool EditorProxyShapeConfig::ShowingSubdivisionLevel() const
    {
        return (m_hasNonUniformScale && (IsCapsuleConfig() || IsSphereConfig() || IsAssetConfig()));
    }

    void EditorColliderComponent::Activate()
    {
        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_editorSceneHandle = m_sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        m_physXConfigChangedHandler = AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler(
            []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                    AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            });

        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        PhysX::MeshColliderComponentRequestsBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusConnect(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        EditorColliderValidationRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this](const AZ::Vector3& scale) {OnNonUniformScaleChanged(scale); });
        AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);
        m_hasNonUniformScale = (AZ::NonUniformScaleRequestBus::FindFirstHandler(GetEntityId()) != nullptr);
        m_shapeConfiguration.m_hasNonUniformScale = m_hasNonUniformScale;

        AZ::TransformBus::EventResult(m_cachedWorldTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        m_cachedNonUniformScale = AZ::Vector3::CreateOne();
        if (m_hasNonUniformScale)
        {
            AZ::NonUniformScaleRequestBus::EventResult(m_cachedNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        }

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);

        // ComponentMode
        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorColliderComponent, ColliderComponentMode>(
                AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);

        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }

        if (IsAssetConfig())
        {
            UpdateMeshAsset();
        }

        UpdateShapeConfiguration();

        CreateStaticEditorCollider();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        m_colliderDebugDraw.Disconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_nonUniformScaleChangedHandler.Disconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        EditorColliderValidationRequestBus::Handler::BusDisconnect();
        EditorColliderComponentRequestBus::Handler::BusDisconnect();
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        PhysX::MeshColliderComponentRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_componentModeDelegate.Disconnect();

        // When Deactivate is triggered from an application shutdown, it's possible that the
        // scene interface has already been deleted, so check for its existence here again
        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    AZ::u32 EditorColliderComponent::OnConfigurationChanged()
    {
        if (IsAssetConfig())
        {
            UpdateMeshAsset();
        }
        else
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(); // Disconnect since the asset is not used anymore

            m_configuration.m_materialSlots.SetSlots(Physics::MaterialDefaultSlot::Default); // Non-asset configs only have the default slot.
            m_configuration.m_materialSlots.SetSlotsReadOnly(false);
        }

        // ensure we refresh the ComponentMode (and Manipulators) when the configuration
        // changes to keep the ComponentMode in sync with the shape (otherwise the manipulators
        // will move out of alignment with the shape)
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        UpdateShapeConfiguration();
        CreateStaticEditorCollider();
        ValidateRigidBodyMeshGeometryType();

        m_colliderDebugDraw.ClearCachedGeometry();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
        }
    }

    void EditorColliderComponent::OnDeselected()
    {
        m_physXConfigChangedHandler.Disconnect();
    }

    void EditorColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto sharedColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>(m_configuration);
        BaseColliderComponent* colliderComponent = nullptr;

        auto buildGameEntityScaledPrimitive = [gameEntity](AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig,
            Physics::ShapeConfiguration& shapeConfig, AZ::u8 subdivisionLevel)
        {
            auto scaledPrimitiveConfig = Utils::CreateConvexFromPrimitive(*colliderConfig,
                shapeConfig, subdivisionLevel, shapeConfig.m_scale);
            if (scaledPrimitiveConfig.has_value())
            {
                colliderConfig->m_rotation = AZ::Quaternion::CreateIdentity();
                colliderConfig->m_position = AZ::Vector3::CreateZero();
                BaseColliderComponent* colliderComponent = gameEntity->CreateComponent<BaseColliderComponent>();
                colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig,
                    AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(scaledPrimitiveConfig.value())) });
            }
        };

        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            if (!m_hasNonUniformScale)
            {
                colliderComponent = gameEntity->CreateComponent<SphereColliderComponent>();
                colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(m_shapeConfiguration.m_sphere)) });
            }
            else
            {
                buildGameEntityScaledPrimitive(sharedColliderConfig, m_shapeConfiguration.m_sphere, m_shapeConfiguration.m_subdivisionLevel);
            }
            break;
        case Physics::ShapeType::Box:
            if (!m_hasNonUniformScale)
            {
                colliderComponent = gameEntity->CreateComponent<BoxColliderComponent>();
                colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(m_shapeConfiguration.m_box)) });
            }
            else
            {
                buildGameEntityScaledPrimitive(sharedColliderConfig, m_shapeConfiguration.m_box, m_shapeConfiguration.m_subdivisionLevel);
            }
            break;
        case Physics::ShapeType::Capsule:
            if (!m_hasNonUniformScale)
            {
                colliderComponent = gameEntity->CreateComponent<CapsuleColliderComponent>();
                colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(m_shapeConfiguration.m_capsule)) });
            }
            else
            {
                buildGameEntityScaledPrimitive(sharedColliderConfig, m_shapeConfiguration.m_capsule, m_shapeConfiguration.m_subdivisionLevel);
            }
            break;
        case Physics::ShapeType::PhysicsAsset:
            colliderComponent = gameEntity->CreateComponent<MeshColliderComponent>();

            m_shapeConfiguration.m_physicsAsset.m_configuration.m_subdivisionLevel = m_shapeConfiguration.m_subdivisionLevel;
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::PhysicsAssetShapeConfiguration>(m_shapeConfiguration.m_physicsAsset.m_configuration)) });

            AZ_Warning("PhysX", m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid(),
                "EditorColliderComponent::BuildGameEntity. No asset assigned to Collider Component. Entity: %s",
                GetEntity()->GetName().c_str());
            break;
        case Physics::ShapeType::Cylinder:
            colliderComponent = gameEntity->CreateComponent<BaseColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                sharedColliderConfig, AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(m_shapeConfiguration.m_cylinder.m_configuration)) });
            break;
        case Physics::ShapeType::CookedMesh:
            colliderComponent = gameEntity->CreateComponent<BaseColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(m_shapeConfiguration.m_cookedMesh)) });
            break;
        default:
            AZ_Warning("EditorColliderComponent", false, "Unsupported shape type for building game entity!");
            break;
        }

        StaticRigidBodyUtils::TryCreateRuntimeComponent(*GetEntity(), *gameEntity);
    }

    AZ::Transform EditorColliderComponent::GetColliderLocalTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            m_configuration.m_rotation, m_configuration.m_position);
    }

    void EditorColliderComponent::UpdateMeshAsset()
    {
        if (m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(); // Disconnect in case there was a previous asset being used.
            AZ::Data::AssetBus::Handler::BusConnect(m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId());
            m_shapeConfiguration.m_physicsAsset.m_pxAsset.QueueLoad();
            m_shapeConfiguration.m_physicsAsset.m_configuration.m_asset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;
            m_colliderDebugDraw.ClearCachedGeometry();
        }

        UpdateMaterialSlotsFromMeshAsset();
    }

    bool IsNonUniformlyScaledPrimitive(const EditorProxyShapeConfig& shapeConfig)
    {
        return shapeConfig.m_hasNonUniformScale && Utils::IsPrimitiveShape(shapeConfig.GetCurrent());
    }

    void EditorColliderComponent::CreateStaticEditorCollider()
    {
        m_cachedAabbDirty = true;

        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        if (m_shapeConfiguration.IsAssetConfig() && m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
        {
            // Mesh asset has not been loaded, wait for OnAssetReady to be invoked.
            // We specifically check Ready state here rather than ReadyPreNotify to ensure OnAssetReady has been invoked
            if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
            }
            return;
        }

        AZ::Transform colliderTransform = GetWorldTM();
        colliderTransform.ExtractUniformScale();
        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = colliderTransform.GetRotation();
        configuration.m_position = colliderTransform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        if (m_shapeConfiguration.IsAssetConfig())
        {
            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
            Utils::GetShapesFromAsset(m_shapeConfiguration.m_physicsAsset.m_configuration,
                m_configuration, m_hasNonUniformScale, m_shapeConfiguration.m_subdivisionLevel, shapes);
            configuration.m_colliderAndShapeData = shapes;
        }
        else
        {
            AZStd::shared_ptr<Physics::ColliderConfiguration> colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>(
                GetColliderConfigurationScaled());
            AZStd::shared_ptr<Physics::ShapeConfiguration> shapeConfig = m_shapeConfiguration.CloneCurrent();

            if (IsNonUniformlyScaledPrimitive(m_shapeConfiguration))
            {
                auto convexConfig = Utils::CreateConvexFromPrimitive(GetColliderConfiguration(), *(shapeConfig.get()),
                    m_shapeConfiguration.m_subdivisionLevel, shapeConfig->m_scale);
                Physics::ColliderConfiguration colliderConfigurationNoOffset = *colliderConfig;
                colliderConfigurationNoOffset.m_rotation = AZ::Quaternion::CreateIdentity();
                colliderConfigurationNoOffset.m_position = AZ::Vector3::CreateZero();

                if (convexConfig.has_value())
                {
                    AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(
                        colliderConfigurationNoOffset, convexConfig.value());
                    configuration.m_colliderAndShapeData = shape;
                }
            }
            else
            {
                configuration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig);
            }
        }

        if (m_sceneInterface)
        {
            //remove the previous body if any
            if (m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
            }
            
            m_editorBodyHandle = m_sceneInterface->AddSimulatedBody(m_editorSceneHandle, &configuration);
        }

        m_colliderDebugDraw.ClearCachedGeometry();

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    AZ::Data::Asset<Pipeline::MeshAsset> EditorColliderComponent::GetMeshAsset() const
    {
        return m_shapeConfiguration.m_physicsAsset.m_pxAsset;
    }

    void EditorColliderComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        if (id.IsValid())
        {
            m_shapeConfiguration.m_shapeType = Physics::ShapeType::PhysicsAsset;
            m_shapeConfiguration.m_physicsAsset.m_pxAsset.Create(id);
            UpdateMeshAsset();
            m_colliderDebugDraw.ClearCachedGeometry();
        }
    }

    void EditorColliderComponent::UpdateMaterialSlotsFromMeshAsset()
    {
        Utils::SetMaterialsFromPhysicsAssetShape(m_shapeConfiguration.GetCurrent(), m_configuration.m_materialSlots);

        if (IsAssetConfig())
        {
            m_configuration.m_materialSlots.SetSlotsReadOnly(m_shapeConfiguration.m_physicsAsset.m_configuration.m_useMaterialsFromAsset);
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);

        // By refreshing the entire tree the component's properties reflected on edit context
        // will get updated correctly and show the right material slots list.
        // Unfortunately, the level prefab did its check against the dirty entity before
        // this and it will save old data to file (the previous material slots list).
        // To workaround this issue we mark the entity as dirty again so the prefab
        // will save the most current data.
        // There is a side effect to this fix though, the undo stack needs to be amended and there is
        // no good way to do that at the moment. This means a user will have to hit Ctrl+Z twice
        // to revert its last change, which is not good, but not as bad as losing data.
        AzToolsFramework::ScopedUndoBatch undoBatch("PhysX editor collider component material slots updated");
        undoBatch.MarkEntityDirty(GetEntityId());

        ValidateAssetMaterials();
    }

    void EditorColliderComponent::ValidateAssetMaterials()
    {
        const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;

        if (!IsAssetConfig() || !physicsAsset.IsReady())
        {
            return;
        }

        // Here we check the material indices assigned to every shape and validate that every index is used at least once.
        // It's not an error if the validation fails here but something we want to let the designers know about.
        [[maybe_unused]] size_t materialsNum = physicsAsset->m_assetData.m_materialSlots.GetSlotsCount();
        const AZStd::vector<AZ::u16>& indexPerShape = physicsAsset->m_assetData.m_materialIndexPerShape;

        AZStd::unordered_set<AZ::u16> usedIndices;

        for (AZ::u16 index : indexPerShape)
        {
            if (index == Pipeline::MeshAssetData::TriangleMeshMaterialIndex)
            {
                // Triangle mesh indices are cooked into binary data, pass the validation in this case.
                return;
            }

            usedIndices.insert(index);
        }

        AZ_Warning("PhysX", usedIndices.size() == materialsNum,
            "EditorColliderComponent::ValidateMaterialSurfaces. Entity: %s. Number of materials used by the shape (%d) does not match the "
            "total number of materials in the asset (%d). Please check that there are no convex meshes with per-face materials. Asset: %s",
            GetEntity()->GetName().c_str(), usedIndices.size(), materialsNum, physicsAsset.GetHint().c_str())
    }

    void EditorColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration.m_physicsAsset.m_pxAsset)
        {
            m_shapeConfiguration.m_physicsAsset.m_pxAsset = asset;
            m_shapeConfiguration.m_physicsAsset.m_configuration.m_asset = asset;

            UpdateMaterialSlotsFromMeshAsset();
            CreateStaticEditorCollider();

            // Invalidate debug draw cached data
            m_colliderDebugDraw.ClearCachedGeometry();

            // Notify about the data update of the collider
            Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
            ValidateRigidBodyMeshGeometryType();
        }
        else
        {
            m_componentWarnings.clear();
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
        }
    }

    void EditorColliderComponent::ValidateRigidBodyMeshGeometryType()
    {
        const PhysX::EditorRigidBodyComponent* entityRigidbody = m_entity->FindComponent<PhysX::EditorRigidBodyComponent>();

        if (entityRigidbody &&
            m_shapeConfiguration.m_shapeType == Physics::ShapeType::PhysicsAsset &&
            m_shapeConfiguration.m_physicsAsset.m_pxAsset.IsReady())
        {
            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
            Utils::GetShapesFromAsset(m_shapeConfiguration.m_physicsAsset.m_configuration, m_configuration, m_hasNonUniformScale,
                m_shapeConfiguration.m_subdivisionLevel, shapes);

            if (shapes.empty())
            {
                m_componentWarnings.clear();

                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
                return;
            }

            //We check if the shapes are triangle meshes, if any mesh is a triangle mesh we activate the warning.
            bool shapeIsTriangleMesh = false;

            for (const auto& shape : shapes)
            {
                auto current_shape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
                if (current_shape &&
                    current_shape->GetPxShape()->getGeometryType() == physx::PxGeometryType::eTRIANGLEMESH &&
                    entityRigidbody->GetRigidBody() &&
                    entityRigidbody->GetRigidBody()->IsKinematic() == false)
                {
                    shapeIsTriangleMesh = true;
                    break;
                }
            }

            if (shapeIsTriangleMesh)
            {
                m_componentWarnings.clear();

                AZStd::string assetPath = m_shapeConfiguration.m_physicsAsset.m_configuration.m_asset.GetHint().c_str();
                const size_t lastSlash = assetPath.rfind('/');
                if (lastSlash != AZStd::string::npos)
                {
                    assetPath = assetPath.substr(lastSlash + 1);
                }

                m_componentWarnings.push_back(AZStd::string::format(
                    "The physics asset \"%s\" was exported using triangle mesh geometry, which is not compatible with non-kinematic "
                    "dynamic rigid bodies. To make the collider compatible, you can export the asset using primitive or convex mesh "
                    "geometry, use mesh decomposition when exporting the asset, or set the rigid body to kinematic. Learn more about "
                    "<a href=\"https://o3de.org/docs/user-guide/components/reference/physx/collider/\">colliders</a>.",
                    assetPath.c_str()));

                // make sure the entity inspector scrolls so the warning is visible by marking this component as having
                // new content
                AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                    &AzToolsFramework::EntityPropertyEditorRequests::SetNewComponentId, GetId());
            }
            else
            {
                m_componentWarnings.clear();
            }
        }
        else
        {
            m_componentWarnings.clear();
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            m_componentWarnings.empty() ? AzToolsFramework::Refresh_EntireTree : AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void EditorColliderComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorColliderComponent::BuildDebugDrawMesh() const
    {
        if (m_shapeConfiguration.IsAssetConfig())
        {
            const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;
            const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_shapeConfiguration.m_physicsAsset.m_configuration;

            if (!physicsAsset.IsReady())
            {
                // Skip processing if the asset isn't ready
                return;
            }

            AzPhysics::ShapeColliderPairList shapeConfigList;
            Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, m_hasNonUniformScale,
                m_shapeConfiguration.m_subdivisionLevel, shapeConfigList);

            for (size_t shapeIndex = 0; shapeIndex < shapeConfigList.size(); shapeIndex++)
            {
                const Physics::ShapeConfiguration* shapeConfiguration = shapeConfigList[shapeIndex].second.get();
                AZ_Assert(shapeConfiguration, "BuildDebugDrawMesh: Invalid shape configuration");

                if (shapeConfiguration)
                {
                    m_colliderDebugDraw.BuildMeshes(*shapeConfiguration, static_cast<AZ::u32>(shapeIndex));
                }
            }
        }
        else
        {
            const AZ::u32 shapeIndex = 0; // There's only one mesh gets built from the primitive collider, hence use geomIndex 0.
            if (m_shapeConfiguration.IsCylinderConfig())
            {
                physx::PxGeometryHolder pxGeometryHolder;
                Utils::CreatePxGeometryFromConfig(
                    m_shapeConfiguration.m_cylinder.m_configuration, pxGeometryHolder); // this will cause the native mesh to be cached
                m_colliderDebugDraw.BuildMeshes(m_shapeConfiguration.m_cylinder.m_configuration, shapeIndex);
            }
            else if (!m_hasNonUniformScale)
            {
                m_colliderDebugDraw.BuildMeshes(m_shapeConfiguration.GetCurrent(), shapeIndex);
            }
            else
            {
                m_scaledPrimitive = Utils::CreateConvexFromPrimitive(GetColliderConfiguration(), m_shapeConfiguration.GetCurrent(),
                    m_shapeConfiguration.m_subdivisionLevel, m_shapeConfiguration.GetCurrent().m_scale);
                if (m_scaledPrimitive.has_value())
                {
                    physx::PxGeometryHolder pxGeometryHolder;
                    Utils::CreatePxGeometryFromConfig(m_scaledPrimitive.value(), pxGeometryHolder); // this will cause the native mesh to be cached
                    m_colliderDebugDraw.BuildMeshes(m_scaledPrimitive.value(), shapeIndex);
                }
            }
        }
    }

    void EditorColliderComponent::DisplayCylinderCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const AZ::u32 shapeIndex = 0;
        m_colliderDebugDraw.DrawMesh(
            debugDisplay,
            m_configuration,
            m_shapeConfiguration.m_cylinder.m_configuration,
            m_shapeConfiguration.m_cylinder.m_configuration.m_scale,
            shapeIndex);
    }

    void EditorColliderComponent::DisplayScaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (m_scaledPrimitive.has_value())
        {
            const AZ::u32 shapeIndex = 0;
            Physics::ColliderConfiguration colliderConfigNoOffset = m_configuration;
            colliderConfigNoOffset.m_rotation = AZ::Quaternion::CreateIdentity();
            colliderConfigNoOffset.m_position = AZ::Vector3::CreateZero();
            m_colliderDebugDraw.DrawMesh(debugDisplay, colliderConfigNoOffset, m_scaledPrimitive.value(),
                GetWorldTM().GetUniformScale() * m_cachedNonUniformScale, shapeIndex);
        }
    }

    void EditorColliderComponent::DisplayUnscaledPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            m_colliderDebugDraw.DrawSphere(debugDisplay, m_configuration, m_shapeConfiguration.m_sphere);
            break;
        case Physics::ShapeType::Box:
            m_colliderDebugDraw.DrawBox(debugDisplay, m_configuration, m_shapeConfiguration.m_box);
            break;
        case Physics::ShapeType::Capsule:
            m_colliderDebugDraw.DrawCapsule(debugDisplay, m_configuration, m_shapeConfiguration.m_capsule);
            break;
        }
    }

    void EditorColliderComponent::DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry())
        {
            return;
        }

        const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_shapeConfiguration.m_physicsAsset.m_configuration;

        AzPhysics::ShapeColliderPairList shapeConfigList;
        Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, m_hasNonUniformScale,
            m_shapeConfiguration.m_subdivisionLevel, shapeConfigList);

        const AZ::Vector3& assetScale = physicsAssetConfiguration.m_assetScale;

        for (size_t shapeIndex = 0; shapeIndex < shapeConfigList.size(); shapeIndex++)
        {
            const Physics::ColliderConfiguration* colliderConfiguration = shapeConfigList[shapeIndex].first.get();
            const Physics::ShapeConfiguration* shapeConfiguration = shapeConfigList[shapeIndex].second.get();

            AZ_Assert(shapeConfiguration && colliderConfiguration, "DisplayMeshCollider: Invalid shape-collider configuration pair");

            switch (shapeConfiguration->GetShapeType())
            {
            case Physics::ShapeType::CookedMesh:
            {
                const Physics::CookedMeshShapeConfiguration* cookedMeshShapeConfiguration =
                    static_cast<const Physics::CookedMeshShapeConfiguration*>(shapeConfiguration);

                const AZ::Vector3 overallScale = Utils::GetTransformScale(GetEntityId()) * m_cachedNonUniformScale * assetScale;
                Physics::ColliderConfiguration nonUniformScaledColliderConfiguration = *colliderConfiguration;
                nonUniformScaledColliderConfiguration.m_position *= m_cachedNonUniformScale;

                m_colliderDebugDraw.DrawMesh(debugDisplay, nonUniformScaledColliderConfiguration, *cookedMeshShapeConfiguration,
                    overallScale, static_cast<AZ::u32>(shapeIndex));
                break;
            }
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration* sphereShapeConfiguration =
                    static_cast<const Physics::SphereShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawSphere(debugDisplay, *colliderConfiguration, *sphereShapeConfiguration, assetScale);
                break;
            }
            case Physics::ShapeType::Box:
            {
                const Physics::BoxShapeConfiguration* boxShapeConfiguration =
                    static_cast<const Physics::BoxShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawBox(debugDisplay, *colliderConfiguration, *boxShapeConfiguration, assetScale);
                break;
            }
            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration* capsuleShapeConfiguration =
                    static_cast<const Physics::CapsuleShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawCapsule(debugDisplay, *colliderConfiguration, *capsuleShapeConfiguration, assetScale);
                break;
            }
            default:
            {
                AZ_Error("EditorColliderComponent", false, "DisplayMeshCollider: Unsupported ShapeType %d. Entity %s, ID: %llu",
                    static_cast<AZ::u32>(shapeConfiguration->GetShapeType()), GetEntity()->GetName().c_str(), GetEntityId());
                break;
            }
            }
        }
    }

    void EditorColliderComponent::Display([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry())
        {
            BuildDebugDrawMesh();
        }

        if (m_colliderDebugDraw.HasCachedGeometry())
        {
            if (m_shapeConfiguration.IsAssetConfig())
            {
                DisplayMeshCollider(debugDisplay);
            }
            else if (m_shapeConfiguration.IsCylinderConfig())
            {
                DisplayCylinderCollider(debugDisplay);
            }
            else 
            {
                if (m_hasNonUniformScale)
                {
                    DisplayScaledPrimitiveCollider(debugDisplay);
                }
                else
                {
                    DisplayUnscaledPrimitiveCollider(debugDisplay);
                }
            }
        }
    }

    bool EditorColliderComponent::IsAssetConfig() const
    {
        return m_shapeConfiguration.IsAssetConfig();
    }

    AZ::Vector3 EditorColliderComponent::GetDimensions()
    {
        return m_shapeConfiguration.m_box.m_dimensions;
    }

    void EditorColliderComponent::SetDimensions(const AZ::Vector3& dimensions)
    {
        m_shapeConfiguration.m_box.m_dimensions = dimensions;
        CreateStaticEditorCollider();
    }

    AZ::Transform EditorColliderComponent::GetCurrentTransform()
    {
        return GetWorldTM();
    }

    AZ::Transform EditorColliderComponent::GetCurrentLocalTransform()
    {
        return GetColliderLocalTransform();
    }

    AZ::Vector3 EditorColliderComponent::GetBoxScale()
    {
        return AZ::Vector3::CreateOne();
    }

    void EditorColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (world.IsClose(m_cachedWorldTransform))
        {
            return;
        }
        m_cachedWorldTransform = world;

        UpdateShapeConfiguration();
        CreateStaticEditorCollider();
    }

    void EditorColliderComponent::OnNonUniformScaleChanged(const AZ::Vector3& nonUniformScale)
    {
        m_cachedNonUniformScale = nonUniformScale;

        UpdateShapeConfiguration();
        CreateStaticEditorCollider();
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb EditorColliderComponent::GetColliderShapeAabb()
    {
        if (m_cachedAabbDirty)
        {
            m_cachedAabb = PhysX::Utils::GetColliderAabb(GetWorldTM()
                , m_hasNonUniformScale
                , m_shapeConfiguration.m_subdivisionLevel
                , m_shapeConfiguration.GetCurrent()
                , m_configuration);
            m_cachedAabbDirty = false;
        }

        return m_cachedAabb;
    }

    void EditorColliderComponent::UpdateShapeConfigurationScale()
    {
        auto& shapeConfiguration = m_shapeConfiguration.GetCurrent();
        shapeConfiguration.m_scale = GetWorldTM().ExtractUniformScale() * m_cachedNonUniformScale;
        m_colliderDebugDraw.ClearCachedGeometry();
    }

    void EditorColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            CreateStaticEditorCollider();
        }
    }

    void EditorColliderComponent::DisablePhysics()
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    bool EditorColliderComponent::IsPhysicsEnabled() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->m_simulating;
            }
        }
        return false;
    }

    AZ::Aabb EditorColliderComponent::GetAabb() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* EditorColliderComponent::GetSimulatedBody()
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body;
            }
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyHandle EditorColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_editorBodyHandle;
    }

    AzPhysics::SceneQueryHit EditorColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->RayCast(request);
            }
        }
        return AzPhysics::SceneQueryHit();
    }

    bool EditorColliderComponent::IsTrigger()
    {
        return m_configuration.m_isTrigger;
    }

    void EditorColliderComponent::SetColliderOffset(const AZ::Vector3& offset)
    {
        m_configuration.m_position = offset;
        CreateStaticEditorCollider();
    }

    AZ::Vector3 EditorColliderComponent::GetColliderOffset() const
    {
        return m_configuration.m_position;
    }

    void EditorColliderComponent::SetColliderRotation(const AZ::Quaternion& rotation)
    {
        m_configuration.m_rotation = rotation;
        CreateStaticEditorCollider();
    }

    AZ::Quaternion EditorColliderComponent::GetColliderRotation() const
    {
        return m_configuration.m_rotation;
    }

    AZ::Transform EditorColliderComponent::GetColliderWorldTransform() const
    {
        return GetWorldTM() * GetColliderLocalTransform();
    }

    bool EditorColliderComponent::ShouldUpdateCollisionMeshFromRender() const
    {
        if (!m_shapeConfiguration.IsAssetConfig())
        {
            return false;
        }

        bool collisionMeshNotSet = !m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid();
        return collisionMeshNotSet;
    }

    AZ::Data::AssetId EditorColliderComponent::FindMatchingPhysicsAsset(
        const AZ::Data::Asset<AZ::Data::AssetData>& renderMeshAsset,
        const AZStd::vector<AZ::Data::AssetId>& physicsAssets)
    {
        AZ::Data::AssetId foundAssetId;

        // Extract the file name from the path to the asset
        AZStd::string renderMeshFileName;
        AzFramework::StringFunc::Path::Split(renderMeshAsset.GetHint().c_str(),
            nullptr, nullptr, &renderMeshFileName);

        // Find the collision mesh asset matching the render mesh
        for (const AZ::Data::AssetId& assetId : physicsAssets)
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath,
                &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

            AZStd::string physicsAssetFileName;
            AzFramework::StringFunc::Path::Split(assetPath.c_str(), nullptr, nullptr, &physicsAssetFileName);

            if (physicsAssetFileName == renderMeshFileName)
            {
                foundAssetId = assetId;
                break;
            }
        }

        return foundAssetId;
    };

    AZ::Data::Asset<AZ::Data::AssetData> EditorColliderComponent::GetRenderMeshAsset() const
    {
        // Try Atom MeshComponent
        AZ::Data::Asset<AZ::RPI::ModelAsset> atomMeshAsset;
        AZ::Render::MeshComponentRequestBus::EventResult(atomMeshAsset, GetEntityId(),
            &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);

        return atomMeshAsset;
    }

    void EditorColliderComponent::SetCollisionMeshFromRender()
    {
        AZ::Data::Asset<AZ::Data::AssetData> renderMeshAsset = GetRenderMeshAsset();
        if (!renderMeshAsset.GetId().IsValid())
        {
            // No render mesh component assigned
            return;
        }

        bool productsQueryResult = false;
        AZStd::vector<AZ::Data::AssetInfo> productsInfo;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(productsQueryResult,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, 
            renderMeshAsset.GetId().m_guid, productsInfo);

        if (productsQueryResult)
        {
            AZStd::vector<AZ::Data::AssetId> physicsAssets;
            physicsAssets.reserve(productsInfo.size());

            for (const AZ::Data::AssetInfo& info : productsInfo)
            {
                if (info.m_assetType == AZ::AzTypeInfo<Pipeline::MeshAsset>::Uuid())
                {
                    physicsAssets.push_back(info.m_assetId);
                }
            }

            // If there's only one physics asset, we set it regardless of the name
            if (physicsAssets.size() == 1)
            {
                SetMeshAsset(physicsAssets[0]);
            }
            // For multiple assets we pick the one matching the name of the render mesh asset
            else if (physicsAssets.size() > 1)
            {
                AZ::Data::AssetId matchingPhysicsAsset = FindMatchingPhysicsAsset(renderMeshAsset, physicsAssets);

                if (matchingPhysicsAsset.IsValid())
                {
                    SetMeshAsset(matchingPhysicsAsset);
                }
                else
                {
                    AZ_Warning("EditorColliderComponent", false,
                        "SetCollisionMeshFromRender on entity %s: Unable to find a matching physics asset "
                        "for the render mesh asset GUID: %s, hint: %s",
                        GetEntity()->GetName().c_str(),
                        renderMeshAsset.GetId().m_guid.ToString<AZStd::string>().c_str(),
                        renderMeshAsset.GetHint().c_str());
                }
            }
            // This is not necessarily an incorrect case but it's worth reporting 
            // in case if we forgot to configure the source asset to produce the collision mesh
            else if (physicsAssets.empty())
            {
                AZ_TracePrintf("EditorColliderComponent",
                    "SetCollisionMeshFromRender on entity %s: The source asset for %s did not produce any physics assets",
                    GetEntity()->GetName().c_str(),
                    renderMeshAsset.GetHint().c_str());
            }
        }
        else
        {
            AZ_Warning("EditorColliderComponent", false, 
                "SetCollisionMeshFromRender on entity %s: Unable to get the assets produced by the render mesh asset GUID: %s, hint: %s",
                GetEntity()->GetName().c_str(), 
                renderMeshAsset.GetId().m_guid.ToString<AZStd::string>().c_str(),
                renderMeshAsset.GetHint().c_str());
        }
    }

    void EditorColliderComponent::OnModelReady([[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
        [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }
    }

    void EditorColliderComponent::SetShapeType(Physics::ShapeType shapeType)
    {
        m_shapeConfiguration.m_shapeType = shapeType;

        if (shapeType == Physics::ShapeType::Cylinder)
        {
            UpdateCylinderCookedMesh();
        }

        CreateStaticEditorCollider();
    }

    Physics::ShapeType EditorColliderComponent::GetShapeType() const
    {
        return m_shapeConfiguration.GetCurrent().GetShapeType();
    }

    void EditorColliderComponent::SetSphereRadius(float radius)
    {
        m_shapeConfiguration.m_sphere.m_radius = radius;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetSphereRadius() const
    {
        return m_shapeConfiguration.m_sphere.m_radius;
    }

    void EditorColliderComponent::SetCapsuleRadius(float radius)
    {
        m_shapeConfiguration.m_capsule.m_radius = radius;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCapsuleRadius() const
    {
        return m_shapeConfiguration.m_capsule.m_radius;
    }

    void EditorColliderComponent::SetCapsuleHeight(float height)
    {
        m_shapeConfiguration.m_capsule.m_height = height;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCapsuleHeight() const
    {
        return m_shapeConfiguration.m_capsule.m_height;
    }

    void EditorColliderComponent::SetCylinderRadius(float radius)
    {
        if (radius <= 0.0f)
        {
            AZ_Error("PhysX", false, "SetCylinderRadius: radius must be greater than zero.");
            return;
        }

        m_shapeConfiguration.m_cylinder.m_radius = radius;
        UpdateCylinderCookedMesh();
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCylinderRadius() const
    {
        return m_shapeConfiguration.m_cylinder.m_radius;
    }

    void EditorColliderComponent::SetCylinderHeight(float height)
    {
        if (height <= 0.0f)
        {
            AZ_Error("PhysX", false, "SetCylinderHeight: height must be greater than zero.");
            return;
        }

        m_shapeConfiguration.m_cylinder.m_height = height;
        UpdateCylinderCookedMesh();
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCylinderHeight() const
    {
        return m_shapeConfiguration.m_cylinder.m_height;
    }

    void EditorColliderComponent::SetCylinderSubdivisionCount(AZ::u8 subdivisionCount)
    {
        const AZ::u8 clampedSubdivisionCount = AZ::GetClamp(subdivisionCount, Utils::MinFrustumSubdivisions, Utils::MaxFrustumSubdivisions);
        AZ_Warning(
            "PhysX",
            clampedSubdivisionCount == subdivisionCount,
            "Requested cylinder subdivision count %d clamped into allowed range (%d - %d). Entity: %s",
            subdivisionCount,
            Utils::MinFrustumSubdivisions,
            Utils::MaxFrustumSubdivisions,
            GetEntity()->GetName().c_str());
        m_shapeConfiguration.m_cylinder.m_subdivisionCount = clampedSubdivisionCount;
        UpdateCylinderCookedMesh();
        CreateStaticEditorCollider();
    }

    AZ::u8 EditorColliderComponent::GetCylinderSubdivisionCount() const
    {
        return m_shapeConfiguration.m_cylinder.m_subdivisionCount;
    }

    void EditorColliderComponent::SetAssetScale(const AZ::Vector3& scale)
    {
        m_shapeConfiguration.m_physicsAsset.m_configuration.m_assetScale = scale;
        CreateStaticEditorCollider();
    }

    AZ::Vector3 EditorColliderComponent::GetAssetScale() const
    {
        return m_shapeConfiguration.m_physicsAsset.m_configuration.m_assetScale;
    }

    void EditorColliderComponent::UpdateShapeConfiguration()
    {
        UpdateShapeConfigurationScale();

        if (m_shapeConfiguration.IsCylinderConfig())
        {
            // Create cooked cylinder convex
            UpdateCylinderCookedMesh();
        }
    }

    void EditorColliderComponent::UpdateCylinderCookedMesh()
    {
        const AZ::u8 subdivisionCount = m_shapeConfiguration.m_cylinder.m_subdivisionCount;
        const float height = m_shapeConfiguration.m_cylinder.m_height;
        const float radius = m_shapeConfiguration.m_cylinder.m_radius;

        if (height <= 0.0f)
        {
            AZ_Error("PhysX", false, "Cylinder height must be greater than zero. Entity: %s", GetEntity()->GetName().c_str());
            return;
        }

        if (radius <= 0.0f)
        {
            AZ_Error("PhysX", false, "Cylinder radius must be greater than zero. Entity: %s", GetEntity()->GetName().c_str());
            return;
        }

        Utils::Geometry::PointList samplePoints = Utils::CreatePointsAtFrustumExtents(height, radius, radius, subdivisionCount).value();

        const AZ::Vector3 scale = m_shapeConfiguration.m_cylinder.m_configuration.m_scale;
        m_shapeConfiguration.m_cylinder.m_configuration = Utils::CreatePxCookedMeshConfiguration(samplePoints, scale).value();
    }

    AZ::Aabb EditorColliderComponent::GetWorldBounds()
    {
        return GetAabb();
    }

    AZ::Aabb EditorColliderComponent::GetLocalBounds()
    {
        AZ::Aabb worldBounds = GetWorldBounds();
        if (worldBounds.IsValid())
        {
            return worldBounds.GetTransformedAabb(m_cachedWorldTransform.GetInverse());
        }

        return AZ::Aabb::CreateNull();
    }

    void EditorColliderComponentDescriptor::Reflect(AZ::ReflectContext* reflection) const
    {
        EditorColliderComponent::Reflect(reflection);
    }

    void EditorColliderComponentDescriptor::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorColliderComponent::GetProvidedServices(provided);
    }

    void EditorColliderComponentDescriptor::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorColliderComponent::GetDependentServices(dependent);
    }

    void EditorColliderComponentDescriptor::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorColliderComponent::GetRequiredServices(required);
    }

    void EditorColliderComponentDescriptor::GetWarnings(AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const
    {
        const PhysX::EditorColliderComponent* editorColliderComponent = azrtti_cast<const PhysX::EditorColliderComponent*>(instance);

        if (editorColliderComponent != nullptr)
        {
            warnings = editorColliderComponent->GetComponentWarnings();
        }
    }
} // namespace PhysX
