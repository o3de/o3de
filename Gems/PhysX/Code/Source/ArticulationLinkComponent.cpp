/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ArticulationLinkComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/Material/PhysXMaterial.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <Shape.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
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

    void ArticulationLinkData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkData>()
                ->Version(1)
                ->Field("ShapeConfiguration", &ArticulationLinkData::m_shapeConfiguration)
                ->Field("ColliderConfiguration", &ArticulationLinkData::m_colliderConfiguration)
                ->Field("EntityId", &ArticulationLinkData::m_entityId)
                ->Field("RelativeTransform", &ArticulationLinkData::m_relativeTransform)
                ->Field("ChildLinks", &ArticulationLinkData::m_childLinks)
                ->Field("PhysxSpecificConfig", &ArticulationLinkData::m_physxSpecificConfig)
                ->Field("GenericProperties", &ArticulationLinkData::m_genericProperties)
                ->Field("Limits", &ArticulationLinkData::m_limits)
                ->Field("Motor", &ArticulationLinkData::m_motor)
            ;
        }
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

    AZ::Entity* ArticulationLinkComponent::GetArticulationRootEntity() const
    {
        bool rootFound = false;
        AZ::Entity* currentEntity = GetEntity();
        while (!rootFound)
        {
            AZ::EntityId parentId = currentEntity->GetTransform()->GetParentId();
            if (parentId.IsValid())
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

    void ArticulationLinkComponent::Activate()
    {
        if (IsRootArticulation())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);
            }

            CreateArticulation();
        }

        else
        {
            // the articulation is owned by the entity which has the root link
            // if this entity is not the root of the articulation, cache a pointer to the PxArticulationLink corresponding to this entity
            // parents are guaranteed to activate before children, so we can go up the hierarchy to find the root
            const auto* articulationRootEntity = GetArticulationRootEntity();
            if (articulationRootEntity)
            {
                const auto rootArticulationLinkComponent = articulationRootEntity->FindComponent<ArticulationLinkComponent>();
                if (rootArticulationLinkComponent)
                {
                    m_link = rootArticulationLinkComponent->GetArticulationLink(GetEntityId());
                    if (m_link)
                    {
                        m_driveJoint = m_link->getInboundJoint();
                    }
                }
            }
        }
    }

    void ArticulationLinkComponent::Deactivate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        if (m_articulation)
        {
            DestroyArticulation();
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void ArticulationLinkComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
    }

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    void ArticulationLinkComponent::CreateArticulation()
    {
        physx::PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        m_articulation = pxPhysics->createArticulationReducedCoordinate();

        CreateChildArticulationLinks(nullptr, *m_articulationLinkData.get());

        // Add articulation to the scene
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_WRITE_LOCK(pxScene);
        pxScene->addArticulation(*m_articulation);
    }

    void ArticulationLinkComponent::CreateChildArticulationLinks(
        physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData)
    {
        const Physics::ColliderConfiguration& colliderConfiguration = thisLinkData.m_colliderConfiguration;
        const Physics::ShapeConfiguration* shapeConfiguration = thisLinkData.m_shapeConfiguration.get();

        AZStd::shared_ptr<Physics::Shape> physicsShape;

        if (shapeConfiguration)
        {
            if (shapeConfiguration->GetShapeType() == Physics::ShapeType::PhysicsAsset)
            {
                const auto* physicsAssetShapeConfiguration =
                    static_cast<const Physics::PhysicsAssetShapeConfiguration*>(shapeConfiguration);
                if (!physicsAssetShapeConfiguration->m_asset.IsReady())
                {
                    const_cast<Physics::PhysicsAssetShapeConfiguration*>(physicsAssetShapeConfiguration)->m_asset.BlockUntilLoadComplete();
                }

                const bool hasNonUniformScale = !Physics::Utils::HasUniformScale(physicsAssetShapeConfiguration->m_assetScale) ||
                    (AZ::NonUniformScaleRequestBus::FindFirstHandler(GetEntityId()) != nullptr);
                AZStd::vector<AZStd::shared_ptr<Physics::Shape>> assetShapes;
                Utils::CreateShapesFromAsset(
                    *physicsAssetShapeConfiguration,
                    colliderConfiguration,
                    hasNonUniformScale,
                    physicsAssetShapeConfiguration->m_subdivisionLevel,
                    assetShapes);

                if (!assetShapes.empty())
                {
                    physicsShape = assetShapes[0];
                    AZ_Warning("PhysX", assetShapes.size() == 1,
                        "Articulation %s has a link with physics mesh with more than 1 shape", GetEntity()->GetName().c_str());
                }
            }
            else
            {
                Physics::SystemRequestBus::BroadcastResult(
                    physicsShape, &Physics::SystemRequests::CreateShape, colliderConfiguration, *shapeConfiguration);
            }

            m_articulationShapes.emplace_back(physicsShape);
        }

        physx::PxTransform thisLinkTransform;
        if (parentLink)
        {
            physx::PxTransform parentLinkTransform = parentLink->getGlobalPose();
            physx::PxTransform thisLinkRelativeTransform = PxMathConvert(thisLinkData.m_relativeTransform);
            thisLinkTransform = parentLinkTransform * thisLinkRelativeTransform;
        }
        else
        {
            thisLinkTransform = PxMathConvert(GetEntity()->GetTransform()->GetWorldTM());
        }

        physx::PxArticulationLink* thisLink = m_articulation->createLink(parentLink, thisLinkTransform);
        if (!thisLink)
        {
            AZ_Error("PhysX", false, "Failed to create articulation link at root %s", GetEntity()->GetName().c_str());
            return;
        }

        // Setup actor data
        AZStd::shared_ptr<ActorData> thisLinkActorData = AZStd::make_shared<ActorData>(thisLink);
        thisLinkActorData->SetEntityId(thisLinkData.m_entityId);
        m_linksActorData.emplace_back(thisLinkActorData);

        if (parentLink)
        {
            physx::PxArticulationJointReducedCoordinate* inboundJoint =
                thisLink->getInboundJoint()->is<physx::PxArticulationJointReducedCoordinate>();
            // TODO: Set the values for joints from thisLinkData
            inboundJoint->setJointType(physx::PxArticulationJointType::eFIX);
            inboundJoint->setParentPose(PxMathConvert(thisLinkData.m_relativeTransform));
            inboundJoint->setChildPose(physx::PxTransform(physx::PxIdentity));
        }

        if (physicsShape)
        {
            thisLink->attachShape(*static_cast<physx::PxShape*>(physicsShape->GetNativePointer()));
        }

        m_articulationLinksByEntityId.insert({ thisLinkData.m_entityId, thisLink });

        for (const auto& childLink : thisLinkData.m_childLinks)
        {
            CreateChildArticulationLinks(thisLink, *childLink);
        }
    }

    void ArticulationLinkComponent::DestroyArticulation()
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
        PHYSX_SCENE_WRITE_LOCK(pxScene);
        m_articulation->release();
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
        const auto iterator = m_articulationLinksByEntityId.find(entityId);
        if (iterator != m_articulationLinksByEntityId.end())
        {
            return iterator->second;
        }
        else
        {
            return nullptr;
        }
    }

    physx::PxArticulationJointReducedCoordinate* ArticulationLinkComponent::GetDriveJoint() const
    {
        AZ_ErrorOnce("Articulation Link Component", m_driveJoint, "Invalid articulation joint pointer");
        return m_driveJoint;
    }

    physx::PxArticulationJointType::Enum ArticulationLinkComponent::GetArticulationJointType(JointType jointType) const
    {
        switch (jointType)
        {
        case JointType::Fixed:
            return physx::PxArticulationJointType::eFIX;
        case JointType::Hinge:
            return physx::PxArticulationJointType::eREVOLUTE;
        case JointType::Prismatic:
            return physx::PxArticulationJointType::ePRISMATIC;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported joint type");
            return physx::PxArticulationJointType::eFIX;
        }
    }

    physx::PxArticulationAxis::Enum ArticulationLinkComponent::GetArticulationAxis(JointAxis jointAxis) const
    {
        switch (jointAxis)
        {
        case JointAxis::Twist:
            return physx::PxArticulationAxis::eTWIST;
        case JointAxis::SwingY:
            return physx::PxArticulationAxis::eSWING1;
        case JointAxis::SwingZ:
            return physx::PxArticulationAxis::eSWING2;
        case JointAxis::X:
            return physx::PxArticulationAxis::eX;
        case JointAxis::Y:
            return physx::PxArticulationAxis::eY;
        case JointAxis::Z:
            return physx::PxArticulationAxis::eZ;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported joint axis");
            return physx::PxArticulationAxis::eTWIST;
        }
    }

    physx::PxArticulationMotion::Enum ArticulationLinkComponent::GetArticulationMotion(JointMotionType jointMotionType) const
    {
        switch (jointMotionType)
        {
        case JointMotionType::Free:
            return physx::PxArticulationMotion::eFREE;
        case JointMotionType::Limited:
            return physx::PxArticulationMotion::eLIMITED;
        case JointMotionType::Locked:
            return physx::PxArticulationMotion::eLOCKED;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported joint motion type");
            return physx::PxArticulationMotion::eLOCKED;
        }
    }

    JointType ArticulationLinkComponent::GetJointType(physx::PxArticulationJointType::Enum articulationJointType) const
    {
        switch (articulationJointType)
        {
        case physx::PxArticulationJointType::eFIX:
            return JointType::Fixed;
        case physx::PxArticulationJointType::eREVOLUTE:
            return JointType::Hinge;
        case physx::PxArticulationJointType::ePRISMATIC:
            return JointType::Prismatic;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported articulation joint type");
            return JointType::Unsupported;
        }
    }

    JointAxis ArticulationLinkComponent::GetJointAxis(physx::PxArticulationAxis::Enum articulationAxis) const
    {
        switch (articulationAxis)
        {
        case physx::PxArticulationAxis::eTWIST:
            return JointAxis::Twist;
        case physx::PxArticulationAxis::eSWING1:
            return JointAxis::SwingY;
        case physx::PxArticulationAxis::eSWING2:
            return JointAxis::SwingZ;
        case physx::PxArticulationAxis::eX:
            return JointAxis::X;
        case physx::PxArticulationAxis::eY:
            return JointAxis::Y;
        case physx::PxArticulationAxis::eZ:
            return JointAxis::Z;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported articulation axis");
            return JointAxis::Twist;
        }
    }

    JointMotionType ArticulationLinkComponent::GetJointMotionType(physx::PxArticulationMotion::Enum articulationMotion) const
    {
        switch (articulationMotion)
        {
        case physx::PxArticulationMotion::eFREE:
            return JointMotionType::Free;
        case physx::PxArticulationMotion::eLIMITED:
            return JointMotionType::Limited;
        case physx::PxArticulationMotion::eLOCKED:
            return JointMotionType::Locked;
        default:
            AZ_ErrorOnce("Articulation Link Component", false, "unsupported articulation motion");
            return JointMotionType::Locked;
        }
    }

    void ArticulationLinkComponent::SetMotion(JointAxis jointAxis, JointMotionType jointMotionType)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setMotion(GetArticulationAxis(jointAxis), GetArticulationMotion(jointMotionType));
        }
    }

    JointMotionType ArticulationLinkComponent::GetMotion(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            return GetJointMotionType(joint->getMotion(GetArticulationAxis(jointAxis)));
        }
        return JointMotionType::Locked;
    }

    void ArticulationLinkComponent::SetLimit(JointAxis jointAxis, AZStd::pair<float, float> limitPair)
    {
        if (auto* joint = GetDriveJoint())
        {
            physx::PxArticulationLimit limit(limitPair.first, limitPair.second);
            joint->setLimitParams(GetArticulationAxis(jointAxis), limit);
        }
    }

    AZStd::pair<float, float> ArticulationLinkComponent::GetLimit(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto limit = joint->getLimitParams(GetArticulationAxis(jointAxis));
            return { limit.low, limit.high };
        }
        return { -AZ::Constants::FloatMax, AZ::Constants::FloatMax };
    }

    void ArticulationLinkComponent::SetDriveStiffness(JointAxis jointAxis, float stiffness)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.stiffness = stiffness;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetDriveStiffness(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetArticulationAxis(jointAxis));
            return driveParams.stiffness;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetDriveDamping(JointAxis jointAxis, float damping)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.damping = damping;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetDriveDamping(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetArticulationAxis(jointAxis));
            return driveParams.damping;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetMaxForce(JointAxis jointAxis, float maxForce)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.maxForce = maxForce;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    float ArticulationLinkComponent::GetMaxForce(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetArticulationAxis(jointAxis));
            return driveParams.maxForce;
        }
        return AZ::Constants::FloatMax;
    }

    void ArticulationLinkComponent::SetIsAccelerationDrive(JointAxis jointAxis, bool isAccelerationDrive)
    {
        if (auto* joint = GetDriveJoint())
        {
            const auto articulationAxis = GetArticulationAxis(jointAxis);
            auto driveParams = joint->getDriveParams(articulationAxis);
            driveParams.driveType =
                isAccelerationDrive ? physx::PxArticulationDriveType::eACCELERATION : physx::PxArticulationDriveType::eFORCE;
            joint->setDriveParams(articulationAxis, driveParams);
        }
    }

    bool ArticulationLinkComponent::GetIsAccelerationDrive(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            auto driveParams = joint->getDriveParams(GetArticulationAxis(jointAxis));
            return driveParams.driveType == physx::PxArticulationDriveType::eACCELERATION;
        }
        return false;
    }

    void ArticulationLinkComponent::SetDriveTarget(JointAxis jointAxis, float target)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setDriveTarget(GetArticulationAxis(jointAxis), target);
        }
    }

    float ArticulationLinkComponent::GetDriveTarget(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getDriveTarget(GetArticulationAxis(jointAxis));
        }
        return 0.0f;
    }

    void ArticulationLinkComponent::SetDriveTargetVelocity(JointAxis jointAxis, float targetVelocity)
    {
        if (auto* joint = GetDriveJoint())
        {
            joint->setDriveVelocity(GetArticulationAxis(jointAxis), targetVelocity);
        }
    }

    float ArticulationLinkComponent::GetDriveTargetVelocity(JointAxis jointAxis)
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getDriveVelocity(GetArticulationAxis(jointAxis));
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

    float ArticulationLinkComponent::GetFrictionCoefficient()
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

    float ArticulationLinkComponent::GetMaxJointVelocity()
    {
        if (auto* joint = GetDriveJoint())
        {
            return joint->getMaxJointVelocity();
        }
        return 0.0f;
    }
#endif
} // namespace PhysX
