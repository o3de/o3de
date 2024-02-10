/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PxPhysicsAPI.h>
#include <Source/RigidBody.h>
#include <Source/RigidBodyComponent.h>
#include <Source/Shape.h>

namespace PhysX
{
    class BehaviorRigidBodyNotificationBusHandler final
        : public Physics::RigidBodyNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorRigidBodyNotificationBusHandler, "{7F3BD6F6-4F84-49BB-8DEC-471272965A5F}", AZ::SystemAllocator,
            OnPhysicsEnabled,
            OnPhysicsDisabled
        );

        void OnPhysicsEnabled(const AZ::EntityId& entityId) override
        {
            Call(FN_OnPhysicsEnabled, entityId);
        }

        void OnPhysicsDisabled(const AZ::EntityId& entityId) override
        {
            Call(FN_OnPhysicsDisabled, entityId);
        }
    };

    void RigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        RigidBodyConfiguration::Reflect(context);
        RigidBody::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RigidBodyComponent, AZ::Component>()
                ->Version(1)
                ->Field("RigidBodyConfiguration", &RigidBodyComponent::m_configuration)
                ->Field("PhysXSpecificConfiguration", &RigidBodyComponent::m_physxSpecificConfiguration)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Physics::RigidBodyRequestBus>("RigidBodyRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Event("EnablePhysics", &Physics::RigidBodyRequests::EnablePhysics)
                ->Event("DisablePhysics", &RigidBodyRequests::DisablePhysics)
                ->Event("IsPhysicsEnabled", &RigidBodyRequests::IsPhysicsEnabled)
                ->Event("GetCenterOfMassWorld", &RigidBodyRequests::GetCenterOfMassWorld)
                ->Event("GetCenterOfMassLocal", &RigidBodyRequests::GetCenterOfMassLocal)
                ->Event("GetMass", &RigidBodyRequests::GetMass)
                ->Event("GetInverseMass", &RigidBodyRequests::GetInverseMass)
                ->Event("SetMass", &RigidBodyRequests::SetMass)
                ->Event("SetCenterOfMassOffset", &RigidBodyRequests::SetCenterOfMassOffset)

                ->Event("GetLinearVelocity", &RigidBodyRequests::GetLinearVelocity)
                ->Event("SetLinearVelocity", &RigidBodyRequests::SetLinearVelocity)
                ->Event("GetAngularVelocity", &RigidBodyRequests::GetAngularVelocity)
                ->Event("SetAngularVelocity", &RigidBodyRequests::SetAngularVelocity)

                ->Event("GetLinearVelocityAtWorldPoint", &RigidBodyRequests::GetLinearVelocityAtWorldPoint)
                ->Event("ApplyLinearImpulse", &RigidBodyRequests::ApplyLinearImpulse)
                ->Event("ApplyLinearImpulseAtWorldPoint", &RigidBodyRequests::ApplyLinearImpulseAtWorldPoint)
                ->Event("ApplyAngularImpulse", &RigidBodyRequests::ApplyAngularImpulse)

                ->Event("GetLinearDamping", &RigidBodyRequests::GetLinearDamping)
                ->Event("SetLinearDamping", &RigidBodyRequests::SetLinearDamping)
                ->Event("GetAngularDamping", &RigidBodyRequests::GetAngularDamping)
                ->Event("SetAngularDamping", &RigidBodyRequests::SetAngularDamping)

                ->Event("IsAwake", &RigidBodyRequests::IsAwake)
                ->Event("ForceAsleep", &RigidBodyRequests::ForceAsleep)
                ->Event("ForceAwake", &RigidBodyRequests::ForceAwake)
                ->Event("GetSleepThreshold", &RigidBodyRequests::GetSleepThreshold)
                ->Event("SetSleepThreshold", &RigidBodyRequests::SetSleepThreshold)

                ->Event("IsKinematic", &RigidBodyRequests::IsKinematic)
                ->Event("SetKinematic", &RigidBodyRequests::SetKinematic)
                ->Event("SetKinematicTarget", &RigidBodyRequests::SetKinematicTarget)

                ->Event("IsGravityEnabled", &RigidBodyRequests::IsGravityEnabled)
                ->Event("SetGravityEnabled", &RigidBodyRequests::SetGravityEnabled)
                ->Event("SetSimulationEnabled", &RigidBodyRequests::SetSimulationEnabled)

                ->Event("GetAabb", &RigidBodyRequests::GetAabb)
            ;

            behaviorContext->Class<RigidBodyComponent>()->RequestBus("RigidBodyRequestBus");

            behaviorContext->EBus<Physics::RigidBodyNotificationBus>("RigidBodyNotificationBus")
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Handler<BehaviorRigidBodyNotificationBusHandler>()
                ;
        }
    }

    RigidBodyComponent::RigidBodyComponent()
    {
        InitPhysicsTickHandler();
    }

    RigidBodyComponent::RigidBodyComponent(const AzPhysics::RigidBodyConfiguration& config, AzPhysics::SceneHandle sceneHandle)
        : m_configuration(config)
        , m_attachedSceneHandle(sceneHandle)
    {
        InitPhysicsTickHandler();
    }

    RigidBodyComponent::RigidBodyComponent(
        const AzPhysics::RigidBodyConfiguration& baseConfig,
        const RigidBodyConfiguration& physxSpecificConfig,
        AzPhysics::SceneHandle sceneHandle)
        : m_configuration(baseConfig)
        , m_physxSpecificConfiguration(physxSpecificConfig)
        , m_attachedSceneHandle(sceneHandle)
    {
        InitPhysicsTickHandler();
    }

    void RigidBodyComponent::Init()
    {
    }

    void RigidBodyComponent::SetupConfiguration()
    {
        // Get necessary information from the components and fill up the configuration structure
        auto entityId = GetEntityId();

        AZ::Transform lyTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(lyTransform, entityId, &AZ::TransformInterface::GetWorldTM);
        m_configuration.m_position = lyTransform.GetTranslation();
        m_configuration.m_orientation = lyTransform.GetRotation();
        m_configuration.m_entityId = entityId;
        m_configuration.m_debugName = GetEntity()->GetName();
    }

    void RigidBodyComponent::Activate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }

        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            // Early out if there's no relevant physics world present.
            // It may be a valid case when we have game-time components assigned to editor entities via a script
            // so no need to print a warning here.
            return;
        }

        m_cachedSceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AZ::TransformBus::EventResult(m_staticTransformAtActivation, GetEntityId(), &AZ::TransformInterface::IsStaticTransform);

        if (m_staticTransformAtActivation)
        {
            AZ_Warning("RigidBodyComponent", false, "It is not valid to have a PhysX Dynamic Rigid Body Component "
                "when the Transform Component is marked static.  Entity \"%s\" will behave as a static rigid body.",
                GetEntity()->GetName().c_str());

            // If we never connect to the rigid body request bus, then that bus will have no handler and we will behave
            // as if there were no rigid body component, i.e. a static rigid body will be created, which is the behaviour
            // we want if the transform component static checkbox is ticked.
            return;
        }

        // During activation all the collider components will create their physics shapes.
        // Delaying the creation of the rigid body to OnEntityActivated so all the shapes are ready.
        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void RigidBodyComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        // Create and setup rigid body & associated bus handlers
        CreateRigidBody();

        // Add to world
        EnablePhysics();
    }

    void RigidBodyComponent::Deactivate()
    {
        if (m_staticTransformAtActivation || m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        DisablePhysics();

        DestroyRigidBody();

        AZ::EntityBus::Handler::BusDisconnect();
    }

    void RigidBodyComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*currentTime*/)
    {
        if (m_configuration.m_interpolateMotion)
        {
            if (AZ::TransformInterface* entityTransform = GetEntity()->GetTransform())
            {
                AZ::Vector3 newPosition = AZ::Vector3::CreateZero();
                AZ::Quaternion newRotation = AZ::Quaternion::CreateIdentity();
                m_interpolator->GetInterpolated(newPosition, newRotation, deltaTime);
            
                AZ::Transform newWorldTransform = entityTransform->GetWorldTM();
                newWorldTransform.SetRotation(newRotation);
                newWorldTransform.SetTranslation(newPosition);
                entityTransform->SetWorldTM(newWorldTransform);
            }
        }
    }

    int RigidBodyComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_PHYSICS;
    }

    void RigidBodyComponent::InitPhysicsTickHandler()
    {
        m_sceneFinishSimHandler = AzPhysics::SceneEvents::OnSceneSimulationFinishHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            float fixedDeltatime
            )
            {
                PostPhysicsTick(fixedDeltatime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));

        m_activeBodySyncTransformHandler = AzPhysics::SimulatedBodyEvents::OnSyncTransform::Handler(
            [this](float fixedDeltatime)
            {
                PostPhysicsTick(fixedDeltatime);
            });
    }

    void RigidBodyComponent::PostPhysicsTick(float fixedDeltaTime)
    {
        // When transform changes, Kinematic Target is updated with the new transform, so don't set the transform again.
        // But in the case of setting the Kinematic Target directly, the transform needs to reflect the new kinematic target
        //    User sets kinematic Target ---> Update transform
        //    User sets transform        ---> Update kinematic target

        if (!IsPhysicsEnabled() || (IsKinematic() && !m_isLastMovementFromKinematicSource))
        {
            return;
        }

        if (m_cachedSceneInterface == nullptr)
        {
            AZ_Error("RigidBodyComponent", false, "PostPhysicsTick, SceneInterface is null");
            return;
        }

        AzPhysics::SimulatedBody* rigidBody = m_cachedSceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle);
        if (rigidBody == nullptr)
        {
            AZ_Error("RigidBodyComponent", false, "Unable to retrieve simulated rigid body");
            return;
        }
        
        AZ::Transform transform = rigidBody->GetTransform();
        if (m_configuration.m_interpolateMotion)
        {
            m_interpolator->SetTarget(transform.GetTranslation(), rigidBody->GetOrientation(), fixedDeltaTime);
        }
        else if (AZ::TransformInterface* entityTransform = GetEntity()->GetTransform())
        {
            AZ::Transform newWorldTransform = entityTransform->GetWorldTM();
            newWorldTransform.SetRotation(rigidBody->GetOrientation());
            newWorldTransform.SetTranslation(rigidBody->GetPosition());
            entityTransform->SetWorldTM(newWorldTransform);
        }
        m_isLastMovementFromKinematicSource = false;
    }

    void RigidBodyComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // Note: OnTransformChanged is not safe at the moment due to TransformComponent design flaw.
        // It is called when the parent entity is activated after the children causing rigid body
        // to move through the level instantly.
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            if (body->m_simulating &&
                (body->IsKinematic() && !m_isLastMovementFromKinematicSource))
            {
                body->SetKinematicTarget(world);
            }
            else if (!body->m_simulating)
            {
                m_rigidBodyTransformNeedsUpdateOnPhysReEnable = true;
            }
        }
    }

    void RigidBodyComponent::CreateRigidBody()
    {
        BodyConfigurationComponentBus::EventResult(m_configuration, GetEntityId(), &BodyConfigurationComponentRequests::GetRigidBodyConfiguration);

        // Create rigid body
        SetupConfiguration();
        // Add shapes
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
        ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&shapes](ColliderComponentRequests* handler)
            {
                AZStd::vector<AZStd::shared_ptr<Physics::Shape>> newShapes = handler->GetShapes();
                shapes.insert(shapes.end(), newShapes.begin(), newShapes.end());
                return true;
            });
        m_configuration.m_colliderAndShapeData = shapes;

        if (m_cachedSceneInterface != nullptr)
        {
            m_configuration.m_startSimulationEnabled = false; //enable physics will enable this when called.
            m_rigidBodyHandle = m_cachedSceneInterface->AddSimulatedBody(m_attachedSceneHandle, &m_configuration);
            ApplyPhysxSpecificConfiguration();

            // Listen to the PhysX system for events concerning this entity.
            AzPhysics::Scene* scene = m_cachedSceneInterface->GetScene(m_attachedSceneHandle);

            if (scene && scene->GetConfiguration().m_enableActiveActors)
            {
                AzPhysics::SimulatedBody* body =
                    m_cachedSceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle);
                body->RegisterOnSyncTransformHandler(m_activeBodySyncTransformHandler);
            }
            else
            {
                m_cachedSceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);
            }
        }

        if (m_configuration.m_interpolateMotion)
        {
            AZ::TickBus::Handler::BusConnect();
        }

        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        Physics::RigidBodyRequestBus::Handler::BusConnect(GetEntityId());
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void RigidBodyComponent::DestroyRigidBody()
    {
        if (m_cachedSceneInterface)
        {
            m_cachedSceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_rigidBodyHandle);
            m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }

        Physics::RigidBodyRequestBus::Handler::BusDisconnect();
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        m_sceneFinishSimHandler.Disconnect();
        m_activeBodySyncTransformHandler.Disconnect();
        AZ::TickBus::Handler::BusDisconnect();

        m_isLastMovementFromKinematicSource = false;
        m_rigidBodyTransformNeedsUpdateOnPhysReEnable = false;
    }

    void RigidBodyComponent::ApplyPhysxSpecificConfiguration()
    {
        if (auto* body = GetRigidBody())
        {
            if (auto* pxRigidDynamic = static_cast<physx::PxRigidDynamic*>(body->GetNativePointer()))
            {
                const AZ::u8 solverPositionIterations =
                    AZ::GetClamp<AZ::u8>(m_physxSpecificConfiguration.m_solverPositionIterations, 1, 255);
                const AZ::u8 solverVelocityIterations =
                    AZ::GetClamp<AZ::u8>(m_physxSpecificConfiguration.m_solverVelocityIterations, 1, 255);
                pxRigidDynamic->setSolverIterationCounts(solverPositionIterations, solverVelocityIterations);
            }
        }
    }

    void RigidBodyComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }

        if (m_cachedSceneInterface == nullptr)
        {
            AZ_Error("RigidBodyComponent", false, "Unable to enable physics, SceneInterface is null");
            return;
        }
        SetSimulationEnabled(true);

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        if (m_rigidBodyTransformNeedsUpdateOnPhysReEnable)
        {
            if (AzPhysics::SimulatedBody* body =
                    m_cachedSceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle))
            {
                body->SetTransform(transform);
            }
            m_rigidBodyTransformNeedsUpdateOnPhysReEnable = false;
        }

        AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
        AZ::TransformBus::EventResult(rotation, GetEntityId(), &AZ::TransformInterface::GetWorldRotationQuaternion);

        m_interpolator = std::make_unique<TransformForwardTimeInterpolator>();
        m_interpolator->Reset(transform.GetTranslation(), rotation);

        // set the transform to not update when the parent's transform changes, to avoid conflict with physics transform updates
        GetEntity()->GetTransform()->SetOnParentChangedBehavior(AZ::OnParentChangedBehavior::DoNotUpdate);

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsEnabled, GetEntityId());
    }

    void RigidBodyComponent::DisablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        SetSimulationEnabled(false);

        // set the behavior when the parent's transform changes back to default, since physics is no longer controlling the transform
        GetEntity()->GetTransform()->SetOnParentChangedBehavior(AZ::OnParentChangedBehavior::Update);

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsDisabled, GetEntityId());
    }

    bool RigidBodyComponent::IsPhysicsEnabled() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->m_simulating;
        }
        return false;
    }

    void RigidBodyComponent::ApplyLinearImpulse(const AZ::Vector3& impulse)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->ApplyLinearImpulse(impulse);
        }
    }

    void RigidBodyComponent::ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->ApplyLinearImpulseAtWorldPoint(impulse, worldSpacePoint);
        }
    }

    void RigidBodyComponent::ApplyAngularImpulse(const AZ::Vector3& impulse)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->ApplyAngularImpulse(impulse);
        }
    }

    AZ::Vector3 RigidBodyComponent::GetLinearVelocity() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetLinearVelocity();
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBodyComponent::SetLinearVelocity(const AZ::Vector3& velocity)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetLinearVelocity(velocity);
        }
    }

    AZ::Vector3 RigidBodyComponent::GetAngularVelocity() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetAngularVelocity();
        }
        return AZ::Vector3::CreateZero();
    }

    void RigidBodyComponent::SetAngularVelocity(const AZ::Vector3& angularVelocity)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetAngularVelocity(angularVelocity);
        }
    }

    AZ::Vector3 RigidBodyComponent::GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetLinearVelocityAtWorldPoint(worldPoint);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 RigidBodyComponent::GetCenterOfMassWorld() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetCenterOfMassWorld();
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 RigidBodyComponent::GetCenterOfMassLocal() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetCenterOfMassLocal();
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInertiaWorld() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetInertiaWorld();
        }
        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInertiaLocal() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetInertiaLocal();
        }
        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInverseInertiaWorld() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetInverseInertiaWorld();
        }
        return AZ::Matrix3x3::CreateZero();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInverseInertiaLocal() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetInverseInertiaLocal();
        }
        return AZ::Matrix3x3::CreateZero();
    }

    float RigidBodyComponent::GetMass() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetMass();
        }
        return 0.0f;
    }

    float RigidBodyComponent::GetInverseMass() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetInverseMass();
        }
        return 0.0f;
    }

    void RigidBodyComponent::SetMass(float mass)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetMass(mass);
        }
    }

    void RigidBodyComponent::SetCenterOfMassOffset(const AZ::Vector3& comOffset)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetCenterOfMassOffset(comOffset);
        }
    }

    float RigidBodyComponent::GetLinearDamping() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetLinearDamping();
        }
        return 0.0f;
    }

    void RigidBodyComponent::SetLinearDamping(float damping)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetLinearDamping(damping);
        }
    }

    float RigidBodyComponent::GetAngularDamping() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetAngularDamping();
        }
        return 0.0f;
    }

    void RigidBodyComponent::SetAngularDamping(float damping)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetAngularDamping(damping);
        }
    }

    bool RigidBodyComponent::IsAwake() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->IsAwake();
        }
        return false;
    }

    void RigidBodyComponent::ForceAsleep()
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->ForceAsleep();
        }
    }

    void RigidBodyComponent::ForceAwake()
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->ForceAwake();
        }
    }

    bool RigidBodyComponent::IsKinematic() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->IsKinematic();
        }
        return false;
    }

    void RigidBodyComponent::SetKinematic(bool kinematic)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetKinematic(kinematic);
        }
    }

    void RigidBodyComponent::SetKinematicTarget(const AZ::Transform& targetPosition)
    {
        m_isLastMovementFromKinematicSource = true;
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetKinematicTarget(targetPosition);
        }
    }

    bool RigidBodyComponent::IsGravityEnabled() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->IsGravityEnabled();
        }
        return false;
    }

    void RigidBodyComponent::SetGravityEnabled(bool enabled)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetGravityEnabled(enabled);
        }
    }

    void RigidBodyComponent::SetSimulationEnabled(bool enabled)
    {
        if (m_cachedSceneInterface)
        {
            if (enabled)
            {
                m_cachedSceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_rigidBodyHandle);
            }
            else
            {
                m_cachedSceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_rigidBodyHandle);
            }
        }
    }

    float RigidBodyComponent::GetSleepThreshold() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetSleepThreshold();
        }
        return 0.0f;
    }

    void RigidBodyComponent::SetSleepThreshold(float threshold)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            body->SetSleepThreshold(threshold);
        }
    }

    AZ::Aabb RigidBodyComponent::GetAabb() const
    {
        if (const AzPhysics::RigidBody* body = GetRigidBodyConst())
        {
            return body->GetAabb();
        }
        return  AZ::Aabb::CreateNull();
    }

    AzPhysics::RigidBody* RigidBodyComponent::GetRigidBody()
    {
        return static_cast<AzPhysics::RigidBody*>(GetSimulatedBody());
    }

    AzPhysics::SimulatedBody* RigidBodyComponent::GetSimulatedBody()
    {
        if (m_cachedSceneInterface)
        {
            return m_cachedSceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle);
        }
        return nullptr;
    }

    const AzPhysics::RigidBody* RigidBodyComponent::GetRigidBodyConst() const
    {
        if (m_cachedSceneInterface)
        {
            return static_cast<AzPhysics::RigidBody*>(
                m_cachedSceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle));
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyHandle RigidBodyComponent::GetSimulatedBodyHandle() const
    {
        return m_rigidBodyHandle;
    }

    AzPhysics::SceneQueryHit RigidBodyComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (AzPhysics::RigidBody* body = GetRigidBody())
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    void TransformForwardTimeInterpolator::Reset(const AZ::Vector3& position, const AZ::Quaternion& rotation)
    {
        m_currentRealTime = m_currentFixedTime = 0.0f;
        m_integralTime = 0;

        m_targetTranslation = AZ::LinearlyInterpolatedSample<AZ::Vector3>();
        m_targetRotation = AZ::LinearlyInterpolatedSample<AZ::Quaternion>();

        m_targetTranslation.SetNewTarget(position, 1);
        m_targetTranslation.GetInterpolatedValue(1);

        m_targetRotation.SetNewTarget(rotation, 1);
        m_targetRotation.GetInterpolatedValue(1);
    }

    void TransformForwardTimeInterpolator::SetTarget(const AZ::Vector3& position, const AZ::Quaternion& rotation, float fixedDeltaTime)
    {
        m_currentFixedTime += fixedDeltaTime;
        AZ::u32 currentIntegral = FloatToIntegralTime(m_currentFixedTime + fixedDeltaTime * 2.0f);

        m_targetTranslation.SetNewTarget(position, currentIntegral);
        m_targetRotation.SetNewTarget(rotation, currentIntegral);

        static const float resetTimeThreshold = 1.0f;

        if (m_currentFixedTime > resetTimeThreshold)
        {
            m_currentFixedTime -= resetTimeThreshold;
            m_currentRealTime -= resetTimeThreshold;
            m_integralTime += static_cast<AZ::u32>(FloatToIntegralResolution * resetTimeThreshold);
        }
    }

    void TransformForwardTimeInterpolator::GetInterpolated(AZ::Vector3& position, AZ::Quaternion& rotation, float realDeltaTime)
    {
        m_currentRealTime += realDeltaTime;

        AZ::u32 currentIntegral = FloatToIntegralTime(m_currentRealTime);

        position = m_targetTranslation.GetInterpolatedValue(currentIntegral);
        rotation = m_targetRotation.GetInterpolatedValue(currentIntegral);
    }
} // namespace PhysX
