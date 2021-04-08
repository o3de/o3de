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

#include <PhysX_precompiled.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/MathConversion.h>
#include <Source/RigidBodyComponent.h>
#include <Source/Shape.h>
#include <Source/RigidBody.h>

namespace PhysX
{


    void RigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        RigidBody::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RigidBodyComponent, AZ::Component>()
                ->Version(1)
                ->Field("RigidBodyConfiguration", &RigidBodyComponent::m_configuration)
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
        
        AZ::TransformBus::EventResult(m_staticTransformAtActivation, GetEntityId(), &AZ::TransformInterface::IsStaticTransform);

        if (m_staticTransformAtActivation)
        {
            AZ_Warning("PhysX Rigid Body Component", false, "It is not valid to have a PhysX Rigid Body Component "
                "when the Transform Component is marked static.  Entity \"%s\" will behave as a static rigid body.",
                GetEntity()->GetName().c_str());

            // If we never connect to the rigid body request bus, then that bus will have no handler and we will behave
            // as if there were no rigid body component, i.e. a static rigid body will be created, which is the behaviour
            // we want if the transform component static checkbox is ticked.
            return;
        }


        AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::GameEntityContextRequestBus::BroadcastResult(gameContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

        // Determine if we're currently instantiating a slice
        // During slice instantiation, it's possible this entity will be activated before its parent. To avoid this, we want to wait to enable physics
        // until the entire slice has been instantiated. To do so, we start listening to the EntityContextEventBus for an OnSliceInstantiated event
        AZ::Data::AssetId instantiatingAsset;
        instantiatingAsset.SetInvalid();
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(instantiatingAsset, gameContextId,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::CurrentlyInstantiatingSlice);

        if (instantiatingAsset.IsValid())
        {
            // Start listening for game context events
            if (!gameContextId.IsNull())
            {
                AzFramework::SliceGameEntityOwnershipServiceNotificationBus::Handler::BusConnect();
            }
        }
        else
        {
            // Create and setup rigid body & associated bus handlers
            CreatePhysics();
            // Add to world
            EnablePhysics();
        }
    }

    void RigidBodyComponent::Deactivate()
    {
        if (m_staticTransformAtActivation)
        {
            return;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_rigidBodyHandle);
            m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
            m_rigidBody = nullptr;
        }

        Physics::RigidBodyRequestBus::Handler::BusDisconnect();
        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        m_sceneFinishSimHandler.Disconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RigidBodyComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*currentTime*/)
    {
        if (m_configuration.m_interpolateMotion)
        {
            AZ::Vector3 newPosition = AZ::Vector3::CreateZero();
            AZ::Quaternion newRotation = AZ::Quaternion::CreateIdentity();
            m_interpolator->GetInterpolated(newPosition, newRotation, deltaTime);

            AZ::Transform interpolatedTransform = AZ::Transform::CreateFromQuaternionAndTranslation(newRotation, newPosition);
            interpolatedTransform.MultiplyByScale(m_initialScale);
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetWorldTM, interpolatedTransform);
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
                this->PostPhysicsTick(fixedDeltatime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));
    }

    void RigidBodyComponent::PostPhysicsTick(float fixedDeltaTime)
    {
        // When transform changes, Kinematic Target is updated with the new transform, so don't set the transform again.
        // But in the case of setting the Kinematic Target directly, the transform needs to reflect the new kinematic target
        //    User sets kinematic Target ---> Update transform
        //    User sets transform        ---> Update kinematic target

        if (!IsPhysicsEnabled() || (m_rigidBody->IsKinematic() && !m_isLastMovementFromKinematicSource))
        {
            return;
        }

        if (m_configuration.m_interpolateMotion)
        {
            AZ::Transform transform = m_rigidBody->GetTransform();
            m_interpolator->SetTarget(transform.GetTranslation(), m_rigidBody->GetOrientation(), fixedDeltaTime);
        }
        else
        {
            AZ::Transform transform = m_rigidBody->GetTransform();

            // Maintain scale (this must be precise).
            AZ::Transform entityTransform = AZ::Transform::Identity();
            AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
            transform.MultiplyByScale(m_initialScale);

            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetWorldTM, transform);
        }
        m_isLastMovementFromKinematicSource = false;
    }

    void RigidBodyComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // Note: OnTransformChanged is not safe at the moment due to TransformComponent design flaw.
        // It is called when the parent entity is activated after the children causing rigid body
        // to move through the level instantly.
        if (IsPhysicsEnabled() && (m_rigidBody->IsKinematic() && !m_isLastMovementFromKinematicSource))
        {
            m_rigidBody->SetKinematicTarget(world);
        }
        else if (!IsPhysicsEnabled())
        {
            m_rigidBodyTransformNeedsUpdateOnPhysReEnable = true;
        }
    }

    void RigidBodyComponent::CreatePhysics()
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

        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface != nullptr)
        {
            m_rigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &m_configuration);
            m_rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_rigidBodyHandle));
            //disable simulating the body until EnablePhysics is called.
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_rigidBodyHandle);
        }
        m_rigidBody->SetKinematic(m_configuration.m_kinematic);

        AzPhysics::MassComputeFlags flags = m_configuration.GetMassComputeFlags();
        m_rigidBody->UpdateMassProperties(flags, &m_configuration.m_centerOfMassOffset, &m_configuration.m_inertiaTensor,
            &m_configuration.m_mass);

        // Listen to the PhysX system for events concerning this entity.
        if (sceneInterface != nullptr)
        {
            sceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);
        }
        AZ::TickBus::Handler::BusConnect();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        Physics::RigidBodyRequestBus::Handler::BusConnect(GetEntityId());
        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RigidBodyComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_rigidBodyHandle);
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        if (m_rigidBodyTransformNeedsUpdateOnPhysReEnable)
        {
            m_rigidBody->SetTransform(transform);
            m_rigidBodyTransformNeedsUpdateOnPhysReEnable = false;
        }

        AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
        AZ::TransformBus::EventResult(rotation, GetEntityId(), &AZ::TransformInterface::GetWorldRotationQuaternion);

        m_interpolator = std::make_unique<TransformForwardTimeInterpolator>();
        m_interpolator->Reset(transform.GetTranslation(), rotation);

        m_initialScale = transform.ExtractScale();

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsEnabled);
        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsEnabled);
    }

    void RigidBodyComponent::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_rigidBodyHandle);
        }

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsDisabled);
        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsDisabled);
    }

    bool RigidBodyComponent::IsPhysicsEnabled() const
    {
        return m_rigidBody != nullptr && m_rigidBody->m_simulating;
    }

    void RigidBodyComponent::ApplyLinearImpulse(const AZ::Vector3& impulse)
    {
        m_rigidBody->ApplyLinearImpulse(impulse);
    }

    void RigidBodyComponent::ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint)
    {
        m_rigidBody->ApplyLinearImpulseAtWorldPoint(impulse, worldSpacePoint);
    }

    void RigidBodyComponent::ApplyAngularImpulse(const AZ::Vector3& impulse)
    {
        m_rigidBody->ApplyAngularImpulse(impulse);
    }

    AZ::Vector3 RigidBodyComponent::GetLinearVelocity() const
    {
        return m_rigidBody->GetLinearVelocity();
    }

    void RigidBodyComponent::SetLinearVelocity(const AZ::Vector3& velocity)
    {
        m_rigidBody->SetLinearVelocity(velocity);
    }

    AZ::Vector3 RigidBodyComponent::GetAngularVelocity() const
    {
        return m_rigidBody->GetAngularVelocity();
    }

    void RigidBodyComponent::SetAngularVelocity(const AZ::Vector3& angularVelocity)
    {
        m_rigidBody->SetAngularVelocity(angularVelocity);
    }

    AZ::Vector3 RigidBodyComponent::GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const
    {
        return m_rigidBody->GetLinearVelocityAtWorldPoint(worldPoint);
    }

    AZ::Vector3 RigidBodyComponent::GetCenterOfMassWorld() const
    {
        return m_rigidBody->GetCenterOfMassWorld();
    }

    AZ::Vector3 RigidBodyComponent::GetCenterOfMassLocal() const
    {
        return m_rigidBody->GetCenterOfMassLocal();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInverseInertiaWorld() const
    {
        return m_rigidBody->GetInverseInertiaWorld();
    }

    AZ::Matrix3x3 RigidBodyComponent::GetInverseInertiaLocal() const
    {
        return m_rigidBody->GetInverseInertiaLocal();
    }

    float RigidBodyComponent::GetMass() const
    {
        return m_rigidBody->GetMass();
    }

    float RigidBodyComponent::GetInverseMass() const
    {
        return m_rigidBody->GetInverseMass();
    }

    void RigidBodyComponent::SetMass(float mass)
    {
        m_rigidBody->SetMass(mass);
    }

    void RigidBodyComponent::SetCenterOfMassOffset(const AZ::Vector3& comOffset)
    {
        m_rigidBody->SetCenterOfMassOffset(comOffset);
    }

    float RigidBodyComponent::GetLinearDamping() const
    {
        return m_rigidBody->GetLinearDamping();
    }

    void RigidBodyComponent::SetLinearDamping(float damping)
    {
        m_rigidBody->SetLinearDamping(damping);
    }

    float RigidBodyComponent::GetAngularDamping() const
    {
        return m_rigidBody->GetAngularDamping();
    }

    void RigidBodyComponent::SetAngularDamping(float damping)
    {
        m_rigidBody->SetAngularDamping(damping);
    }

    bool RigidBodyComponent::IsAwake() const
    {
        return m_rigidBody->IsAwake();
    }

    void RigidBodyComponent::ForceAsleep()
    {
        m_rigidBody->ForceAsleep();
    }

    void RigidBodyComponent::ForceAwake()
    {
        m_rigidBody->ForceAwake();
    }

    bool RigidBodyComponent::IsKinematic() const
    {
        return m_rigidBody->IsKinematic();
    }

    void RigidBodyComponent::SetKinematic(bool kinematic)
    {
        m_rigidBody->SetKinematic(kinematic);
    }

    void RigidBodyComponent::SetKinematicTarget(const AZ::Transform& targetPosition)
    {
        m_isLastMovementFromKinematicSource = true;
        m_rigidBody->SetKinematicTarget(targetPosition);
    }

    bool RigidBodyComponent::IsGravityEnabled() const
    {
        return m_rigidBody->IsGravityEnabled();
    }

    void RigidBodyComponent::SetGravityEnabled(bool enabled)
    {
        m_rigidBody->SetGravityEnabled(enabled);
    }

    void RigidBodyComponent::SetSimulationEnabled(bool enabled)
    {
        m_rigidBody->SetSimulationEnabled(enabled);
    }

    float RigidBodyComponent::GetSleepThreshold() const
    {
        return m_rigidBody->GetSleepThreshold();
    }

    void RigidBodyComponent::SetSleepThreshold(float threshold)
    {
        m_rigidBody->SetSleepThreshold(threshold);
    }

    AZ::Aabb RigidBodyComponent::GetAabb() const
    {
        return m_rigidBody->GetAabb();
    }

    AzPhysics::RigidBody* RigidBodyComponent::GetRigidBody()
    {
        return m_rigidBody;
    }

    AzPhysics::SimulatedBody* RigidBodyComponent::GetWorldBody()
    {
        return m_rigidBody;
    }

    AzPhysics::SceneQueryHit RigidBodyComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_rigidBody)
        {
            return m_rigidBody->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    void RigidBodyComponent::OnSliceInstantiated(const AZ::Data::AssetId&, const AZ::SliceComponent::SliceInstanceAddress&,
        const AzFramework::SliceInstantiationTicket&)
    {
        CreatePhysics();
        EnablePhysics();
        AzFramework::SliceGameEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
    }

    void RigidBodyComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId&, const AzFramework::SliceInstantiationTicket&)
    {
        // Enable physics even in the case of instantiation failure. If we've made it this far, the
        // entity is valid and should be activated normally.
        CreatePhysics();
        EnablePhysics();
        AzFramework::SliceGameEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
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
