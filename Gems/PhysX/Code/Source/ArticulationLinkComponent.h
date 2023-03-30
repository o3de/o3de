/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/UserDataTypes.h>
#include <Source/RigidBody.h>
#include <Source/Articulation/ArticulationLinkConfiguration.h>

namespace physx
{
    class PxArticulationReducedCoordinate;
    class PxArticulationJointReducedCoordinate;
}

namespace PhysX
{
    //! Maximum number of articulation links in a single articulation.
    constexpr size_t MaxArticulationLinks = 16;

    //! Configuration data for an articulation joint.
    struct ArticulationJointData
    {
        AZ_CLASS_ALLOCATOR(ArticulationJointData, AZ::SystemAllocator);
        AZ_TYPE_INFO(ArticulationJointData, "{F7ADD440-07DA-437F-AF77-B747327B9336}");
        static void Reflect(AZ::ReflectContext* context);

        ArticulationJointType m_jointType = ArticulationJointType::Fix;
        AZ::Transform m_jointLeadLocalFrame = AZ::Transform::CreateIdentity();
        AZ::Transform m_jointFollowerLocalFrame = AZ::Transform::CreateIdentity();
        JointGenericProperties m_genericProperties;
        JointLimitProperties m_limits;
        JointMotorProperties m_motor;
    };

    //! Configuration data for an articulation link. Contains references to child links.
    struct ArticulationLinkData
    {
        AZ_CLASS_ALLOCATOR(ArticulationLinkData, AZ::SystemAllocator);
        AZ_TYPE_INFO(ArticulationLinkData, "{C9862FF7-FFAC-4A49-A51D-A555C4303F74}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfiguration;
        Physics::ColliderConfiguration m_colliderConfiguration;
        AZ::EntityId m_entityId;
        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity(); //<! Local transform with respect to the parent entity.
        AzPhysics::RigidBodyConfiguration m_config; //!< Generic properties from AzPhysics.
        RigidBodyConfiguration
            m_physxSpecificConfig; //!< Properties specific to PhysX which might not have exact equivalents in other physics engines.
        ArticulationJointData m_articulationJointData;

        AZStd::vector<AZStd::shared_ptr<ArticulationLinkData>> m_childLinks;
    };

    //! Component implementing articulation link logic.
    class ArticulationLinkComponent final
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ArticulationLinkComponent, ArticulationLinkComponentTypeId);

        ArticulationLinkComponent();
        explicit ArticulationLinkComponent(ArticulationLinkConfiguration& config);
        ~ArticulationLinkComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AZStd::shared_ptr<ArticulationLinkData> m_articulationLinkData;
        ArticulationLinkConfiguration m_config;

    private:
        bool IsRootArticulation() const;

        void CreateArticulation();
        void CreateChildArticulationLinks(physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData);
        void DestroyArticulation();

        void InitPhysicsTickHandler();
        void PostPhysicsTick(float fixedDeltaTime);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationsBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        physx::PxArticulationReducedCoordinate* m_articulation = nullptr;
        physx::PxArticulationJointReducedCoordinate* m_driveJoint = nullptr;
        bool m_tempClosing = true;

        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
        AzPhysics::SimulatedBodyEvents::OnSyncTransform::Handler m_activeBodySyncTransformHandler;

        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> m_articulationShapes;
        AZStd::vector<AZStd::shared_ptr<ActorData>> m_linksActorData; // TODO: Move to AzPhysics::ArticulationLink
    };

    //! Utility function for detecting if the current entity is the root of articulation
    template<typename ArticulationComponentClass>
    bool IsRootArticulationEntity(AZ::Entity* entity)
    {
        AZ::EntityId parentId = entity->GetTransform()->GetParentId();
        if (parentId.IsValid())
        {
            AZ::Entity* parentEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

            if (parentEntity && parentEntity->FindComponent<ArticulationComponentClass>())
            {
                return false;
            }
        }

        return true;
    }
} // namespace PhysX
