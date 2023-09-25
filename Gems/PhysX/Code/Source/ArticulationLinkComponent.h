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
#include <PhysX/ArticulationJointBus.h>
#include <PhysX/ArticulationSensorBus.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/UserDataTypes.h>
#include <Source/Articulation.h>
#include <Source/Articulation/ArticulationLinkConfiguration.h>

#include <AzFramework/Physics/PhysicsScene.h>

namespace physx
{
    class PxArticulationReducedCoordinate;
    class PxArticulationJointReducedCoordinate;
}

namespace PhysX
{
    //! Component implementing articulation link logic.
    class ArticulationLinkComponent final
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
#if (PX_PHYSICS_VERSION_MAJOR == 5)
        , public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public PhysX::ArticulationJointRequestBus::Handler
        , public PhysX::ArticulationSensorRequestBus::Handler
#endif
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

#if (PX_PHYSICS_VERSION_MAJOR == 5)
        // ArticulationJointRequestBus overrides ...
        void SetMotion(ArticulationJointAxis jointAxis, ArticulationJointMotionType jointMotionType) override;
        ArticulationJointMotionType GetMotion(ArticulationJointAxis jointAxis) const override;
        void SetLimit(ArticulationJointAxis jointAxis, AZStd::pair<float, float> limitPair) override;
        AZStd::pair<float, float> GetLimit(ArticulationJointAxis jointAxis) const override;
        void SetDriveStiffness(ArticulationJointAxis jointAxis, float stiffness) override;
        float GetDriveStiffness(ArticulationJointAxis jointAxis) const override;
        void SetDriveDamping(ArticulationJointAxis jointAxis, float damping) override;
        float GetDriveDamping(ArticulationJointAxis jointAxis) const override;
        void SetMaxForce(ArticulationJointAxis jointAxis, float maxForce) override;
        float GetMaxForce(ArticulationJointAxis jointAxis) const override;
        void SetIsAccelerationDrive(ArticulationJointAxis jointAxis, bool isAccelerationDrive) override;
        bool IsAccelerationDrive(ArticulationJointAxis jointAxis) const override;
        void SetDriveTarget(ArticulationJointAxis jointAxis, float target) override;
        float GetDriveTarget(ArticulationJointAxis jointAxis) const override;
        void SetDriveTargetVelocity(ArticulationJointAxis jointAxis, float targetVelocity) override;
        float GetDriveTargetVelocity(ArticulationJointAxis jointAxis) const override;
        void SetFrictionCoefficient(float frictionCoefficient) override;
        float GetFrictionCoefficient() const override;
        void SetMaxJointVelocity(float maxJointVelocity) override;
        float GetMaxJointVelocity() const override;
        float GetJointPosition(ArticulationJointAxis jointAxis) const override;
        float GetJointVelocity(ArticulationJointAxis jointAxis) const override;
        // ArticulationSensorRequestBus overrides ...
        AZ::Transform GetSensorTransform(AZ::u32 sensorIndex) const override;
        void SetSensorTransform(AZ::u32 sensorIndex, const AZ::Transform& sensorTransform) override;
        AZ::Vector3 GetForce(AZ::u32 sensorIndex) const override;
        AZ::Vector3 GetTorque(AZ::u32 sensorIndex) const override;
        const AzPhysics::SimulatedBody* GetSimulatedBodyConst() const;
        void FillSimulatedBodyHandle();

        // SimulatedBodyComponentRequests overrides ...
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;
#endif


        physx::PxArticulationLink* GetArticulationLink(const AZ::EntityId entityId);
        const AZStd::vector<AZ::u32> GetSensorIndices(const AZ::EntityId entityId);
        const physx::PxArticulationJointReducedCoordinate* GetDriveJoint() const;
        physx::PxArticulationJointReducedCoordinate* GetDriveJoint();
        AZStd::vector<AzPhysics::SimulatedBodyHandle> GetSimulatedBodyHandles() const;
        AZStd::shared_ptr<ArticulationLinkData> m_articulationLinkData;
        ArticulationLinkConfiguration m_config;

    private:
        bool IsRootArticulation() const;
        const AZ::Entity* GetArticulationRootEntity() const;

        void CreateArticulation();
        void DestroyArticulation();
        void InitPhysicsTickHandler();
#if (PX_PHYSICS_VERSION_MAJOR == 5)

        const physx::PxArticulationSensor* GetSensor(AZ::u32 sensorIndex) const;
        physx::PxArticulationSensor* GetSensor(AZ::u32 sensorIndex);

        void SetRootSpecificProperties(const ArticulationLinkConfiguration& rootLinkConfiguration);

        void CreateChildArticulationLinks(physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData);

        void AddCollisionShape(const ArticulationLinkData& thisLinkData, ArticulationLink* articulationLink);

        void PostPhysicsTick(float fixedDeltaTime);
#endif

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationsBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        physx::PxArticulationReducedCoordinate* m_articulation = nullptr;

        physx::PxArticulationLink* m_link = nullptr;
        physx::PxArticulationJointReducedCoordinate* m_driveJoint = nullptr;

        AZStd::vector<AZ::u32> m_sensorIndices;

        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        AZStd::vector<AzPhysics::SimulatedBodyHandle> m_articulationLinks;
        AzPhysics::SimulatedBodyHandle m_bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
        AzPhysics::SystemEvents::OnSceneRemovedEvent::Handler m_sceneRemovedHandler;

        using EntityIdArticulationLinkPair = AZStd::pair<AZ::EntityId, physx::PxArticulationLink*>;
        AZStd::unordered_map<AZ::EntityId, physx::PxArticulationLink*> m_articulationLinksByEntityId;
        using EntityIdSensorIndexListPair = AZStd::pair<AZ::EntityId, AZStd::vector<AZ::u32>>;
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::u32>> m_sensorIndicesByEntityId;
    };

    //! Utility function for detecting if the current entity is the root of articulation.
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
