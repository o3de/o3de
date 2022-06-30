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
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
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
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsHeightfieldColliderService"));
    }

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorHeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsColliderService"));
        incompatible.push_back(AZ_CRC_CE("PhysicsStaticRigidBodyService"));
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    EditorHeightfieldColliderComponent::EditorHeightfieldColliderComponent()
        : m_physXConfigChangedHandler(
              []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
              {
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
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);
    }

    EditorHeightfieldColliderComponent ::~EditorHeightfieldColliderComponent()
    {
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {

        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        if (auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        m_heightfieldCollider = AZStd::make_unique<HeightfieldCollider>(
            GetEntityId(), GetEntity()->GetName(), sceneHandle, m_colliderConfig, m_shapeConfig);

        AzToolsFramework::Components::EditorComponentBase::Activate();

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(entityId);

        // Debug drawing
        m_colliderDebugDraw.Connect(entityId);
        m_colliderDebugDraw.SetDisplayCallback(this);

    }

    void EditorHeightfieldColliderComponent::Deactivate()
    {
        m_colliderDebugDraw.Disconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_heightfieldCollider.reset();
    }

    void EditorHeightfieldColliderComponent::BlockOnPendingJobs()
    {
        if (m_heightfieldCollider)
        {
            m_heightfieldCollider->BlockOnPendingJobs();
        }
    }

    void EditorHeightfieldColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* heightfieldColliderComponent = gameEntity->CreateComponent<HeightfieldColliderComponent>();
        heightfieldColliderComponent->SetColliderConfiguration(*m_colliderConfig);
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        m_heightfieldCollider->RefreshHeightfield(Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);
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
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnDeselected()
    {
        m_physXConfigChangedHandler.Disconnect();
    }

    // DisplayCallback
    void EditorHeightfieldColliderComponent::Display(const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const AzPhysics::SimulatedBody* simulatedBody = m_heightfieldCollider->GetSimulatedBody();
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

} // namespace PhysX
