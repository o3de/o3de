/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Utils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/ColliderComponentMode.h>
#include <System/PhysXSystem.h>

#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorStaticRigidBodyComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/Utils.h>
#include <Source/EditorMeshColliderComponent.h>

namespace PhysX
{
    void EditorProxyPhysicsAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyPhysicsAsset>()
                ->Version(1)
                ->Field("Asset", &EditorProxyPhysicsAsset::m_pxAsset)
                ->Field("Configuration", &EditorProxyPhysicsAsset::m_configuration);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyPhysicsAsset>("EditorProxyPhysicsAsset", "PhysX Asset.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorProxyPhysicsAsset::m_pxAsset,
                        "PhysX Mesh",
                        "Specifies the PhysX mesh collider asset for this PhysX collider component.")
                    ->Attribute(AZ_CRC_CE("EditButton"), "")
                    ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Scene Settings")
                    ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorProxyPhysicsAsset::m_configuration,
                        "Configuration",
                        "PhysX mesh asset collider configuration.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorProxyAssetShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyPhysicsAsset::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyAssetShapeConfig>()
                ->Version(1)
                ->Field("PhysicsAsset", &EditorProxyAssetShapeConfig::m_physicsAsset)
                ->Field("HasNonUniformScale", &EditorProxyAssetShapeConfig::m_hasNonUniformScale)
                ->Field("SubdivisionLevel", &EditorProxyAssetShapeConfig::m_subdivisionLevel);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyAssetShapeConfig>("EditorProxyAssetShapeConfig", "PhysX asset collider.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorProxyAssetShapeConfig::m_physicsAsset,
                        "Asset",
                        "Configuration of asset shape.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyAssetShapeConfig::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorProxyAssetShapeConfig::PhysXMeshAssetShapeTypeName)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorProxyAssetShapeConfig::m_subdivisionLevel,
                        "Subdivision level",
                        "The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.")
                    ->Attribute(AZ::Edit::Attributes::Min, Utils::MinCapsuleSubdivisionLevel)
                    ->Attribute(AZ::Edit::Attributes::Max, Utils::MaxCapsuleSubdivisionLevel)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyAssetShapeConfig::ShowingSubdivisionLevel)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyAssetShapeConfig::OnConfigurationChanged);
            }
        }
    }

    EditorProxyAssetShapeConfig::EditorProxyAssetShapeConfig(
        const Physics::PhysicsAssetShapeConfiguration& assetShapeConfiguration)
    {
        m_physicsAsset.m_pxAsset = assetShapeConfiguration.m_asset;
        m_physicsAsset.m_configuration = assetShapeConfiguration;
    }

    AZStd::vector<EditorProxyAssetShapeConfig::ShapeType> EditorProxyAssetShapeConfig::GetShapeTypesInsideAsset() const
    {
        if (!m_physicsAsset.m_pxAsset.IsReady())
        {
            return {};
        }

        Physics::ColliderConfiguration defaultColliderConfiguration;
        Physics::PhysicsAssetShapeConfiguration physicsAssetConfiguration = m_physicsAsset.m_configuration;
        physicsAssetConfiguration.m_asset = m_physicsAsset.m_pxAsset;
        physicsAssetConfiguration.m_assetScale = AZ::Vector3::CreateOne(); // Remove the scale so it doesn't affect the query for the asset mesh type
        const bool hasNonUniformScale = false;

        AzPhysics::ShapeColliderPairList shapeConfigList;
        Utils::GetColliderShapeConfigsFromAsset(
            physicsAssetConfiguration, defaultColliderConfiguration, hasNonUniformScale, m_subdivisionLevel, shapeConfigList);

        AZStd::vector<ShapeType> shapeTypes;
        shapeTypes.reserve(shapeConfigList.size());
        for (const auto& shapeConfig : shapeConfigList)
        {
            const Physics::ShapeConfiguration* shapeConfiguration = shapeConfig.second.get();
            AZ_Assert(shapeConfiguration, "GetShapeTypesInsideAsset: Invalid shape-collider configuration pair");

            switch (shapeConfiguration->GetShapeType())
            {
            case Physics::ShapeType::CookedMesh:
                {
                    const Physics::CookedMeshShapeConfiguration* cookedMeshShapeConfiguration =
                        static_cast<const Physics::CookedMeshShapeConfiguration*>(shapeConfiguration);
                    switch (cookedMeshShapeConfiguration->GetMeshType())
                    {
                    case Physics::CookedMeshShapeConfiguration::MeshType::Convex:
                        shapeTypes.push_back(ShapeType::Convex);
                        break;
                    case Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh:
                        shapeTypes.push_back(ShapeType::TriangleMesh);
                        break;
                    default:
                        shapeTypes.push_back(ShapeType::Invalid);
                        break;
                    }
                }
                break;
            case Physics::ShapeType::Sphere:
            case Physics::ShapeType::Box:
            case Physics::ShapeType::Capsule:
                shapeTypes.push_back(ShapeType::Primitive);
                break;
            default:
                shapeTypes.push_back(ShapeType::Invalid);
                break;
            }
        }

        return shapeTypes;
    }

    AZStd::string EditorProxyAssetShapeConfig::PhysXMeshAssetShapeTypeName() const
    {
        const AZStd::string assetName = "Asset";

        const AZStd::vector<ShapeType> shapeTypes = GetShapeTypesInsideAsset();
        if (shapeTypes.empty())
        {
            return assetName;
        }

        // Using the first shape type as representative for shapes inside the asset.
        switch (shapeTypes[0])
        {
        case ShapeType::Primitive:
            return assetName + " (Primitive)";
        case ShapeType::Convex:
            return assetName + " (Convex)";
        case ShapeType::TriangleMesh:
            return assetName + " (Triangle Mesh)";
        default:
            return assetName;
        }
    }

    bool EditorProxyAssetShapeConfig::ShowingSubdivisionLevel() const
    {
        const AZStd::vector<ShapeType> shapeTypes = GetShapeTypesInsideAsset();

        return m_hasNonUniformScale &&
            AZStd::any_of(
                   shapeTypes.begin(),
                   shapeTypes.end(),
                   [](const ShapeType& shapeType)
                   {
                       return shapeType == ShapeType::Primitive;
                   });
    }

    AZ::u32 EditorProxyAssetShapeConfig::OnConfigurationChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorMeshColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsTriggerService"));
    }

    void EditorMeshColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorMeshColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorMeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyAssetShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMeshColliderComponent, EditorComponentBase>()
                ->Version(1 + (1<<PX_PHYSICS_VERSION_MAJOR)) // Use PhysX version to trigger prefabs recompilation when switching between PhysX 4 and 5.
                ->Field("ColliderConfiguration", &EditorMeshColliderComponent::m_configuration)
                ->Field("ShapeConfiguration", &EditorMeshColliderComponent::m_proxyShapeConfiguration)
                ->Field("DebugDrawSettings", &EditorMeshColliderComponent::m_colliderDebugDraw)
                ->Field("ComponentMode", &EditorMeshColliderComponent::m_componentModeDelegate)
                ->Field("HasNonUniformScale", &EditorMeshColliderComponent::m_hasNonUniformScale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorMeshColliderComponent>(
                    "PhysX Mesh Collider", "Creates geometry in the PhysX simulation using geometry from an asset.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXMeshCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXMeshCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/mesh-collider/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshColliderComponent::m_configuration, "Collider Configuration", "Configuration of the collider.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMeshColliderComponent::OnConfigurationChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorMeshColliderComponent::m_proxyShapeConfiguration,
                        "Shape Configuration",
                        "Configuration of physics asset shape.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMeshColliderComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::RemoveNotify, &EditorMeshColliderComponent::ValidateRigidBodyMeshGeometryType)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorMeshColliderComponent::m_componentModeDelegate,
                        "Component Mode",
                        "Collider Component Mode.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorMeshColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    AZ::ComponentDescriptor* EditorMeshColliderComponent::CreateDescriptor()
    {
        return aznew EditorMeshColliderComponentDescriptor();
    }

    EditorMeshColliderComponent::EditorMeshColliderComponent(
        const Physics::ColliderConfiguration& colliderConfiguration,
        const EditorProxyAssetShapeConfig& proxyAssetShapeConfig,
        bool debugDrawDisplayFlagEnabled)
        : m_configuration(colliderConfiguration)
        , m_proxyShapeConfiguration(proxyAssetShapeConfig)
    {
        m_colliderDebugDraw.SetDisplayFlag(debugDrawDisplayFlagEnabled);
    }

    EditorMeshColliderComponent::EditorMeshColliderComponent(
        const Physics::ColliderConfiguration& colliderConfiguration,
        const Physics::PhysicsAssetShapeConfiguration& assetShapeConfig)
        : m_configuration(colliderConfiguration)
        , m_proxyShapeConfiguration(assetShapeConfig)
    {
    }

    const EditorProxyAssetShapeConfig& EditorMeshColliderComponent::GetShapeConfiguration() const
    {
        return m_proxyShapeConfiguration;
    }

    const Physics::ColliderConfiguration& EditorMeshColliderComponent::GetColliderConfiguration() const
    {
        return m_configuration;
    }

    Physics::ColliderConfiguration EditorMeshColliderComponent::GetColliderConfigurationScaled() const
    {
        // Scale the collider offset
        Physics::ColliderConfiguration colliderConfiguration = m_configuration;
        colliderConfiguration.m_position *= Utils::GetTransformScale(GetEntityId()) * m_cachedNonUniformScale;
        return colliderConfiguration;
    }

    Physics::ColliderConfiguration EditorMeshColliderComponent::GetColliderConfigurationNoOffset() const
    {
        Physics::ColliderConfiguration colliderConfiguration = m_configuration;
        colliderConfiguration.m_position = AZ::Vector3::CreateZero();
        colliderConfiguration.m_rotation = AZ::Quaternion::CreateIdentity();
        return colliderConfiguration;
    }

    bool EditorMeshColliderComponent::IsDebugDrawDisplayFlagEnabled() const
    {
        return m_colliderDebugDraw.IsDisplayFlagEnabled();
    }

    void EditorMeshColliderComponent::Activate()
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

        const auto entityId = GetEntityId();
        const auto componentId = GetId();

        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(entityId);
        PhysX::MeshColliderComponentRequestsBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ColliderShapeRequestBus::Handler::BusConnect(entityId);
        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(entityId);
        EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, componentId));
        EditorMeshColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, componentId));
        EditorMeshColliderValidationRequestBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this](const AZ::Vector3& scale) {OnNonUniformScaleChanged(scale); });
        AZ::NonUniformScaleRequestBus::Event(
            entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);

        AZ::TransformBus::EventResult(m_cachedWorldTransform, entityId, &AZ::TransformInterface::GetWorldTM);
        m_cachedNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_cachedNonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);

        // Debug drawing
        m_colliderDebugDraw.Connect(entityId);
        m_colliderDebugDraw.SetDisplayCallback(this);

        // ComponentMode
        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorMeshColliderComponent, ColliderComponentMode>(
            AZ::EntityComponentIdPair(entityId, componentId), nullptr);

        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }

        UpdateMeshAsset();

        UpdateCollider();
    }

    void EditorMeshColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        m_colliderDebugDraw.Disconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_nonUniformScaleChangedHandler.Disconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        EditorMeshColliderValidationRequestBus::Handler::BusDisconnect();
        EditorMeshColliderComponentRequestBus::Handler::BusDisconnect();
        EditorColliderComponentRequestBus::Handler::BusDisconnect();
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
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

    AZ::u32 EditorMeshColliderComponent::OnConfigurationChanged()
    {
        UpdateMeshAsset();

        // ensure we refresh the ComponentMode (and Manipulators) when the configuration
        // changes to keep the ComponentMode in sync with the shape (otherwise the manipulators
        // will move out of alignment with the shape)
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        UpdateCollider();
        ValidateRigidBodyMeshGeometryType();

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorMeshColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
        }
    }

    void EditorMeshColliderComponent::OnDeselected()
    {
        m_physXConfigChangedHandler.Disconnect();
    }

    void EditorMeshColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto sharedColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>(m_configuration);
        m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_subdivisionLevel = m_proxyShapeConfiguration.m_subdivisionLevel;

        auto* meshColliderComponent = gameEntity->CreateComponent<MeshColliderComponent>();
        meshColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
            sharedColliderConfig,
            AZStd::make_shared<Physics::PhysicsAssetShapeConfiguration>(m_proxyShapeConfiguration.m_physicsAsset.m_configuration)) });

        AZ_Warning(
            "PhysX",
            m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid(),
            "EditorMeshColliderComponent::BuildGameEntity. No asset assigned to Collider Component. Entity: %s",
            GetEntity()->GetName().c_str());
    }

    AZ::Transform EditorMeshColliderComponent::GetColliderLocalTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            m_configuration.m_rotation, m_configuration.m_position);
    }

    void EditorMeshColliderComponent::UpdateMeshAsset()
    {
        if (m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(); // Disconnect in case there was a previous asset being used.
            AZ::Data::AssetBus::Handler::BusConnect(m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.GetId());
            m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.QueueLoad();
            m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_asset = m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset;
            m_colliderDebugDraw.ClearCachedGeometry();
        }

        UpdateMaterialSlotsFromMeshAsset();
    }

    void EditorMeshColliderComponent::UpdateCollider()
    {
        UpdateShapeConfiguration();
        CreateStaticEditorCollider();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorMeshColliderComponent::CreateStaticEditorCollider()
    {
        m_cachedAabbDirty = true;

        if (!GetEntity()->FindComponent<EditorStaticRigidBodyComponent>())
        {
            m_colliderDebugDraw.ClearCachedGeometry();
            return;
        }

        if (m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
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

        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
        Utils::CreateShapesFromAsset(
            m_proxyShapeConfiguration.m_physicsAsset.m_configuration,
            m_configuration, m_hasNonUniformScale, m_proxyShapeConfiguration.m_subdivisionLevel, shapes);
        configuration.m_colliderAndShapeData = shapes;

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

    AZ::Data::Asset<Pipeline::MeshAsset> EditorMeshColliderComponent::GetMeshAsset() const
    {
        return m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset;
    }

    void EditorMeshColliderComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        if (id.IsValid())
        {
            m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.Create(id);
            UpdateMeshAsset();
            m_colliderDebugDraw.ClearCachedGeometry();
        }
    }

    void EditorMeshColliderComponent::UpdateMaterialSlotsFromMeshAsset()
    {
        Utils::SetMaterialsFromPhysicsAssetShape(m_proxyShapeConfiguration.m_physicsAsset.m_configuration, m_configuration.m_materialSlots);

        m_configuration.m_materialSlots.SetSlotsReadOnly(m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_useMaterialsFromAsset);

        InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);

        // By refreshing the entire tree the component's properties reflected on edit context
        // will get updated correctly and show the right material slots list.
        // Unfortunately, the level prefab did its check against the dirty entity before
        // this and it will save old data to file (the previous material slots list).
        // To workaround this issue we mark the entity as dirty again so the prefab
        // will save the most current data.
        // There is a side effect to this fix though, the undo stack needs to be amended and there is
        // no good way to do that at the moment. This means a user will have to hit Ctrl+Z twice
        // to revert its last change, which is not good, but not as bad as losing data.
        AzToolsFramework::ScopedUndoBatch undoBatch("PhysX editor mesh collider component material slots updated");
        undoBatch.MarkEntityDirty(GetEntityId());

        ValidateAssetMaterials();
    }

    void EditorMeshColliderComponent::ValidateAssetMaterials()
    {
        const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset;

        if (!physicsAsset.IsReady())
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
            "EditorMeshColliderComponent::ValidateMaterialSurfaces. Entity: %s. Number of materials used by the shape (%d) does not match the "
            "total number of materials in the asset (%d). Please check that there are no convex meshes with per-face materials. Asset: %s",
            GetEntity()->GetName().c_str(), usedIndices.size(), materialsNum, physicsAsset.GetHint().c_str())
    }

    void EditorMeshColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset)
        {
            m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset = asset;
            m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_asset = asset;

            UpdateMaterialSlotsFromMeshAsset();
            UpdateCollider();
            ValidateRigidBodyMeshGeometryType();
        }
        else
        {
            m_componentWarnings.clear();
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
        }
    }

    void EditorMeshColliderComponent::ValidateRigidBodyMeshGeometryType()
    {
        const PhysX::EditorRigidBodyComponent* entityRigidbody = m_entity->FindComponent<PhysX::EditorRigidBodyComponent>();

        if (entityRigidbody && m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.IsReady())
        {
            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
            Utils::CreateShapesFromAsset(
                m_proxyShapeConfiguration.m_physicsAsset.m_configuration,
                m_configuration,
                m_hasNonUniformScale,
                m_proxyShapeConfiguration.m_subdivisionLevel, shapes);

            if (shapes.empty())
            {
                m_componentWarnings.clear();

                InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
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

                AZStd::string assetPath = m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_asset.GetHint().c_str();
                const size_t lastSlash = assetPath.rfind('/');
                if (lastSlash != AZStd::string::npos)
                {
                    assetPath = assetPath.substr(lastSlash + 1);
                }

                m_componentWarnings.push_back(AZStd::string::format(
                    "The physics asset \"%s\" was exported using triangle mesh geometry, which is not compatible with non-kinematic "
                    "dynamic rigid bodies. To make the collider compatible, you can export the asset using primitive or convex mesh "
                    "geometry, use mesh decomposition when exporting the asset, or set the rigid body to kinematic. Learn more about "
                    "<a href=\"https://o3de.org/docs/user-guide/components/reference/physx/mesh-collider/\">colliders</a>.",
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

        InvalidatePropertyDisplay(
            m_componentWarnings.empty() ? AzToolsFramework::Refresh_EntireTree : AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void EditorMeshColliderComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorMeshColliderComponent::BuildDebugDrawMesh() const
    {
        const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset;
        const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_proxyShapeConfiguration.m_physicsAsset.m_configuration;

        if (!physicsAsset.IsReady())
        {
            // Skip processing if the asset isn't ready
            return;
        }

        AzPhysics::ShapeColliderPairList shapeConfigList;
        Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, m_hasNonUniformScale,
            m_proxyShapeConfiguration.m_subdivisionLevel, shapeConfigList);

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

    void EditorMeshColliderComponent::DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry() || !m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_asset.IsReady())
        {
            return;
        }

        const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_proxyShapeConfiguration.m_physicsAsset.m_configuration;

        AzPhysics::ShapeColliderPairList shapeConfigList;
        Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, m_hasNonUniformScale,
            m_proxyShapeConfiguration.m_subdivisionLevel, shapeConfigList);

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
                AZ_Error("EditorMeshColliderComponent", false, "DisplayMeshCollider: Unsupported ShapeType %d. Entity %s, ID: %llu",
                    static_cast<AZ::u32>(shapeConfiguration->GetShapeType()), GetEntity()->GetName().c_str(), GetEntityId());
                break;
            }
            }
        }
    }

    void EditorMeshColliderComponent::Display(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry())
        {
            BuildDebugDrawMesh();
        }

        if (m_colliderDebugDraw.HasCachedGeometry())
        {
            DisplayMeshCollider(debugDisplay);
        }
    }

    void EditorMeshColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (world.IsClose(m_cachedWorldTransform))
        {
            return;
        }
        m_cachedWorldTransform = world;

        UpdateCollider();
    }

    void EditorMeshColliderComponent::OnNonUniformScaleChanged(const AZ::Vector3& nonUniformScale)
    {
        m_cachedNonUniformScale = nonUniformScale;

        UpdateCollider();
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb EditorMeshColliderComponent::GetColliderShapeAabb()
    {
        if (m_cachedAabbDirty)
        {
            m_cachedAabb = PhysX::Utils::GetColliderAabb(GetWorldTM()
                , m_hasNonUniformScale
                , m_proxyShapeConfiguration.m_subdivisionLevel
                , m_proxyShapeConfiguration.m_physicsAsset.m_configuration
                , m_configuration);
            m_cachedAabbDirty = false;
        }

        return m_cachedAabb;
    }

    void EditorMeshColliderComponent::UpdateShapeConfigurationScale()
    {
        const AZ::Vector3& assetScale = m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_assetScale;
        m_hasNonUniformScale =
            !Physics::Utils::HasUniformScale(assetScale) || (AZ::NonUniformScaleRequestBus::FindFirstHandler(GetEntityId()) != nullptr);
        m_proxyShapeConfiguration.m_hasNonUniformScale = m_hasNonUniformScale;
        m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_scale = GetWorldTM().ExtractUniformScale() * m_cachedNonUniformScale;
    }

    void EditorMeshColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            UpdateCollider();
        }
    }

    void EditorMeshColliderComponent::DisablePhysics()
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    bool EditorMeshColliderComponent::IsPhysicsEnabled() const
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

    AZ::Aabb EditorMeshColliderComponent::GetAabb() const
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

    AzPhysics::SimulatedBody* EditorMeshColliderComponent::GetSimulatedBody()
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

    AzPhysics::SimulatedBodyHandle EditorMeshColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_editorBodyHandle;
    }

    AzPhysics::SceneQueryHit EditorMeshColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
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

    bool EditorMeshColliderComponent::IsTrigger()
    {
        return m_configuration.m_isTrigger;
    }

    void EditorMeshColliderComponent::SetColliderOffset(const AZ::Vector3& offset)
    {
        m_configuration.m_position = offset;
        UpdateCollider();
    }

    AZ::Vector3 EditorMeshColliderComponent::GetColliderOffset() const
    {
        return m_configuration.m_position;
    }

    void EditorMeshColliderComponent::SetColliderRotation(const AZ::Quaternion& rotation)
    {
        m_configuration.m_rotation = rotation;
        UpdateCollider();
    }

    AZ::Quaternion EditorMeshColliderComponent::GetColliderRotation() const
    {
        return m_configuration.m_rotation;
    }

    AZ::Transform EditorMeshColliderComponent::GetColliderWorldTransform() const
    {
        return GetWorldTM() * GetColliderLocalTransform();
    }

    bool EditorMeshColliderComponent::ShouldUpdateCollisionMeshFromRender() const
    {
        const bool collisionMeshNotSet = !m_proxyShapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid();
        return collisionMeshNotSet;
    }

    AZ::Data::AssetId EditorMeshColliderComponent::FindMatchingPhysicsAsset(
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

    AZ::Data::Asset<AZ::Data::AssetData> EditorMeshColliderComponent::GetRenderMeshAsset() const
    {
        // Try Atom MeshComponent
        AZ::Data::Asset<AZ::RPI::ModelAsset> atomMeshAsset;
        AZ::Render::MeshComponentRequestBus::EventResult(atomMeshAsset, GetEntityId(),
            &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);

        return atomMeshAsset;
    }

    void EditorMeshColliderComponent::SetCollisionMeshFromRender()
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
                    AZ_Warning("EditorMeshColliderComponent", false,
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
                AZ_TracePrintf("EditorMeshColliderComponent",
                    "SetCollisionMeshFromRender on entity %s: The source asset for %s did not produce any physics assets",
                    GetEntity()->GetName().c_str(),
                    renderMeshAsset.GetHint().c_str());
            }
        }
        else
        {
            AZ_Warning("EditorMeshColliderComponent", false, 
                "SetCollisionMeshFromRender on entity %s: Unable to get the assets produced by the render mesh asset GUID: %s, hint: %s",
                GetEntity()->GetName().c_str(), 
                renderMeshAsset.GetId().m_guid.ToString<AZStd::string>().c_str(),
                renderMeshAsset.GetHint().c_str());
        }
    }

    void EditorMeshColliderComponent::OnModelReady(
        [[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
        [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }
    }

    void EditorMeshColliderComponent::SetAssetScale(const AZ::Vector3& scale)
    {
        m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_assetScale = scale;
        UpdateCollider();
    }

    AZ::Vector3 EditorMeshColliderComponent::GetAssetScale() const
    {
        return m_proxyShapeConfiguration.m_physicsAsset.m_configuration.m_assetScale;
    }

    void EditorMeshColliderComponent::UpdateShapeConfiguration()
    {
        UpdateShapeConfigurationScale();
    }

    AZ::Aabb EditorMeshColliderComponent::GetWorldBounds() const
    {
        return GetAabb();
    }

    AZ::Aabb EditorMeshColliderComponent::GetLocalBounds() const
    {
        AZ::Aabb worldBounds = GetWorldBounds();
        if (worldBounds.IsValid())
        {
            return worldBounds.GetTransformedAabb(m_cachedWorldTransform.GetInverse());
        }

        return AZ::Aabb::CreateNull();
    }

    bool EditorMeshColliderComponent::SupportsEditorRayIntersect()
    {
        return true;
    }

    AZ::Aabb EditorMeshColliderComponent::GetEditorSelectionBoundsViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
    {
        return GetWorldBounds();
    }

    bool EditorMeshColliderComponent::EditorSelectionIntersectRayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        AzPhysics::RayCastRequest request;
        request.m_direction = dir;
        request.m_distance = distance;
        request.m_start = src;

        if (auto hit = RayCast(request))
        {
            distance = hit.m_distance;
            return true;
        }

        return false;
    }

    void EditorMeshColliderComponentDescriptor::Reflect(AZ::ReflectContext* reflection) const
    {
        EditorMeshColliderComponent::Reflect(reflection);
    }

    void EditorMeshColliderComponentDescriptor::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorMeshColliderComponent::GetProvidedServices(provided);
    }

    void EditorMeshColliderComponentDescriptor::GetDependentServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorMeshColliderComponent::GetDependentServices(dependent);
    }

    void EditorMeshColliderComponentDescriptor::GetRequiredServices(
        AZ::ComponentDescriptor::DependencyArrayType& required, [[maybe_unused]] const AZ::Component* instance) const
    {
        EditorMeshColliderComponent::GetRequiredServices(required);
    }

    void EditorMeshColliderComponentDescriptor::GetWarnings(
        AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const
    {
        const PhysX::EditorMeshColliderComponent* editorMeshColliderComponent =
            azrtti_cast<const PhysX::EditorMeshColliderComponent*>(instance);

        if (editorMeshColliderComponent != nullptr)
        {
            warnings = editorMeshColliderComponent->GetComponentWarnings();
        }
    }
} // namespace PhysX
