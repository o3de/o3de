/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Articulation.h>

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/MathConversion.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>

namespace PhysX
{
    void ArticulationLinkData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkData>()
                ->Version(1)
                ->Field("LinkConfiguration", &ArticulationLinkData::m_articulationLinkConfiguration)
                ->Field("ShapeColliderPair", &ArticulationLinkData::m_shapeColliderConfiguration)
                ->Field("LocalTransform", &ArticulationLinkData::m_localTransform)
                ->Field("JointLeadLocalFrame", &ArticulationLinkData::m_jointLeadLocalFrame)
                ->Field("JointFollowerLocalFrame", &ArticulationLinkData::m_jointFollowerLocalFrame)
                ->Field("ChildLinks", &ArticulationLinkData::m_childLinks)
            ;
        }
    }

    void ArticulationLink::SetPxArticulationLink(physx::PxArticulationLink* pxLink)
    {
        m_pxLink = pxLink;
    }

    void ArticulationLink::SetupFromLinkData(const ArticulationLinkData& thisLinkData)
    {
        const ArticulationLinkConfiguration& configuration
            = thisLinkData.m_articulationLinkConfiguration;

        m_actorData = ActorData(m_pxLink);
        m_actorData.SetEntityId(configuration.m_entityId);
        m_actorData.SetArticulationLink(this);

        m_pxLink->setName(configuration.m_debugName.c_str());

        m_pxLink->setCMassLocalPose(physx::PxTransform(PxMathConvert(configuration.m_centerOfMassOffset)));
        m_pxLink->setMass(configuration.m_mass);
        m_pxLink->setLinearDamping(configuration.m_linearDamping);
        m_pxLink->setAngularDamping(configuration.m_angularDamping);
        m_pxLink->setMaxAngularVelocity(configuration.m_maxAngularVelocity);
        m_pxLink->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, configuration.m_gravityEnabled == false);

        AddCollisionShape(thisLinkData);
    }

    void ArticulationLink::AddCollisionShape(const ArticulationLinkData& thisLinkData)
    {
        const Physics::ColliderConfiguration* colliderConfiguration = thisLinkData.m_shapeColliderConfiguration.first.get();
        const Physics::ShapeConfiguration* shapeConfiguration = thisLinkData.m_shapeColliderConfiguration.second.get();

        if (shapeConfiguration && colliderConfiguration)
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
                    *colliderConfiguration,
                    hasNonUniformScale,
                    physicsAssetShapeConfiguration->m_subdivisionLevel,
                    assetShapes);

                if (!assetShapes.empty())
                {
                    m_physicsShape = assetShapes[0];
                    AZ_Warning(
                        "PhysX",
                        assetShapes.size() == 1,
                        "Articulation %s has a link with physics mesh with more than 1 shape",
                        m_pxLink->getName());
                }
            }
            else
            {
                m_physicsShape =
                    AZ::Interface<Physics::System>::Get()->CreateShape(*colliderConfiguration, *shapeConfiguration);
            }
        }

        if (m_physicsShape)
        {
            m_pxLink->attachShape(*static_cast<physx::PxShape*>(m_physicsShape->GetNativePointer()));
        }
    }

    AzPhysics::SceneQueryHit ArticulationLink::RayCast(const AzPhysics::RayCastRequest& request)
    {
        return PhysX::SceneQueryHelpers::ClosestRayHitAgainstPxRigidActor(request, m_pxLink);
    }

    AZ::Crc32 ArticulationLink::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::ArticulationLink;
    }

    void* ArticulationLink::GetNativePointer() const
    {
        return m_pxLink;
    }

    AZ::EntityId ArticulationLink::GetEntityId() const
    {
        return m_actorData.GetEntityId();
    }

    AZ::Transform ArticulationLink::GetTransform() const
    {
        if (m_pxLink)
        {
            PHYSX_SCENE_READ_LOCK(m_pxLink->getScene());
            return PxMathConvert(m_pxLink->getGlobalPose());
        }
        return AZ::Transform::CreateIdentity();
    }

    void ArticulationLink::SetTransform([[maybe_unused]] const AZ::Transform& transform)
    {
        // Not allowed for articulation link
        AZ_Error("PhysX", false, "Cannot set transform to articulation link.");
    }

    AZ::Vector3 ArticulationLink::GetPosition() const
    {
        if (m_pxLink)
        {
            PHYSX_SCENE_READ_LOCK(m_pxLink->getScene());
            return PxMathConvert(m_pxLink->getGlobalPose().p);
        }
        return AZ::Vector3::CreateZero();

    }

    AZ::Quaternion ArticulationLink::GetOrientation() const
    {
        if (m_pxLink)
        {
            PHYSX_SCENE_READ_LOCK(m_pxLink->getScene());
            return PxMathConvert(m_pxLink->getGlobalPose().q);
        }
        return AZ::Quaternion::CreateZero();
    }

    AZ::Aabb ArticulationLink::GetAabb() const
    {
        if (m_pxLink)
        {
            PHYSX_SCENE_READ_LOCK(m_pxLink->getScene());
            return PxMathConvert(m_pxLink->getWorldBounds(1.0f));
        }
        return AZ::Aabb::CreateNull();
    }

    ArticulationLink* CreateArticulationLink([[maybe_unused]] const ArticulationLinkConfiguration* articulationConfig)
    {
        ArticulationLink* articulationLink = aznew ArticulationLink;
        return articulationLink;
    }

} // namespace PhysX

