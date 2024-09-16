/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ArticulationLinkComponent.h>

#include <ArticulationUtils.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    // Definitions are put in .cpp so we can have AZStd::unique_ptr<T> member with forward declared T in the header
    // This causes AZStd::unique_ptr<T> ctor/dtor to be generated when full type info is available
    ArticulationLinkComponent::ArticulationLinkComponent()
    {
        InitPhysicsTickHandler();
    }

    ArticulationLinkComponent::~ArticulationLinkComponent() = default;

    ArticulationLinkComponent::ArticulationLinkComponent(ArticulationLinkConfiguration& config)
        : m_config(config)
    {
        InitPhysicsTickHandler();
    }

    void ArticulationLinkComponent::Reflect(AZ::ReflectContext* context)
    {
        ArticulationLinkData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkComponent, AZ::Component>()
                ->Version(1)
                ->Field("ArticulationLinkData", &ArticulationLinkComponent::m_articulationLinkData)
                ->Field("ArticulationLinkConfiguration", &ArticulationLinkComponent::m_config);
        }
    }

    void ArticulationLinkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
        provided.push_back(AZ_CRC_CE("ArticulationLinkService"));
    }

    void ArticulationLinkComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void ArticulationLinkComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void ArticulationLinkComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    bool ArticulationLinkComponent::IsRootArticulation() const
    {
        return IsRootArticulationEntity<ArticulationLinkComponent>(GetEntity());
    }

    const AZ::Entity* ArticulationLinkComponent::GetArticulationRootEntity() const
    {
        bool rootFound = false;
        AZ::Entity* currentEntity = GetEntity();
        while (!rootFound)
        {
            AZ::EntityId parentId = currentEntity->GetTransform()->GetParentId();
            if (!parentId.IsValid())
            {
                rootFound = true;
            }
            else
            {
                AZ::Entity* parentEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

                if (parentEntity && parentEntity->FindComponent<ArticulationLinkComponent>())
                {
                    currentEntity = parentEntity;
                }
                else
                {
                    rootFound = true;
                }
            }
        }
        return currentEntity;
    }

    AZStd::vector<AzPhysics::SimulatedBodyHandle> ArticulationLinkComponent::GetSimulatedBodyHandles() const
    {
        return m_articulationLinks;
    }

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    void ArticulationLinkComponent::Activate()
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (!sceneInterface)
        {
            return;
        }
        Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            AZ_Error("ArticulationLinkComponent", false, "Invalid Scene Handle");
            return;
        }

        // set the transform to not update when the parent's transform changes, to avoid conflict with physics transform updates
        GetEntity()->GetTransform()->SetOnParentChangedBehavior(AZ::OnParentChangedBehavior::DoNotUpdate);

        if (IsRootArticulation())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
            if (m_attachedSceneHandle != AzPhysics::InvalidSceneHandle)
            {
                sceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);

                // Create a handler that in the case that the scene was removed before the deactivation of the component,
                // ensures that all articulations are destroyed.
                m_sceneRemovedHandler = AzPhysics::SystemEvents::OnSceneRemovedEvent::Handler(
                    [this](AzPhysics::SceneHandle sceneHandle)
                    {
                        if (sceneHandle == m_attachedSceneHandle && m_articulation)
                        {
                            DestroyArticulation();
                        }
                    });

                AZ::Interface<AzPhysics::SystemInterface>::Get()->RegisterSceneRemovedEvent(m_sceneRemovedHandler);

                CreateArticulation();
                m_link = GetArticulationLink(GetEntityId());
                m_sensorIndices = GetSensorIndices(GetEntityId());
            }
        }
        else
        {
            // the articulation is owned by the entity which has the root link
            // if this entity is not the root of the articulation, cache a pointer to the PxArticulationLink corresponding to this entity
            // parents are guaranteed to activate before children, so we can go up the hierarchy to find the root
            const auto* articulationRootEntity = GetArticulationRootEntity();
            if (articulationRootEntity)
            {
                auto* rootArticulationLinkComponent = articulationRootEntity->FindComponent<ArticulationLinkComponent>();
                AZ_Assert(rootArticulationLinkComponent, "Articulation root has to have ArticulationLinkComponent");

                     m_link = rootArticulationLinkComponent->GetArticulationLink(GetEntityId());
                     AZ_Assert(m_link, "Scene not found for the root articulation link component");

                     AzPhysics::Scene* scene = sceneInterface->GetScene(rootArticulationLinkComponent->m_attachedSceneHandle);
                     AZ_Assert(scene, "Scene not found for the root articulation link component");

                     auto* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
                     if (m_link && pxScene)
                     {
                         PHYSX_SCENE_READ_LOCK(pxScene);
                         m_driveJoint = m_link->getInboundJoint()->is<physx::PxArticulationJointReducedCoordinate>();
                     }

                m_sensorIndices = rootArticulationLinkComponent->GetSensorIndices(GetEntityId());
            }
        }

        FillSimulatedBodyHandle();

        ArticulationJointRequestBus::Handler::BusConnect(GetEntityId());
        ArticulationSensorRequestBus::Handler::BusConnect(GetEntityId());
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());

        Physics::RigidBodyNotificationBus::Event(
            GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsEnabled, GetEntityId());
    }

    void ArticulationLinkComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        ArticulationSensorRequestBus::Handler::BusDisconnect();
        ArticulationJointRequestBus::Handler::BusDisconnect();

        if (IsRootArticulation())
        {
            m_sceneRemovedHandler.Disconnect();

            if (m_articulation)
            {
                DestroyArticulation();
            }

            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }
        else
        {
            m_driveJoint = nullptr;
        }

        m_link = nullptr;
        m_sensorIndices.clear();

        // set the behavior when the parent's transform changes back to default, since physics is no longer controlling the transform
        GetEntity()->GetTransform()->SetOnParentChangedBehavior(AZ::OnParentChangedBehavior::Update);

        Physics::RigidBodyNotificationBus::Event(
            GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsDisabled, GetEntityId());
    }
#endif

    void ArticulationLinkComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
    }

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    void ArticulationLinkComponent::CreateArticulation()
    {
        AzPhysics::SceneInterface* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "PhysX Scene Interface not found");
        if (!sceneInterface)
        {
            return;
        }

        physx::PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        m_articulation = pxPhysics->createArticulationReducedCoordinate();


        const auto& rootLinkConfiguration = m_articulationLinkData->m_articulationLinkConfiguration;
        SetRootSpecificProperties(rootLinkConfiguration);

        CreateChildArticulationLinks(nullptr, *m_articulationLinkData.get());

        // Add articulation to the scene
        AzPhysics::Scene* scene = sceneInterface->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_WRITE_LOCK(pxScene);
        pxScene->addArticulation(*m_articulation);

        const AZ::u32 numSensors = m_articulation->getNbSensors();
        for (AZ::u32 sensorIndex = 0; sensorIndex < numSensors; sensorIndex++)
        {
            physx::PxArticulationSensor* sensor = nullptr;
            m_articulation->getSensors(&sensor, 1, sensorIndex);
            ActorData* linkActorData = Utils::GetUserData(sensor->getLink());
            if (linkActorData)
            {
                const auto entityId = linkActorData->GetEntityId();
                if (auto iterator = m_sensorIndicesByEntityId.find(entityId);
                    iterator != m_sensorIndicesByEntityId.end())
                {
                    iterator->second.push_back(sensor->getIndex());
                }
                else
                {
                    m_sensorIndicesByEntityId.insert(EntityIdSensorIndexListPair({ entityId, { sensor->getIndex() } }));
                }
            }
        }
    }

    void ArticulationLinkComponent::SetRootSpecificProperties(const ArticulationLinkConfiguration& rootLinkConfiguration)
    {
        m_articulation->setSleepThreshold(rootLinkConfiguration.m_sleepMinEnergy);
        if (rootLinkConfiguration.m_startAsleep)
        {
            m_articulation->putToSleep();
        }

        physx::PxArticulationFlags articulationFlags(0);
        if (rootLinkConfiguration.m_isFixedBase)
        {
            articulationFlags.raise(physx::PxArticulationFlag::eFIX_BASE);
        }

        if (!rootLinkConfiguration.m_selfCollide)
        {
            // Disable collisions between the articulation's links (note that parent/child collisions
            // are disabled internally in either case).
            articulationFlags.raise(physx::PxArticulationFlag::eDISABLE_SELF_COLLISION);
        }

        m_articulation->setArticulationFlags(articulationFlags);

        m_articulation->setSolverIterationCounts(
            rootLinkConfiguration.m_solverPositionIterations, rootLinkConfiguration.m_solverVelocityIterations);
        // TODO: Expose these in the configuration
        //      eDRIVE_LIMITS_ARE_FORCES //!< Limits for drive effort are forces and torques rather than impulses
        //      eCOMPUTE_JOINT_FORCES //!< Enable in order to be able to query joint solver .
    }

    void setInboundJointDriveParams(
        physx::PxArticulationJointReducedCoordinate* inboundJoint,
        [[maybe_unused]] physx::PxArticulationAxis articulationAxis,
        const ArticulationJointMotorProperties& motorProperties)
    {
        physx::PxArticulationDrive drive;
        drive.driveType = physx::PxArticulationDriveType::eFORCE;
        drive.maxForce = motorProperties.m_driveForceLimit;
        drive.damping = motorProperties.m_driveDamping;
        drive.stiffness = motorProperties.m_driveStiffness;
        inboundJoint->setDriveParams(physx::PxArticulationAxis::eTWIST, drive);
    }

    void ArticulationLinkComponent::CreateChildArticulationLinks(
        physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData)
    {
        const ArticulationLinkConfiguration& articulationLinkConfiguration = thisLinkData.m_articulationLinkConfiguration;

        physx::PxTransform thisLinkTransform;
        if (parentLink)
        {
            physx::PxTransform parentLinkTransform = parentLink->getGlobalPose();
            physx::PxTransform thisLinkRelativeTransform = PxMathConvert(thisLinkData.m_localTransform);
            thisLinkTransform = parentLinkTransform * thisLinkRelativeTransform;
        }
        else
        {
            thisLinkTransform = PxMathConvert(GetEntity()->GetTransform()->GetWorldTM());
        }

        physx::PxArticulationLink* thisPxLink = m_articulation->createLink(parentLink, thisLinkTransform);
        if (!thisPxLink)
        {
            AZ_Error("PhysX", false, "Failed to create articulation link at root %s", GetEntity()->GetName().c_str());
            return;
        }

        AzPhysics::SimulatedBodyHandle articulationLinkHandle =
            AZ::Interface<AzPhysics::SceneInterface>::Get()->AddSimulatedBody(m_attachedSceneHandle, &articulationLinkConfiguration);
        if (articulationLinkHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            AZ_Error("PhysX", false, "Failed to create a simulated body for the articulation link at root %s",
                GetEntity()->GetName().c_str());
            return;
        }

        m_articulationLinks.emplace_back(articulationLinkHandle);

        AzPhysics::SimulatedBody* simulatedBody =
            AZ::Interface<AzPhysics::SceneInterface>::Get()->GetSimulatedBodyFromHandle(m_attachedSceneHandle, articulationLinkHandle);

        ArticulationLink* articulationLink = azrtti_cast<ArticulationLink*>(simulatedBody);
        articulationLink->SetPxArticulationLink(thisPxLink);
        articulationLink->SetupFromLinkData(thisLinkData);

        if (parentLink)
        {
            physx::PxArticulationJointReducedCoordinate* inboundJoint =
                thisPxLink->getInboundJoint()->is<physx::PxArticulationJointReducedCoordinate>();
            // Sets the joint pose in the lead link actor frame.
            inboundJoint->setParentPose(PxMathConvert(thisLinkData.m_jointLeadLocalFrame));
            // Sets the joint pose in the follower link actor frame.
            inboundJoint->setChildPose(PxMathConvert(thisLinkData.m_jointFollowerLocalFrame));
            // Sets the joint type and limits.
            switch (articulationLinkConfiguration.m_articulationJointType)
            {
            case ArticulationJointType::Fix:
                inboundJoint->setJointType(physx::PxArticulationJointType::eFIX);
                break;
            case ArticulationJointType::Hinge:
                inboundJoint->setJointType(physx::PxArticulationJointType::eREVOLUTE);
                if (articulationLinkConfiguration.m_isLimited)
                {
                    // The lower limit should be strictly smaller than the higher limit.
                    physx::PxArticulationLimit limits;
                    limits.low = AZ::DegToRad(AZStd::min(
                        articulationLinkConfiguration.m_angularLimitNegative, articulationLinkConfiguration.m_angularLimitPositive));
                    limits.high = AZ::DegToRad(AZStd::max(
                        articulationLinkConfiguration.m_angularLimitNegative, articulationLinkConfiguration.m_angularLimitPositive));

                    // From PhysX documentation: If the limits should be equal, use PxArticulationMotion::eLOCKED
                    if (limits.low == limits.high)
                    {
                        inboundJoint->setMotion(physx::PxArticulationAxis::eTWIST, physx::PxArticulationMotion::eLOCKED);
                    }
                    else
                    {
                        inboundJoint->setMotion(
                            physx::PxArticulationAxis::eTWIST, physx::PxArticulationMotion::eLIMITED); // limit the x rotation axis (eTWIST)
                    }
                    inboundJoint->setLimitParams(physx::PxArticulationAxis::eTWIST, limits);
                }
                else
                {
                    inboundJoint->setMotion(
                        physx::PxArticulationAxis::eTWIST, physx::PxArticulationMotion::eFREE); // free on the x rotation axis (eTWIST)
                }
                if (articulationLinkConfiguration.m_motorConfiguration.m_useMotor)
                {
                    physx::PxArticulationDrive drive;
                    drive.driveType = physx::PxArticulationDriveType::eFORCE;
                    drive.maxForce = articulationLinkConfiguration.m_motorConfiguration.m_driveForceLimit;
                    drive.damping = articulationLinkConfiguration.m_motorConfiguration.m_driveDamping;
                    drive.stiffness = articulationLinkConfiguration.m_motorConfiguration.m_driveStiffness;
                    inboundJoint->setDriveParams(physx::PxArticulationAxis::eTWIST, drive);
                }
                inboundJoint->setFrictionCoefficient(articulationLinkConfiguration.m_jointFriction);
                if (articulationLinkConfiguration.m_armature.GetX() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eTWIST, articulationLinkConfiguration.m_armature.GetX());
                }
                if (articulationLinkConfiguration.m_armature.GetY() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eSWING1, articulationLinkConfiguration.m_armature.GetY());
                }
                if (articulationLinkConfiguration.m_armature.GetZ() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eSWING2, articulationLinkConfiguration.m_armature.GetZ());
                }
                break;
            case ArticulationJointType::Prismatic:
                inboundJoint->setJointType(physx::PxArticulationJointType::ePRISMATIC);
                if (articulationLinkConfiguration.m_isLimited)
                {
                    // The lower limit should be strictly smaller than the higher limit.
                    physx::PxArticulationLimit limits;
                    limits.low =
                        AZStd::min(articulationLinkConfiguration.m_linearLimitLower, articulationLinkConfiguration.m_linearLimitUpper);
                    limits.high =
                        AZStd::max(articulationLinkConfiguration.m_linearLimitLower, articulationLinkConfiguration.m_linearLimitUpper);

                    // From PhysX documentation: If the limits should be equal, use PxArticulationMotion::eLOCKED
                    if (limits.low == limits.high)
                    {
                        inboundJoint->setMotion(physx::PxArticulationAxis::eX, physx::PxArticulationMotion::eLOCKED);
                    }
                    else
                    {
                        inboundJoint->setMotion(
                            physx::PxArticulationAxis::eX, physx::PxArticulationMotion::eLIMITED); // limit the x movement axis (eX)
                    }
                    inboundJoint->setLimitParams(physx::PxArticulationAxis::eX, limits);
                }
                else
                {
                    inboundJoint->setMotion(
                        physx::PxArticulationAxis::eX, physx::PxArticulationMotion::eFREE); // free on the x movement axis (eX)
                }
                if (articulationLinkConfiguration.m_motorConfiguration.m_useMotor)
                {
                    physx::PxArticulationDrive drive;
                    drive.driveType = physx::PxArticulationDriveType::eFORCE;
                    drive.maxForce = articulationLinkConfiguration.m_motorConfiguration.m_driveForceLimit;
                    drive.damping = articulationLinkConfiguration.m_motorConfiguration.m_driveDamping;
                    drive.stiffness = articulationLinkConfiguration.m_motorConfiguration.m_driveStiffness;
                    inboundJoint->setDriveParams(physx::PxArticulationAxis::eX, drive);
                }
                inboundJoint->setFrictionCoefficient(articulationLinkConfiguration.m_jointFriction);
                if (articulationLinkConfiguration.m_armature.GetX() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eX, articulationLinkConfiguration.m_armature.GetX());
                }
                if (articulationLinkConfiguration.m_armature.GetY() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eY, articulationLinkConfiguration.m_armature.GetY());
                }
                if (articulationLinkConfiguration.m_armature.GetZ() > AZ::Constants::FloatEpsilon)
                {
                    inboundJoint->setArmature(physx::PxArticulationAxis::eZ, articulationLinkConfiguration.m_armature.GetZ());
                }
                break;
            default:
                AZ_Error("ArticulationLinkComponent", false, "Unexpected articulation joint type.");
                break;
            }
        }

        // set up sensors
        for (const auto& sensorConfig : articulationLinkConfiguration.m_sensorConfigs)
        {
            const AZ::Transform sensorTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(sensorConfig.m_localRotation), sensorConfig.m_localPosition);
            auto* sensor = thisPxLink->getArticulation().createSensor(thisPxLink, PxMathConvert(sensorTransform));
            sensor->setFlag(physx::PxArticulationSensorFlag::eFORWARD_DYNAMICS_FORCES, sensorConfig.m_includeForwardDynamicsForces);
            sensor->setFlag(physx::PxArticulationSensorFlag::eCONSTRAINT_SOLVER_FORCES, sensorConfig.m_includeConstraintSolverForces);
            sensor->setFlag(physx::PxArticulationSensorFlag::eWORLD_FRAME, sensorConfig.m_useWorldFrame);
        }

        m_articulationLinksByEntityId.insert(EntityIdArticulationLinkPair{ articulationLinkConfiguration.m_entityId, thisPxLink });

        for (const auto& childLink : thisLinkData.m_childLinks)
        {
            CreateChildArticulationLinks(thisPxLink, *childLink);
        }
    }


    void ArticulationLinkComponent::DestroyArticulation()
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        if (scene == nullptr)
        {
            // The scene can be removed before articulation is destroyed.
            // If the scene was removed. Articulations were also removed.
            return;
        }
        scene->RemoveSimulatedBodies(m_articulationLinks);
        m_articulationLinks.clear();

        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
        PHYSX_SCENE_WRITE_LOCK(pxScene);
        m_articulation->release();

        m_sensorIndicesByEntityId.clear();
    }

    void ArticulationLinkComponent::InitPhysicsTickHandler()
    {
        m_sceneFinishSimHandler = AzPhysics::SceneEvents::OnSceneSimulationFinishHandler(
            [this]([[maybe_unused]] AzPhysics::SceneHandle sceneHandle, float fixedDeltatime)
            {
                PostPhysicsTick(fixedDeltatime);
            },
            aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));
    }

    void ArticulationLinkComponent::PostPhysicsTick([[maybe_unused]] float fixedDeltaTime)
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxScene);

        if (m_articulation->isSleeping())
        {
            return;
        }

        physx::PxArticulationLink* links[MaxArticulationLinks] = { 0 };
        m_articulation->getLinks(links, MaxArticulationLinks);

        const physx::PxU32 linksNum = m_articulation->getNbLinks();
        AZ_Assert(
            linksNum <= MaxArticulationLinks,
            "Error. Number of articulation links %d is greater than the maximum supported %d",
            linksNum,
            MaxArticulationLinks);

        for (physx::PxU32 linkIndex = 0; linkIndex < linksNum; ++linkIndex)
        {
            physx::PxArticulationLink* link = links[linkIndex];
            physx::PxTransform pxGlobalPose = link->getGlobalPose();
            AZ::Transform globalTransform = PxMathConvert(pxGlobalPose);
            ActorData* linkActorData = Utils::GetUserData(link);
            if (linkActorData)
            {
                AZ::EntityId linkEntityId = linkActorData->GetEntityId();
                AZ::TransformBus::Event(linkEntityId, &AZ::TransformBus::Events::SetWorldTM, globalTransform);
            }
        }
    }

    physx::PxArticulationLink* ArticulationLinkComponent::GetArticulationLink(const AZ::EntityId entityId)
    {
        if (const auto iterator = m_articulationLinksByEntityId.find(entityId);
            iterator != m_articulationLinksByEntityId.end())
        {
            return iterator->second;
        }
        else
        {
            return nullptr;
        }
    }

    const AZStd::vector<AZ::u32> ArticulationLinkComponent::GetSensorIndices(const AZ::EntityId entityId)
    {
        if (const auto iterator = m_sensorIndicesByEntityId.find(entityId);
            iterator != m_sensorIndicesByEntityId.end())
        {
            return iterator->second;
        }
        else
        {
            return {};
        }
    }

    const physx::PxArticulationJointReducedCoordinate* ArticulationLinkComponent::GetDriveJoint() const
    {
        [[maybe_unused]] const bool isRootArticulation = IsRootArticulation();
        AZ_ErrorOnce("Articulation Link Component", !isRootArticulation, "Articulation root does not have an inbound joint.");
        AZ_ErrorOnce("Articulation Link Component", m_driveJoint || IsRootArticulation(), "Invalid articulation joint pointer");
        return m_driveJoint;
    }

    physx::PxArticulationJointReducedCoordinate* ArticulationLinkComponent::GetDriveJoint()
    {
        return const_cast<physx::PxArticulationJointReducedCoordinate*>(
            static_cast<const ArticulationLinkComponent&>(*this).GetDriveJoint());
    }

    void ArticulationLinkComponent::SetMotion(ArticulationJointAxis jointAxis, ArticulationJointMotionType jointMotionType)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setMotion(GetPxArticulationAxis(jointAxis), GetPxArticulationMotion(jointMotionType));
        }
    }

    ArticulationJointMotionType ArticulationLinkComponent::GetMotion(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            return GetArticulationJointMotionType(joint->getMotion(GetPxArticulationAxis(jointAxis)));
        }
        return ArticulationJointMotionType::Locked;
    }

    void ArticulationLinkComponent::SetLimit(ArticulationJointAxis jointAxis, AZStd::pair<float, float> limitPair)
    {
        if (auto* joint = GetDriveJoint())
        {
            const physx::PxArticulationLimit limit(limitPair.first, limitPair.second);
            joint->setLimitParams(GetPxArticulationAxis(jointAxis), limit);
        }
    }

    AZStd::pair<float, float> ArticulationLinkComponent::GetLimit(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto limit = joint->getLimitParams(GetPxArticulationAxis(jointAxis));
            return { limit.low, limit.high };
        }
        return { -AZ::Constants::FloatMax, AZ::Constants::FloatMax };
    }

    void ArticulationLinkComponent::SetDriveStiffness(ArticulationJointAxis jointAxis, float stiffness)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetPxArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.stiffness = stiffness;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetDriveStiffness(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetPxArticulationAxis(jointAxis));
            return driveParams.stiffness;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetDriveDamping(ArticulationJointAxis jointAxis, float damping)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetPxArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.damping = damping;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetDriveDamping(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetPxArticulationAxis(jointAxis));
            return driveParams.damping;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetMaxForce(ArticulationJointAxis jointAxis, float maxForce)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetPxArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.maxForce = maxForce;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetMaxForce(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetPxArticulationAxis(jointAxis));
            return driveParams.maxForce;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetIsAccelerationDrive(ArticulationJointAxis jointAxis, bool isAccelerationDrive)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetPxArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.driveType =
                isAccelerationDrive ? physx::PxArticulationDriveType::eACCELERATION : physx::PxArticulationDriveType::eFORCE;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    bool ArticulationLinkComponent::IsAccelerationDrive(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetPxArticulationAxis(jointAxis));
            return driveParams.driveType == physx::PxArticulationDriveType::eACCELERATION;
        }
        return false;
    }

    void ArticulationLinkComponent::SetDriveTarget(ArticulationJointAxis jointAxis, float target)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setDriveTarget(GetPxArticulationAxis(jointAxis), target);
        }
    }

    float ArticulationLinkComponent::GetDriveTarget(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getDriveTarget(GetPxArticulationAxis(jointAxis));
        }
        return 0.0f;
    }

    void ArticulationLinkComponent::SetDriveTargetVelocity(ArticulationJointAxis jointAxis, float targetVelocity)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setDriveVelocity(GetPxArticulationAxis(jointAxis), targetVelocity);
        }
    }

    float ArticulationLinkComponent::GetDriveTargetVelocity(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getDriveVelocity(GetPxArticulationAxis(jointAxis));
        }
        return 0.0f;
    }

    float ArticulationLinkComponent::GetJointPosition(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getJointPosition(GetPxArticulationAxis(jointAxis));
        }
        return 0.0f;
    }

    float ArticulationLinkComponent::GetJointVelocity(ArticulationJointAxis jointAxis) const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getJointVelocity(GetPxArticulationAxis(jointAxis));
        }
        return 0.0f;
    }

    void ArticulationLinkComponent::SetFrictionCoefficient(float frictionCoefficient)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setFrictionCoefficient(frictionCoefficient);
        }
    }

    float ArticulationLinkComponent::GetFrictionCoefficient() const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getFrictionCoefficient();
        }
        return 0.0f;
    }

    void ArticulationLinkComponent::SetMaxJointVelocity(float maxJointVelocity)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setMaxJointVelocity(maxJointVelocity);
        }
    }

    float ArticulationLinkComponent::GetMaxJointVelocity() const
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getMaxJointVelocity();
        }
        return 0.0f;
    }

    const physx::PxArticulationSensor* ArticulationLinkComponent::GetSensor(AZ::u32 sensorIndex) const
    {
        if (sensorIndex >= m_sensorIndices.size())
        {
            AZ_ErrorOnce(
                "Articulation Link Component", false, "Invalid sensor index (%i) for entity %s", sensorIndex, GetEntity()->GetName().c_str());
            return nullptr;
        }

        if (!m_link)
        {
            AZ_ErrorOnce("Articulation Link Component", false, "Invalid link pointer for entity %s", GetEntity()->GetName().c_str());
            return nullptr;
        }

        AZ::u32 internalIndex = m_sensorIndices[sensorIndex];
        auto& articulation = m_link->getArticulation();
        const auto numSensors = articulation.getNbSensors();
        if (internalIndex >= numSensors)
        {
            AZ_ErrorOnce(
                "Articulation Link Component",
                false,
                "Invalid internal sensor index (%i) for entity %s",
                sensorIndex,
                GetEntity()->GetName().c_str());
            return nullptr;
        }

        physx::PxArticulationSensor* sensor;
        articulation.getSensors(&sensor, 1, internalIndex);
        return sensor;
    }

    physx::PxArticulationSensor* ArticulationLinkComponent::GetSensor(AZ::u32 sensorIndex)
    {
        return const_cast<physx::PxArticulationSensor*>(static_cast<const ArticulationLinkComponent&>(*this).GetSensor(sensorIndex));
    }

    AZ::Transform ArticulationLinkComponent::GetSensorTransform(AZ::u32 sensorIndex) const
    {
        if (auto* sensor = GetSensor(sensorIndex))
        {
            return PxMathConvert(sensor->getRelativePose());
        }
        return AZ::Transform::CreateIdentity();
    }

    void ArticulationLinkComponent::SetSensorTransform(AZ::u32 sensorIndex, const AZ::Transform& sensorTransform)
    {
        if (auto* sensor = GetSensor(sensorIndex))
        {
            sensor->setRelativePose(PxMathConvert(sensorTransform));
        }
    }

    AZ::Vector3 ArticulationLinkComponent::GetForce(AZ::u32 sensorIndex) const
    {
        if (auto* sensor = GetSensor(sensorIndex))
        {
            return PxMathConvert(sensor->getForces().force);
        }
        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 ArticulationLinkComponent::GetTorque(AZ::u32 sensorIndex) const
    {
        if (auto* sensor = GetSensor(sensorIndex))
        {
            return PxMathConvert(sensor->getForces().torque);
        }
        return AZ::Vector3::CreateZero();
    }


    const AzPhysics::SimulatedBody* ArticulationLinkComponent::GetSimulatedBodyConst() const
    {
        const AZ::Entity* rootEntity = GetArticulationRootEntity();
        const auto rootComponent = rootEntity->FindComponent<ArticulationLinkComponent>();

        return AZ::Interface<AzPhysics::SceneInterface>::Get()->GetSimulatedBodyFromHandle(
            rootComponent->m_attachedSceneHandle, GetSimulatedBodyHandle());
    }

    AzPhysics::SimulatedBody* ArticulationLinkComponent::GetSimulatedBody()
    {
        return const_cast<AzPhysics::SimulatedBody*>(GetSimulatedBodyConst());
    }

    AzPhysics::SimulatedBodyHandle ArticulationLinkComponent::GetSimulatedBodyHandle() const
    {
        return m_bodyHandle;
    }

    void ArticulationLinkComponent::FillSimulatedBodyHandle()
    {
        const AZ::Entity* rootEntity = GetArticulationRootEntity();
        AZ_Assert(rootEntity, "Articulation root entity is null");
        const auto rootComponent = rootEntity->FindComponent<ArticulationLinkComponent>();
        AZ_Assert(rootComponent, "Articulation root entity has not ArticulationLinkComponent");

        for (auto articulationHandle : rootComponent->GetSimulatedBodyHandles())
        {
            auto simulatedBody = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetSimulatedBodyFromHandle(
                rootComponent->m_attachedSceneHandle, articulationHandle);
            if (simulatedBody)
            {
                if (simulatedBody->GetEntityId() == GetEntityId())
                {
                    m_bodyHandle = articulationHandle;
                    return;
                }
            }
            else
            {
                AZ_Error("ArticulationLinkComponent", false, "Failed to get simulated body from simulated body handle");
            }
        }

        AZ_Error("ArticulationLinkComponent", false, "No simulated body handle found");
    }

    void ArticulationLinkComponent::EnablePhysics()
    {
        AZ_Error("ArticulationLinkComponent", false, "Articulation links don't support enabling and disabling physics yet. Physics is always enabled.");
    }

    void ArticulationLinkComponent::DisablePhysics()
    {
        AZ_Error("ArticulationLinkComponent", false, "Articulation links don't support enabling and disabling physics yet. Physics is always enabled.");
    }

    bool ArticulationLinkComponent::IsPhysicsEnabled() const
    {
        return true;
    }

    AZ::Aabb ArticulationLinkComponent::GetAabb() const
    {
        return GetSimulatedBodyConst()->GetAabb();
    }

    AzPhysics::SceneQueryHit ArticulationLinkComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        return GetSimulatedBody()->RayCast(request);
    }

#else
    void ArticulationLinkComponent::Activate() {}
    void ArticulationLinkComponent::Deactivate() {}
    void ArticulationLinkComponent::CreateArticulation() {}
    void ArticulationLinkComponent::DestroyArticulation() {}
    void ArticulationLinkComponent::InitPhysicsTickHandler() {}
#endif
} // namespace PhysX
