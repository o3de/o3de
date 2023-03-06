/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ArticulatedBodyComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzCore/Component/Entity.h>

#include <PhysX/ColliderComponentBus.h>
#include "AzFramework/Physics/Material/PhysicsMaterialManager.h"
#include "PhysX/Material/PhysXMaterial.h"
#include "System/PhysXSystem.h"

namespace PhysX
{
    // Definitions are put in .cpp so we can have AZStd::unique_ptr<T> member with forward declared T in the header
    // This causes AZStd::unique_ptr<T> ctor/dtor to be generated when full type info is available
    ArticulatedBodyComponent::ArticulatedBodyComponent() = default;
    ArticulatedBodyComponent::~ArticulatedBodyComponent() = default;

    ArticulatedBodyComponent::ArticulatedBodyComponent(AzPhysics::SceneHandle sceneHandle)
        : m_attachedSceneHandle(sceneHandle)
    {

    }

    ArticulationLinkData::~ArticulationLinkData()
    {
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
                ->Field("ChildLinks", &ArticulationLinkData::m_childLinks)
                ->Field("PhysxSpecificConfig", &ArticulationLinkData::m_physxSpecificConfig)
                ->Field("GenericProperties", &ArticulationLinkData::m_genericProperties)
                ->Field("Limits", &ArticulationLinkData::m_limits)
                ->Field("Motor", &ArticulationLinkData::m_motor)
            ;
        }
    }

    void ArticulationLinkData::Reset()
    {
        *this = ArticulationLinkData();
    }

    void ArticulatedBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        ArticulationLinkData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulatedBodyComponent, AZ::Component>()
                ->Version(1)
                ->Field("ArticulationLinkData", &ArticulatedBodyComponent::m_articulationLinkData)
                ;
        }
    }

    void ArticulatedBodyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void ArticulatedBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void ArticulatedBodyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void ArticulatedBodyComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void ArticulatedBodyComponent::CreateRigidBody()
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> allshapes;
        ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&allshapes](ColliderComponentRequests* handler)
        {
            const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes = handler->GetShapes();
            allshapes.insert(allshapes.end(), shapes.begin(), shapes.end());
            return true;
        });
        configuration.m_colliderAndShapeData = allshapes;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            configuration.m_startSimulationEnabled = false; // enable physics will enable this when called.
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ArticulatedBodyComponent::DestroyRigidBody()
    {
        m_articulation->release();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
            m_staticRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    bool ArticulatedBodyComponent::IsRootArticulation() const
    {
        AZ::EntityId parentId = GetEntity()->GetTransform()->GetParentId();
        if (parentId.IsValid())
        {
            AZ::Entity* parentEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

            if (parentEntity && parentEntity->FindComponent<ArticulatedBodyComponent>())
            {
                return false;
            }
        }

        return true;
    }

    void ArticulatedBodyComponent::Activate()
    {
        if (IsRootArticulation())
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
            CreateArticulation();
        }
    }

    void ArticulatedBodyComponent::CreateArticulation()
    {
        using namespace physx;

        PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        PxScene* pxScene = static_cast<PxScene*>(scene->GetNativePointer());
        pxScene;
        m_articulation = pxPhysics->createArticulationReducedCoordinate();
    }

    
    void ArticulatedBodyComponent::UpdateArticulationHierarchy()
    {
        AZStd::vector<AZ::EntityId> children = GetEntity()->GetTransform()->GetChildren();
        for (auto childId : children)
        {
            AZ::Entity* childEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, childId);

            if (!childEntity)
            {
                continue;
            }

            if (auto* articulatedComponent = childEntity->FindComponent<ArticulatedBodyComponent>())
            {
                articulatedComponent->UpdateArticulationHierarchy();
                m_articulationLinkData.m_childLinks.emplace_back(&articulatedComponent->m_articulationLinkData);
            }
        }
    }

    void ArticulatedBodyComponent::Deactivate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        DestroyRigidBody();
    }

    void ArticulatedBodyComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {

    }

    void ArticulatedBodyComponent::SetupSample()
    {
        using namespace physx;

        PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        PxScene* pxScene = static_cast<PxScene*>(scene->GetNativePointer());

        const PxReal runnerLength = 2.f;
        const PxReal placementDistance = 1.8f;

        const PxReal cosAng = (placementDistance) / (runnerLength);

        const PxReal angle = PxAcos(cosAng);

        const PxReal sinAng = PxSin(angle);
        const PxQuat leftRot(-angle, PxVec3(1.f, 0.f, 0.f));
        const PxQuat rightRot(angle, PxVec3(1.f, 0.f, 0.f));

        m_articulation = pxPhysics->createArticulationReducedCoordinate();

        PxArticulationLink* base = m_articulation->createLink(NULL, PxTransform(PxVec3(0.f, 0.25f, 50.f)));

        const auto defaultMaterial = AZ::Interface<Physics::MaterialManager>::Get()->GetDefaultMaterial();
        const physx::PxMaterial* pxMaterial = static_cast<PhysX::Material*>(defaultMaterial.get())->GetPxMaterial();

        PxRigidActorExt::createExclusiveShape(*base, PxBoxGeometry(0.5f, 0.25f, 1.5f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*base, 3.f);

        m_articulation->setSolverIterationCounts(32);

        PxArticulationLink* leftRoot = m_articulation->createLink(base, PxTransform(PxVec3(0.f, 0.55f, -0.9f)));
        PxRigidActorExt::createExclusiveShape(*leftRoot, PxBoxGeometry(0.5f, 0.05f, 0.05f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*leftRoot, 1.f);

        PxArticulationLink* rightRoot = m_articulation->createLink(base, PxTransform(PxVec3(0.f, 0.55f, 0.9f)));
        PxRigidActorExt::createExclusiveShape(*rightRoot, PxBoxGeometry(0.5f, 0.05f, 0.05f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*rightRoot, 1.f);

        PxArticulationJointReducedCoordinate* joint = static_cast<PxArticulationJointReducedCoordinate*>(leftRoot->getInboundJoint());
        joint->setJointType(PxArticulationJointType::eFIX);
        joint->setParentPose(PxTransform(PxVec3(0.f, 0.25f, -0.9f)));
        joint->setChildPose(PxTransform(PxVec3(0.f, -0.05f, 0.f)));

        // Set up the drive joint...
        m_driveJoint = static_cast<PxArticulationJointReducedCoordinate*>(rightRoot->getInboundJoint());
        m_driveJoint->setJointType(PxArticulationJointType::ePRISMATIC);
        m_driveJoint->setMotion(PxArticulationAxis::eZ, PxArticulationMotion::eLIMITED);
        m_driveJoint->setLimit(PxArticulationAxis::eZ, -1.4f, 0.2f);
        m_driveJoint->setDrive(PxArticulationAxis::eZ, 100000.f, 0.f, PX_MAX_F32);

        m_driveJoint->setParentPose(PxTransform(PxVec3(0.f, 0.25f, 0.9f)));
        m_driveJoint->setChildPose(PxTransform(PxVec3(0.f, -0.05f, 0.f)));

        const PxU32 linkHeight = 3;
        PxArticulationLink *currLeft = leftRoot, *currRight = rightRoot;

        PxQuat rightParentRot(PxIdentity);
        PxQuat leftParentRot(PxIdentity);
        for (PxU32 i = 0; i < linkHeight; ++i)
        {
            const PxVec3 pos(0.5f, 0.55f + 0.1f * (1 + i), 0.f);
            PxArticulationLink* leftLink =
                m_articulation->createLink(currLeft, PxTransform(pos + PxVec3(0.f, sinAng * (2 * i + 1), 0.f), leftRot));
            PxRigidActorExt::createExclusiveShape(*leftLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *pxMaterial);
            PxRigidBodyExt::updateMassAndInertia(*leftLink, 1.f);

            const PxVec3 leftAnchorLocation = pos + PxVec3(0.f, sinAng * (2 * i), -0.9f);

            joint = static_cast<PxArticulationJointReducedCoordinate*>(leftLink->getInboundJoint());
            joint->setParentPose(PxTransform(currLeft->getGlobalPose().transformInv(leftAnchorLocation), leftParentRot));
            joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, -1.f), rightRot));
            joint->setJointType(PxArticulationJointType::eREVOLUTE);

            leftParentRot = leftRot;

            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
            joint->setLimit(PxArticulationAxis::eTWIST, -PxPi, angle);

            PxArticulationLink* rightLink =
                m_articulation->createLink(currRight, PxTransform(pos + PxVec3(0.f, sinAng * (2 * i + 1), 0.f), rightRot));
            PxRigidActorExt::createExclusiveShape(*rightLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *pxMaterial);
            PxRigidBodyExt::updateMassAndInertia(*rightLink, 1.f);

            const PxVec3 rightAnchorLocation = pos + PxVec3(0.f, sinAng * (2 * i), 0.9f);

            joint = static_cast<PxArticulationJointReducedCoordinate*>(rightLink->getInboundJoint());
            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setParentPose(PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation), rightParentRot));
            joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, 1.f), leftRot));
            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
            joint->setLimit(PxArticulationAxis::eTWIST, -angle, PxPi);

            rightParentRot = rightRot;

            PxD6Joint* d6joint = PxD6JointCreate(*pxPhysics, leftLink, PxTransform(PxIdentity), rightLink, PxTransform(PxIdentity));

            d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
            d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
            d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

            currLeft = rightLink;
            currRight = leftLink;
        }

        PxArticulationLink* leftTop =
            m_articulation->createLink(currLeft, currLeft->getGlobalPose().transform(PxTransform(PxVec3(-0.5f, 0.f, -1.0f), leftParentRot)));
        PxRigidActorExt::createExclusiveShape(*leftTop, PxBoxGeometry(0.5f, 0.05f, 0.05f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*leftTop, 1.f);

        PxArticulationLink* rightTop = m_articulation->createLink(
            currRight, currRight->getGlobalPose().transform(PxTransform(PxVec3(-0.5f, 0.f, 1.0f), rightParentRot)));
        PxRigidActorExt::createExclusiveShape(*rightTop, PxCapsuleGeometry(0.05f, 0.8f), *pxMaterial);
        // PxRigidActorExt::createExclusiveShape(*rightTop, PxBoxGeometry(0.5f, 0.05f, 0.05f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*rightTop, 1.f);

        joint = static_cast<PxArticulationJointReducedCoordinate*>(leftTop->getInboundJoint());
        joint->setParentPose(PxTransform(PxVec3(0.f, 0.f, -1.f), currLeft->getGlobalPose().q.getConjugate()));
        joint->setChildPose(PxTransform(PxVec3(0.5f, 0.f, 0.f), leftTop->getGlobalPose().q.getConjugate()));
        joint->setJointType(PxArticulationJointType::eREVOLUTE);
        joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
        // joint->setDrive(PxArticulationAxis::eTWIST, 0.f, 10.f, PX_MAX_F32);

        joint = static_cast<PxArticulationJointReducedCoordinate*>(rightTop->getInboundJoint());
        joint->setParentPose(PxTransform(PxVec3(0.f, 0.f, 1.f), currRight->getGlobalPose().q.getConjugate()));
        joint->setChildPose(PxTransform(PxVec3(0.5f, 0.f, 0.f), rightTop->getGlobalPose().q.getConjugate()));
        joint->setJointType(PxArticulationJointType::eREVOLUTE);
        joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
        // joint->setDrive(PxArticulationAxis::eTWIST, 0.f, 10.f, PX_MAX_F32);

        currLeft = leftRoot;
        currRight = rightRoot;

        rightParentRot = PxQuat(PxIdentity);
        leftParentRot = PxQuat(PxIdentity);

        for (PxU32 i = 0; i < linkHeight; ++i)
        {
            const PxVec3 pos(-0.5f, 0.55f + 0.1f * (1 + i), 0.f);
            PxArticulationLink* leftLink =
                m_articulation->createLink(currLeft, PxTransform(pos + PxVec3(0.f, sinAng * (2 * i + 1), 0.f), leftRot));
            PxRigidActorExt::createExclusiveShape(*leftLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *pxMaterial);
            PxRigidBodyExt::updateMassAndInertia(*leftLink, 1.f);

            const PxVec3 leftAnchorLocation = pos + PxVec3(0.f, sinAng * (2 * i), -0.9f);

            joint = static_cast<PxArticulationJointReducedCoordinate*>(leftLink->getInboundJoint());
            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setParentPose(PxTransform(currLeft->getGlobalPose().transformInv(leftAnchorLocation), leftParentRot));
            joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, -1.f), rightRot));

            leftParentRot = leftRot;

            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
            joint->setLimit(PxArticulationAxis::eTWIST, -PxPi, angle);

            PxArticulationLink* rightLink =
                m_articulation->createLink(currRight, PxTransform(pos + PxVec3(0.f, sinAng * (2 * i + 1), 0.f), rightRot));
            PxRigidActorExt::createExclusiveShape(*rightLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *pxMaterial);
            PxRigidBodyExt::updateMassAndInertia(*rightLink, 1.f);

            const PxVec3 rightAnchorLocation = pos + PxVec3(0.f, sinAng * (2 * i), 0.9f);

            /*joint = PxD6JointCreate(getPhysics(), currRight, PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation)),
            rightLink, PxTransform(PxVec3(0.f, 0.f, 1.f)));*/

            joint = static_cast<PxArticulationJointReducedCoordinate*>(rightLink->getInboundJoint());
            joint->setParentPose(PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation), rightParentRot));
            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, 1.f), leftRot));
            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
            joint->setLimit(PxArticulationAxis::eTWIST, -angle, PxPi);

            rightParentRot = rightRot;

            PxD6Joint* d6joint = PxD6JointCreate(*pxPhysics, leftLink, PxTransform(PxIdentity), rightLink, PxTransform(PxIdentity));

            d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
            d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
            d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

            currLeft = rightLink;
            currRight = leftLink;
        }

        PxD6Joint* d6joint =
            PxD6JointCreate(*pxPhysics, currLeft, PxTransform(PxVec3(0.f, 0.f, -1.f)), leftTop, PxTransform(PxVec3(-0.5f, 0.f, 0.f)));

        d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
        d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

        d6joint = PxD6JointCreate(*pxPhysics, currRight, PxTransform(PxVec3(0.f, 0.f, 1.f)), rightTop, PxTransform(PxVec3(-0.5f, 0.f, 0.f)));

        d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
        d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

        const PxTransform topPose(PxVec3(0.f, leftTop->getGlobalPose().p.y + 0.15f, 0.f));

        PxArticulationLink* top = m_articulation->createLink(leftTop, topPose);
        PxRigidActorExt::createExclusiveShape(*top, PxBoxGeometry(0.5f, 0.1f, 1.5f), *pxMaterial);
        PxRigidBodyExt::updateMassAndInertia(*top, 1.f);

        joint = static_cast<PxArticulationJointReducedCoordinate*>(top->getInboundJoint());
        joint->setJointType(PxArticulationJointType::eFIX);
        joint->setParentPose(PxTransform(PxVec3(0.f, 0.0f, 0.f)));
        joint->setChildPose(PxTransform(PxVec3(0.f, -0.15f, -0.9f)));

        pxScene->addArticulation(*m_articulation);

        for (PxU32 i = 0; i < m_articulation->getNbLinks(); ++i)
        {
            PxArticulationLink* link;
            m_articulation->getLinks(&link, 1, i);

            link->setLinearDamping(0.2f);
            link->setAngularDamping(0.2f);

            link->setMaxAngularVelocity(20.f);
            link->setMaxLinearVelocity(100.f);

            //if (link != top)
            //{
            //    for (PxU32 b = 0; b < link->getNbShapes(); ++b)
            //    {
            //        PxShape* shape;
            //        link->getShapes(&shape, 1, b);

            //        shape->setSimulationFilterData(PxFilterData(0, 0, 1, 0));
            //    }
            //}
        }
    }


} // namespace PhysX
