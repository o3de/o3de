/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EditorHeightfieldColliderComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/utility/as_const.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/MaterialBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Editor/ColliderComponentMode.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Shape.h>
#include <Source/Utils.h>
#include <System/PhysXSystem.h>

#include <utility>

namespace PhysX
{
    AZ_CVAR(float, physx_heightfieldDebugDrawDistance, 50.0f, nullptr,
        AZ::ConsoleFunctorFlags::Null, "Distance for PhysX Heightfields debug visualization.");
    AZ_CVAR(bool, physx_heightfieldDebugDrawBoundingBox, false,
        nullptr, AZ::ConsoleFunctorFlags::Null, "Draw the bounding box used for heightfield debug visualization.");

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
        // By default, disable heightfield collider debug drawing. This doesn't need to be viewed in the common case.
        m_colliderDebugDraw.SetDisplayFlag(false);

        // Heightfields don't support the following:
        // - Offset:  There shouldn't be a need to offset the data, since the heightfield provider is giving a physics representation
        // - IsTrigger:  PhysX heightfields don't support acting as triggers
        // - MaterialSelection:  The heightfield provider provides per-vertex material selection
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);
    }

    EditorHeightfieldColliderComponent ::~EditorHeightfieldColliderComponent()
    {
        ClearHeightfield();
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

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

        // Create an empty shapeConfig for initializing the component's ShapeConfiguration.
        // The actual shapeConfig for the component will get filled out at runtime as everything initializes,
        // so the values set here during initialization don't matter.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> shapeConfig{ aznew Physics::HeightfieldShapeConfiguration() };

        heightfieldColliderComponent->SetShapeConfiguration(
            { AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), shapeConfig });
    }

    void EditorHeightfieldColliderComponent::OnHeightfieldDataChanged(const AZ::Aabb& dirtyRegion, 
        const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask)
    {
        RefreshHeightfield(changeMask, dirtyRegion);
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
        const AZ::Transform baseTransform = GetWorldTM();
        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = baseTransform.GetRotation();
        configuration.m_position = baseTransform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        // Get the transform from the HeightfieldProvider.  Because rotation and scale can indirectly affect how the heightfield itself
        // is computed and the size of the heightfield, it's possible that the HeightfieldProvider will provide a different transform
        // back to us than the one that's directly on that entity.
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            transform, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

        // Because the heightfield's transform may not match the entity's transform, use the heightfield transform
        // to generate an offset rotation/position from the entity's transform for the collider configuration.
        m_colliderConfig.m_rotation = transform.GetRotation() * baseTransform.GetRotation().GetInverseFull();
        m_colliderConfig.m_position =
            m_colliderConfig.m_rotation.TransformVector(transform.GetTranslation() - baseTransform.GetTranslation());

        // Update material selection from the mapping
        Utils::SetMaterialsFromHeightfieldProvider(GetEntityId(), m_colliderConfig.m_materialSelection);

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

    void EditorHeightfieldColliderComponent::RefreshHeightfield(
        const Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask,
        const AZ::Aabb& dirtyRegion)
    {
        using Physics::HeightfieldProviderNotifications;

        if (HeightfieldChangeMask::DestroyBegin == (changeMask & HeightfieldChangeMask::DestroyBegin) ||
            HeightfieldChangeMask::DestroyEnd == (changeMask & HeightfieldChangeMask::DestroyEnd))
        {
            // Clear the entire terrain if destroying
            ClearHeightfield();
            Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
            return;
        }

        // If the change is only about heightfield materials mapping, we can simply update material selection in the heightfield shape
        if (changeMask == HeightfieldChangeMask::SurfaceMapping)
        {
            Physics::MaterialSelection updatedMaterialSelection;
            Utils::SetMaterialsFromHeightfieldProvider(GetEntityId(), updatedMaterialSelection);

            // Make sure the number of slots is the same.
            // Otherwise the heightfield needs to be rebuilt to support updated indices.
            if (updatedMaterialSelection.GetMaterialIdsAssignedToSlots().size()
                == m_colliderConfig.m_materialSelection.GetMaterialIdsAssignedToSlots().size())
            {
                UpdateHeightfieldMaterialSelection(updatedMaterialSelection);
                return;
            }
        }

        AZ::Aabb heightfieldAabb = GetColliderShapeAabb();
        AZ::Aabb requestRegion = dirtyRegion;

        if (!requestRegion.IsValid())
        {
            requestRegion = heightfieldAabb;
        }

        // Early out if the updated region is outside of the heightfield Aabb
        if (heightfieldAabb.Disjoint(requestRegion))
        {
            return;
        }

        // Clamp requested region to the entire heightfield AABB
        requestRegion.Clamp(heightfieldAabb);

        // if dirty region invalid, recreate the entire heightfield, otherwise request samples
        bool shouldRecreateHeightfield = (m_shapeConfig == nullptr) ||
            (HeightfieldChangeMask::CreateEnd == (changeMask & HeightfieldChangeMask::CreateEnd));

        // Check if dirtyRegion covers the entire terrain
        shouldRecreateHeightfield = shouldRecreateHeightfield || (requestRegion == heightfieldAabb);

        // Check if base configuration parameters have changed
        if (!shouldRecreateHeightfield)
        {
            Physics::HeightfieldShapeConfiguration baseConfiguration = Utils::CreateBaseHeightfieldShapeConfiguration(GetEntityId());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetNumRowVertices() != m_shapeConfig->GetNumRowVertices());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetNumColumnVertices() != m_shapeConfig->GetNumColumnVertices());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetMinHeightBounds() != m_shapeConfig->GetMinHeightBounds());
            shouldRecreateHeightfield = shouldRecreateHeightfield || (baseConfiguration.GetMaxHeightBounds() != m_shapeConfig->GetMaxHeightBounds());
        }

        if (shouldRecreateHeightfield)
        {
            ClearHeightfield();
            InitHeightfieldShapeConfiguration();

            if (!m_shapeConfig->GetSamples().empty())
            {
                InitStaticRigidBody();
            }
        }
        else
        {
            // Update m_shapeConfig
            Physics::HeightfieldProviderRequestsBus::Event(GetEntityId(),
                &Physics::HeightfieldProviderRequestsBus::Events::UpdateHeightsAndMaterials,
                [this](int32_t row, int32_t col, const Physics::HeightMaterialPoint& point)
                {
                    m_shapeConfig->ModifySample(row, col, point);
                },
                requestRegion);

            if (!m_shapeConfig->GetSamples().empty())
            {
                ClearHeightfield();
                InitStaticRigidBody();
            }
        }

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        RefreshHeightfield(Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);
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
    void EditorHeightfieldColliderComponent::Display(const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const AzPhysics::SimulatedBody* simulatedBody = GetSimulatedBody();
        if (!simulatedBody)
        {
            return;
        }

        const AzPhysics::StaticRigidBody* staticRigidBody = azrtti_cast<const AzPhysics::StaticRigidBody*>(simulatedBody);
        if (!staticRigidBody)
        {
            return;
        }

        // Calculate the center of a box in front of the camera - this will be the area to draw
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        const AZ::Vector3 boundsAabbCenter = cameraState.m_position + cameraState.m_forward * physx_heightfieldDebugDrawDistance * 0.5f;

        const AZ::Vector3 bodyPosition = staticRigidBody->GetPosition();
        const AZ::Vector3 aabbCenterLocalBody = boundsAabbCenter - bodyPosition;

        const AZ::u32 shapeCount = staticRigidBody->GetShapeCount();
        for (AZ::u32 shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
        {
            const AZStd::shared_ptr<const Physics::Shape> shape = staticRigidBody->GetShape(shapeIndex);
            m_colliderDebugDraw.DrawHeightfield(debugDisplay, aabbCenterLocalBody,
                physx_heightfieldDebugDrawDistance, shape);
        }

        if (physx_heightfieldDebugDrawBoundingBox)
        {
            const AZ::Aabb boundsAabb = AZ::Aabb::CreateCenterRadius(aabbCenterLocalBody, physx_heightfieldDebugDrawDistance);
            if (boundsAabb.IsValid())
            {
                debugDisplay.DrawWireBox(boundsAabb.GetMin(), boundsAabb.GetMax());
            }
        }
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
        return const_cast<AzPhysics::SimulatedBody*>(AZStd::as_const(*this).GetSimulatedBody());
    }

    const AzPhysics::SimulatedBody* EditorHeightfieldColliderComponent::GetSimulatedBody() const
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

    void EditorHeightfieldColliderComponent::UpdateHeightfieldMaterialSelection(const Physics::MaterialSelection& updatedMaterialSelection)
    {
        AzPhysics::SimulatedBody* simulatedBody = m_sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        if (!simulatedBody)
        {
            return;
        }

        AzPhysics::StaticRigidBody* rigidBody = azdynamic_cast<AzPhysics::StaticRigidBody*>(simulatedBody);

        if (rigidBody->GetShapeCount() != 1)
        {
            AZ_Error("UpdateHeightfieldMaterialSelection",
                rigidBody->GetShapeCount() == 1, "Heightfield collider should have only 1 shape. Count: %d", rigidBody->GetShapeCount());
            return;
        }

        AZStd::shared_ptr<Physics::Shape> shape = rigidBody->GetShape(0);
        PhysX::Shape* physxShape = azdynamic_cast<PhysX::Shape*>(shape.get());

        AZStd::vector<AZStd::shared_ptr<Physics::Material>> materials;

        Physics::PhysicsMaterialRequestBus::Broadcast(
            &Physics::PhysicsMaterialRequestBus::Events::GetMaterials,
            updatedMaterialSelection,
            materials);

        physxShape->SetMaterials(materials);

        m_colliderConfig.m_materialSelection = updatedMaterialSelection;
    }

} // namespace PhysX
