/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <Editor/ColliderComponentMode.h>
#include <EditorHeightfieldColliderComponent.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Utils.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHeightfieldColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorHeightfieldColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorHeightfieldColliderComponent::m_colliderDebugDraw)
                ->Field("ShapeConfig", &EditorHeightfieldColliderComponent::m_shapeConfig)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorHeightfieldColliderComponent>(
                    "PhysX Heightfield Collider", "Creates geometry in the PhysX simulation based on an attached heightfield component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXHeightfieldCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PhysXHeightfieldCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/heightfield-collider/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_colliderConfig, "Collider configuration",
                        "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHeightfieldColliderComponent::OnConfigurationChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_colliderDebugDraw, "Debug draw settings",
                        "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorHeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorHeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXColliderService"));
        incompatible.push_back(AZ_CRC_CE("PhysXStaticRigidBodyService"));
        incompatible.push_back(AZ_CRC_CE("PhysXRigidBodyService"));
    }

    EditorHeightfieldColliderComponent::EditorHeightfieldColliderComponent()
        : m_physXConfigChangedHandler(
              []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
              {
                  AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                      &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                      AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
              })
        , m_onMaterialLibraryChangedEventHandler(
              [this](const AZ::Data::AssetId& defaultMaterialLibrary)
              {
                  m_colliderConfig.m_materialSelection.OnMaterialLibraryChanged(defaultMaterialLibrary);
                  Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);

                  AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                      &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                      AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
              })
    {
    }

    EditorHeightfieldColliderComponent ::~EditorHeightfieldColliderComponent()
    {
        ClearHeightfield();
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        // Heightfields don't support the following:
        // - Offset:  There shouldn't be a need to offset the data, since the heightfield provider is giving a physics representation
        // - IsTrigger:  PhysX heightfields don't support acting as triggers
        // - MaterialSelection:  The heightfield provider provides per-vertex material selection
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);

        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_attachedSceneHandle = m_sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(entityId);

        // Debug drawing
        m_colliderDebugDraw.Connect(entityId);
        m_colliderDebugDraw.SetDisplayCallback(this);

        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(entityId);
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(entityId);
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(entityId);

        RefreshHeightfield();
    }

    void EditorHeightfieldColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();
        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();

        m_colliderDebugDraw.Disconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        ClearHeightfield();
    }

    void EditorHeightfieldColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* heightfieldColliderComponent = gameEntity->CreateComponent<HeightfieldColliderComponent>();
        heightfieldColliderComponent->SetShapeConfiguration(
            { AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), m_shapeConfig });
    }

    void EditorHeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void EditorHeightfieldColliderComponent::ClearHeightfield()
    {
        // There are two references to the heightfield data, we need to clear both to make the heightfield clear out and deallocate:
        // - The simulated body has a pointer to the shape, which has a GeometryHolder, which has the Heightfield inside it
        // - The shape config is also holding onto a pointer to the Heightfield

        // We remove the simulated body first, since we don't want the heightfield to exist any more.
        if (m_sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        // Now we can safely clear out the cached heightfield pointer.
        m_shapeConfig->SetCachedNativeHeightfield(nullptr);
    }

    void EditorHeightfieldColliderComponent::InitStaticRigidBody()
    {
        // Get the transform from the HeightfieldProvider.  Because rotation and scale can indirectly affect how the heightfield itself
        // is computed and the size of the heightfield, it's possible that the HeightfieldProvider will provide a different transform
        // back to us than the one that's directly on that entity.
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            transform, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        AzPhysics::ShapeColliderPairList colliderShapePairs;
        colliderShapePairs.emplace_back(AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), m_shapeConfig);
        configuration.m_colliderAndShapeData = colliderShapePairs;

        if (m_sceneInterface)
        {
            m_staticRigidBodyHandle = m_sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }
    }

    void EditorHeightfieldColliderComponent::InitHeightfieldShapeConfiguration()
    {
        *m_shapeConfig = Utils::CreateHeightfieldShapeConfiguration(GetEntityId());
    }

    void EditorHeightfieldColliderComponent::RefreshHeightfield()
    {
        ClearHeightfield();
        InitHeightfieldShapeConfiguration();
        InitStaticRigidBody();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        RefreshHeightfield();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (!m_physXConfigChangedHandler.IsConnected())
            {
                physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
            }
            if (!m_onMaterialLibraryChangedEventHandler.IsConnected())
            {
                physXSystem->RegisterOnMaterialLibraryChangedEventHandler(m_onMaterialLibraryChangedEventHandler);
            }
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnDeselected()
    {
        m_onMaterialLibraryChangedEventHandler.Disconnect();
        m_physXConfigChangedHandler.Disconnect();
    }

    // DisplayCallback
    void EditorHeightfieldColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const auto& heightfieldConfig = static_cast<const Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig);
        m_colliderDebugDraw.DrawHeightfield(debugDisplay, m_colliderConfig, heightfieldConfig);
    }

    // SimulatedBodyComponentRequestsBus
    void EditorHeightfieldColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled() && m_sceneInterface)
        {
            m_sceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    // SimulatedBodyComponentRequestsBus
    void EditorHeightfieldColliderComponent::DisablePhysics()
    {
        if (m_sceneInterface)
        {
            m_sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    // SimulatedBodyComponentRequestsBus
    bool EditorHeightfieldColliderComponent::IsPhysicsEnabled() const
    {
        if (m_sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->m_simulating;
            }
        }
        return false;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBodyHandle EditorHeightfieldColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_staticRigidBodyHandle;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBody* EditorHeightfieldColliderComponent::GetSimulatedBody()
    {
        if (m_sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body;
            }
        }
        return nullptr;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SceneQueryHit EditorHeightfieldColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->RayCast(request);
            }
        }
        return AzPhysics::SceneQueryHit();
    }

    // ColliderShapeRequestBus
    AZ::Aabb EditorHeightfieldColliderComponent::GetColliderShapeAabb()
    {
        // Get the Collider AABB directly from the heightfield provider.
        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            colliderAabb, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb);

        return colliderAabb;
    }

    // SimulatedBodyComponentRequestsBus
    AZ::Aabb EditorHeightfieldColliderComponent::GetAabb() const
    {
        // On the SimulatedBodyComponentRequestsBus, get the AABB from the simulated body instead of the collider.
        if (m_sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

} // namespace PhysX
