/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/NameConstants.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Editor/Source/Components/EditorSystemComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Editor/InertiaPropertyHandler.h>
#include <Editor/EditorClassConverters.h>
#include <Source/NameConstants.h>
#include <Source/Utils.h>

namespace PhysX
{
    namespace Internal
    {
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetCollisionShapes(AZ::Entity* entity)
        {
            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> allShapes;

            const bool hasNonUniformScaleComponent = (AZ::NonUniformScaleRequestBus::FindFirstHandler(entity->GetId()) != nullptr);

            const AZStd::vector<EditorColliderComponent*> colliders = entity->FindComponents<EditorColliderComponent>();
            for (const EditorColliderComponent* collider : colliders)
            {
                const EditorProxyShapeConfig& shapeConfigurationProxy = collider->GetShapeConfiguration();
                if (shapeConfigurationProxy.IsAssetConfig() && !shapeConfigurationProxy.m_physicsAsset.m_configuration.m_asset.IsReady())
                {
                    continue;
                }

                const Physics::ColliderConfiguration colliderConfigurationScaled = collider->GetColliderConfigurationScaled();
                const Physics::ColliderConfiguration colliderConfigurationUnscaled = collider->GetColliderConfiguration();

                if (shapeConfigurationProxy.IsAssetConfig())
                {
                    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
                    Utils::GetShapesFromAsset(shapeConfigurationProxy.m_physicsAsset.m_configuration,
                        colliderConfigurationUnscaled, hasNonUniformScaleComponent, shapeConfigurationProxy.m_subdivisionLevel, shapes);

                    for (const auto& shape : shapes)
                    {
                        AZ_Assert(shape, "CreateEditorWorldRigidBody: Shape must not be null!");
                        allShapes.emplace_back(shape);
                    }
                }
                else
                {
                    const Physics::ShapeConfiguration& shapeConfiguration = shapeConfigurationProxy.GetCurrent();
                    if (!hasNonUniformScaleComponent)
                    {
                        AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(
                            colliderConfigurationScaled, shapeConfiguration);
                        AZ_Assert(shape, "CreateEditorWorldRigidBody: Shape must not be null!");
                        if (shape)
                        {
                            allShapes.emplace_back(shape);
                        }
                    }
                    else
                    {
                        auto convexConfig = Utils::CreateConvexFromPrimitive(colliderConfigurationUnscaled, shapeConfiguration,
                            shapeConfigurationProxy.m_subdivisionLevel, shapeConfiguration.m_scale);
                        auto colliderConfigurationNoOffset = colliderConfigurationUnscaled;
                        colliderConfigurationNoOffset.m_rotation = AZ::Quaternion::CreateIdentity();
                        colliderConfigurationNoOffset.m_position = AZ::Vector3::CreateZero();

                        if (convexConfig.has_value())
                        {
                            AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(
                                colliderConfigurationNoOffset, convexConfig.value());
                            allShapes.emplace_back(shape);
                        }
                    }
                }
            }

            const AZStd::vector<EditorShapeColliderComponent*> shapeColliders = entity->FindComponents<EditorShapeColliderComponent>();
            for (const EditorShapeColliderComponent* shapeCollider : shapeColliders)
            {
                const Physics::ColliderConfiguration colliderConfig = shapeCollider->GetColliderConfigurationScaled();
                const AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>>& shapeConfigs =
                    shapeCollider->GetShapeConfigurations();
                for (const auto& shapeConfig : shapeConfigs)
                {
                    AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, *shapeConfig);
                    AZ_Assert(shape, "CreateEditorWorldRigidBody: Shape must not be null!");
                    allShapes.emplace_back(shape);
                }
            }

            return allShapes;
        }
    } // namespace Internal

    void EditorRigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidBodyConfiguration, AzPhysics::RigidBodyConfiguration>()
                ->Version(2, &ClassConverters::EditorRigidBodyConfigVersionConverter)
                ->Field("Debug Draw Center of Mass", &EditorRigidBodyConfiguration::m_centerOfMassDebugDraw)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<AzPhysics::RigidBodyConfiguration>(
                    "PhysX Rigid Body Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_initialLinearVelocity,
                        "Initial linear velocity", "Linear velocity applied when the rigid body is activated.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInitialVelocitiesVisibility)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSpeedUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_initialAngularVelocity,
                        "Initial angular velocity", "Angular velocity applied when the rigid body is activated (limited by maximum angular velocity).")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInitialVelocitiesVisibility)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_linearDamping,
                        "Linear damping", "The rate of decay over time for linear velocity even if no forces are acting on the rigid body.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetDampingVisibility)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_angularDamping,
                        "Angular damping", "The rate of decay over time for angular velocity even if no forces are acting on the rigid body.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetDampingVisibility)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_sleepMinEnergy,
                        "Sleep threshold", "The rigid body can go to sleep (settle) when kinetic energy per unit mass is persistently below this value.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetSleepOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSleepThresholdUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_startAsleep,
                        "Start asleep", "When active, the rigid body will be asleep when spawned, and wake when the body is disturbed.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetSleepOptionsVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_interpolateMotion,
                        "Interpolate motion", "When active, simulation results are interpolated resulting in smoother motion.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInterpolationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_gravityEnabled,
                        "Gravity enabled", "When active, global gravity affects this rigid body.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetGravityVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_kinematic,
                        "Kinematic", "When active, the rigid body is not affected by gravity or other forces and is moved by script.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetKinematicVisibility)

                    // Linear axis locking properties
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Linear Axis Locking")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockLinearX, "Lock X",
                        "When active, forces won't create translation on the X axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockLinearY, "Lock Y",
                        "When active, forces won't create translation on the Y axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockLinearZ, "Lock Z",
                        "When active, forces won't create translation on the Z axis of the rigid body.")

                    // Angular axis locking properties
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Angular Axis Locking")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockAngularX, "Lock X",
                        "When active, forces won't create rotation on the X axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockAngularY, "Lock Y",
                        "When active, forces won't create rotation on the Y axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_lockAngularZ, "Lock Z",
                        "When active, forces won't create rotation on the Z axis of the rigid body.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Continuous Collision Detection")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetCCDVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_ccdEnabled,
                        "CCD enabled", "When active, the rigid body has continuous collision detection (CCD). Use this to ensure accurate "
                        "collision detection, particularly for fast moving rigid bodies. CCD must be activated in the global PhysX preferences.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetCCDVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_ccdMinAdvanceCoefficient,
                        "Min advance coefficient", "Lower values reduce clipping but can affect simulation smoothness.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.99f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::IsCCDEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_ccdFrictionEnabled,
                        "CCD friction", "When active, friction is applied when continuous collision detection (CCD) collisions are resolved.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::IsCCDEnabled)
                    ->EndGroup()

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_maxAngularVelocity,
                        "Maximum angular velocity", "Clamp angular velocities to this maximum value. "
                        "This prevents rigid bodies from rotating at unrealistic velocities after collisions.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetMaxVelocitiesVisibility)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())

                    // Mass properties
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_computeCenterOfMass,
                        "Compute COM", "Compute the center of mass (COM) for this rigid body.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_centerOfMassOffset,
                        "COM offset", "Local space offset for the center of mass (COM).")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetCoMVisibility)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetLengthUnit())

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_computeMass,
                        "Compute Mass", "When active, the mass of the rigid body is computed based on the volume and density values of its colliders.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzPhysics::RigidBodyConfiguration::m_mass,
                        "Mass", "The mass of the rigid body in kilograms. A value of 0 is treated as infinite. "
                        "The trajectory of infinite mass bodies cannot be affected by any collisions or forces other than gravity.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetMassUnit())
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetMassVisibility)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_computeInertiaTensor,
                        "Compute inertia", "When active, inertia is computed based on the mass and shape of the rigid body.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(Editor::InertiaHandler, &AzPhysics::RigidBodyConfiguration::m_inertiaTensor,
                        "Inertia diagonal", "Inertia diagonal elements that specify an inertia tensor; determines the "
                        "torque required to rotate the rigid body on each axis.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInertiaVisibility)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetInertiaUnit())

                    ->DataElement(AZ::Edit::UIHandlers::Default, &RigidBodyConfiguration::m_includeAllShapesInMassCalculation,
                        "Include non-simulated shapes in Mass",
                        "When active, non-simulated shapes are included in the center of mass, inertia, and mass calculations.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzPhysics::RigidBodyConfiguration::GetInertiaSettingsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ;

                editContext->Class<EditorRigidBodyConfiguration>(
                    "PhysX Rigid Body Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRigidBodyConfiguration::m_centerOfMassDebugDraw,
                        "Debug draw COM", "Display the rigid body's center of mass (COM) in the viewport.")
                ;
            }
        }
    }

    void EditorRigidBodyComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Physics::ColliderComponentEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle editorSceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
            sceneInterface->RegisterSceneSimulationStartHandler(editorSceneHandle, m_sceneStartSimHandler);
        }

        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this](const AZ::Vector3& scale) {OnNonUniformScaleChanged(scale); });
        AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);

        if (auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
        {
            m_debugDisplayDataChangeHandler = Debug::DebugDisplayDataChangedEvent::Handler(
                [this](const Debug::DebugDisplayData& data)
                {
                    this->UpdateDebugDrawSettings(data);
                });
            physXDebug->RegisterDebugDisplayDataChangedEvent(m_debugDisplayDataChangeHandler);
            UpdateDebugDrawSettings(physXDebug->GetDebugDisplayData());
        }
        CreateEditorWorldRigidBody();

        PhysX::EditorColliderValidationRequestBus::Event(
            GetEntityId(), &PhysX::EditorColliderValidationRequestBus::Events::ValidateRigidBodyMeshGeometryType);

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void EditorRigidBodyComponent::Deactivate()
    {
        m_debugDisplayDataChangeHandler.Disconnect();

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        m_nonUniformScaleChangedHandler.Disconnect();
        m_sceneStartSimHandler.Disconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        Physics::ColliderComponentEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorRigidBodyHandle);
        }
    }

    void EditorRigidBodyComponent::OnConfigurationChanged()
    {
        CreateEditorWorldRigidBody();

        // required in case the kinematic setting has changed
        PhysX::EditorColliderValidationRequestBus::Event(
            GetEntityId(), &PhysX::EditorColliderValidationRequestBus::Events::ValidateRigidBodyMeshGeometryType);
    }

    void EditorRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorRigidBodyConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorRigidBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("Configuration", &EditorRigidBodyComponent::m_config)
                ->Field("PhysXSpecificConfiguration", &EditorRigidBodyComponent::m_physxSpecificConfig)
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorRigidBodyComponent>(
                    "PhysX Rigid Body", "The entity behaves as a movable rigid object in PhysX.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/rigid-body/")
                    ->DataElement(0, &EditorRigidBodyComponent::m_config, "Configuration", "Configuration for rigid body physics.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRigidBodyComponent::OnConfigurationChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorRigidBodyComponent::m_physxSpecificConfig,
                        "PhysX-Specific Configuration",
                        "Settings which are specific to PhysX, rather than generic.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRigidBodyComponent::OnConfigurationChanged)
                    ;
            }
        }
    }

    EditorRigidBodyComponent::EditorRigidBodyComponent()
    {
        InitPhysicsTickHandler();
    }

    EditorRigidBodyComponent::EditorRigidBodyComponent(const EditorRigidBodyConfiguration& config)
        : m_config(config)
    {
        InitPhysicsTickHandler();
    }

    EditorRigidBodyComponent::EditorRigidBodyComponent(
        const EditorRigidBodyConfiguration& configuration, const RigidBodyConfiguration& physxSpecificConfiguration)
        : m_config(configuration)
        , m_physxSpecificConfig(physxSpecificConfiguration)
    {
        InitPhysicsTickHandler();
    }

    void EditorRigidBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // for now use Invalid scene which will fall back on default scene when entity is activated.
        // update to correct scene once multi-scene is fully supported.
        gameEntity->CreateComponent<RigidBodyComponent>(m_config, m_physxSpecificConfig, AzPhysics::InvalidSceneHandle);
    }

    void EditorRigidBodyComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_config.m_centerOfMassDebugDraw)
        {
            if (const AzPhysics::RigidBody* body = GetRigidBody())
            {
                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(m_centerOfMassDebugColor);
                debugDisplay.DrawBall(body->GetCenterOfMassWorld(), m_centerOfMassDebugSize);
                debugDisplay.DepthTestOn();
            }
        }
    }

    void EditorRigidBodyComponent::CreateEditorWorldRigidBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_editorSceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
            if (m_editorSceneHandle == AzPhysics::InvalidSceneHandle)
            {
                AZ_Assert(false, "Attempting to create an edit time rigid body without an editor scene.");
                return;
            }
        }

        AZ::Transform colliderTransform = GetWorldTM();
        colliderTransform.ExtractUniformScale();

        AzPhysics::RigidBodyConfiguration configuration = m_config;
        configuration.m_orientation = colliderTransform.GetRotation();
        configuration.m_position = colliderTransform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();
        configuration.m_startSimulationEnabled = false;
        configuration.m_colliderAndShapeData = Internal::GetCollisionShapes(GetEntity());

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_editorRigidBodyHandle = sceneInterface->AddSimulatedBody(m_editorSceneHandle, &configuration);
            if (auto* body = azdynamic_cast<AzPhysics::RigidBody*>(
                sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorRigidBodyHandle)
                ))
            {
                // AddSimulatedBody may update mass / CoM / Inertia tensor based on the config, so grab the updated values.
                m_config.m_mass = body->GetMass();
                m_config.m_centerOfMassOffset = body->GetCenterOfMassLocal();
                m_config.m_inertiaTensor = body->GetInertiaLocal();
            }
        }
        AZ_Error("EditorRigidBodyComponent",
            m_editorRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle, "Failed to create editor rigid body");
    }

    void EditorRigidBodyComponent::OnColliderChanged()
    {
        //recreate the rigid body when collider changes
        SetShouldBeRecreated();
    }

    void EditorRigidBodyComponent::OnTransformChanged([[maybe_unused]]const AZ::Transform& local, [[maybe_unused]]const AZ::Transform& world)
    {
        SetShouldBeRecreated();
    }

    void EditorRigidBodyComponent::OnNonUniformScaleChanged([[maybe_unused]] const AZ::Vector3& scale)
    {
        SetShouldBeRecreated();
    }

    void EditorRigidBodyComponent::InitPhysicsTickHandler()
    {
        m_sceneStartSimHandler = AzPhysics::SceneEvents::OnSceneSimulationStartHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] float fixedDeltatime
            )
            {
                this->PrePhysicsTick();
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components));
    }

    void EditorRigidBodyComponent::PrePhysicsTick()
    {
        if (m_shouldBeRecreated)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorRigidBodyHandle);

                CreateEditorWorldRigidBody();
            }
            m_shouldBeRecreated = false;
        }
    }

    void EditorRigidBodyComponent::EnablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_editorSceneHandle, m_editorRigidBodyHandle);
        }
    }

    void EditorRigidBodyComponent::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_editorSceneHandle, m_editorRigidBodyHandle);
        }
    }

    bool EditorRigidBodyComponent::IsPhysicsEnabled() const
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body =
                sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorRigidBodyHandle))
            {
                return body->m_simulating;
            }
        }
        return false;
    }

    AZ::Aabb EditorRigidBodyComponent::GetAabb() const
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body =
                sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* EditorRigidBodyComponent::GetSimulatedBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorRigidBodyHandle);
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyHandle EditorRigidBodyComponent::GetSimulatedBodyHandle() const
    {
        return m_editorRigidBodyHandle;
    }

    AzPhysics::SceneQueryHit EditorRigidBodyComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (AzPhysics::SimulatedBody* body = GetSimulatedBody())
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    void EditorRigidBodyComponent::UpdateDebugDrawSettings(const Debug::DebugDisplayData& data)
    {
        m_centerOfMassDebugColor = data.m_centerOfMassDebugColor;
        m_centerOfMassDebugSize = data.m_centerOfMassDebugSize;
    }

    const AzPhysics::RigidBody* EditorRigidBodyComponent::GetRigidBody() const
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return azdynamic_cast<AzPhysics::RigidBody*>(
                sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorRigidBodyHandle));
        }
        return nullptr;
    }

    void EditorRigidBodyComponent::SetShouldBeRecreated()
    {
        if (!m_shouldBeRecreated)
        {
            m_shouldBeRecreated = true;
        }
    }

    AZ::Aabb EditorRigidBodyComponent::GetWorldBounds()
    {
        return GetAabb();
    }

    AZ::Aabb EditorRigidBodyComponent::GetLocalBounds()
    {
        AZ::Aabb worldBounds = GetWorldBounds();
        if (worldBounds.IsValid())
        {
            return worldBounds.GetTransformedAabb(GetWorldTM().GetInverse());
        }

        return AZ::Aabb::CreateNull();
    }
} // namespace PhysX
