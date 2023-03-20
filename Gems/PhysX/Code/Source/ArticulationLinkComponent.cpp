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
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
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

    ArticulationLinkComponent::ArticulationLinkComponent(AzPhysics::SceneHandle sceneHandle)
        : m_attachedSceneHandle(sceneHandle)
    {
        InitPhysicsTickHandler();
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
                ->Field("RelativeTransform", &ArticulationLinkData::m_relativeTransform)
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

    void ArticulationLinkComponent::Reflect(AZ::ReflectContext* context)
    {
        ArticulationLinkData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkComponent, AZ::Component>()
                ->Version(1)
                ->Field("ArticulationLinkData", &ArticulationLinkComponent::m_articulationLinkData)
                ;
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

    void ArticulationLinkComponent::CreateRigidBody()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ArticulationLinkComponent::DestroyRigidBody()
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
        PHYSX_SCENE_WRITE_LOCK(pxScene);

        m_articulation->release();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
            m_staticRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    bool ArticulationLinkComponent::IsRootArticulation() const
    {
        AZ::EntityId parentId = GetEntity()->GetTransform()->GetParentId();
        if (parentId.IsValid())
        {
            AZ::Entity* parentEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

            if (parentEntity && parentEntity->FindComponent<ArticulationLinkComponent>())
            {
                return false;
            }
        }

        return true;
    }

    void ArticulationLinkComponent::Activate()
    {
        if (IsRootArticulation())
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);
            }

            CreateArticulation();
        }
    }

    void ArticulationLinkComponent::CreateArticulation()
    {
        using namespace physx;

        PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        m_articulation = pxPhysics->createArticulationReducedCoordinate();

        CreateChildArticulationLinks(nullptr, *m_articulationLinkData.get());

        // Add articulation to the scene
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        PxScene* pxScene = static_cast<PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_WRITE_LOCK(pxScene);
        pxScene->addArticulation(*m_articulation);
    }

    void ArticulationLinkComponent::CreateChildArticulationLinks(
        physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData)
    {
        using namespace physx;

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

                const bool hasNonUniformScale = (AZ::NonUniformScaleRequestBus::FindFirstHandler(GetEntityId()) != nullptr);
                AZStd::vector<AZStd::shared_ptr<Physics::Shape>> assetShapes;
                Utils::CreateShapesFromAsset(
                    *physicsAssetShapeConfiguration,
                    colliderConfiguration,
                    hasNonUniformScale,
                    physicsAssetShapeConfiguration->m_subdivisionLevel,
                    assetShapes);

                physicsShape = assetShapes[0];
            }
            else
            {
                Physics::SystemRequestBus::BroadcastResult(
                    physicsShape, &Physics::SystemRequests::CreateShape, colliderConfiguration, *shapeConfiguration);
            }

            m_articulationShapes.emplace_back(physicsShape);
        }

        PxTransform thisLinkTransform;
        if (parentLink)
        {
            PxTransform parentLinkTransform = parentLink->getGlobalPose();
            PxTransform thisLinkRelativeTransform = PxMathConvert(thisLinkData.m_relativeTransform);
            thisLinkTransform = parentLinkTransform * thisLinkRelativeTransform;
        }
        else
        {
            thisLinkTransform = PxMathConvert(GetEntity()->GetTransform()->GetWorldTM());
        }

        PxArticulationLink* thisLink = m_articulation->createLink(parentLink, thisLinkTransform);
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
            PxArticulationJointReducedCoordinate* inboundJoint = thisLink->getInboundJoint();
            // TODO: Set the values for joints from thisLinkData
            inboundJoint->setJointType(PxArticulationJointType::eFIX);
            inboundJoint->setParentPose(PxMathConvert(thisLinkData.m_relativeTransform));
            inboundJoint->setChildPose(PxTransform(PxIdentity));
        }

        if (physicsShape)
        {
            thisLink->attachShape(*(PxShape*)physicsShape->GetNativePointer());
        }

        for (const auto& childLink : thisLinkData.m_childLinks)
        {
            CreateChildArticulationLinks(thisLink, *childLink);
        }
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
    
    void ArticulationLinkComponent::PostPhysicsTick(float /*fixedDeltaTime*/)
    {
        using namespace physx;

        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        PxScene* pxScene = static_cast<PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxScene);

        if (m_articulation->isSleeping())
        {
            return;
        }

        constexpr size_t MaxArticulationLinks = 16;

        PxArticulationLink* links[MaxArticulationLinks] = {0};
        m_articulation->getLinks(links, MaxArticulationLinks);

        const PxU32 linksNum = m_articulation->getNbLinks();

        for (PxU32 linkIndex = 0; linkIndex < linksNum; ++linkIndex)
        {
            PxArticulationLink* link = links[linkIndex];
            PxTransform pxGlobalPose = link->getGlobalPose();
            AZ::Transform globalTransform = PxMathConvert(pxGlobalPose);
            ActorData* linkActorData = Utils::GetUserData(link);
            if (linkActorData)
            {
                AZ::EntityId linkEntityId = linkActorData->GetEntityId();
                AZ::TransformBus::Event(linkEntityId, &AZ::TransformBus::Events::SetWorldTM, globalTransform);
            }
        }
    }

    void ArticulationLinkComponent::Deactivate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        DestroyRigidBody();
    }

    void ArticulationLinkComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {

    }
} // namespace PhysX
