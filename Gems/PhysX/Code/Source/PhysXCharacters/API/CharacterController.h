/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzFramework/Physics/Character.h>
#include <PhysX/UserDataTypes.h>
#include <PxPhysicsAPI.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace PhysX
{
    static const float epsilon = 1e-3f;

    enum class SlopeBehaviour
    {
        PreventClimbing,
        ForceSliding
    };

    /// Allows PhysX specific character controller properties that are not included in the generic configuration.
    class CharacterControllerConfiguration
        : public Physics::CharacterConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterControllerConfiguration, AZ::SystemAllocator);
        AZ_RTTI(CharacterControllerConfiguration, "{23A8DFD6-7DA4-4CB3-BBD3-7FB58DEE6F9D}", Physics::CharacterConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        CharacterControllerConfiguration() = default;
        CharacterControllerConfiguration(const CharacterControllerConfiguration&) = default;
        virtual ~CharacterControllerConfiguration() = default;

        SlopeBehaviour m_slopeBehaviour = SlopeBehaviour::PreventClimbing; ///< Behaviour on surfaces above maximum slope.
        float m_contactOffset = 0.1f; ///< Extra distance outside the controller used to give smoother contact resolution.
        float m_scaleCoefficient = 0.8f; ///< Scalar coefficient used to scale the controller, usually slightly smaller than 1.
    };

    /// Manages callbacks for character controller collision filtering, collision notifications, and handling riding on objects.
    class CharacterControllerCallbackManager
        : public physx::PxControllerFilterCallback
        , public physx::PxQueryFilterCallback
        , public physx::PxUserControllerHitReport
        , public physx::PxControllerBehaviorCallback
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterControllerCallbackManager, AZ::SystemAllocator);
        AZ_TYPE_INFO(CharacterControllerCallbackManager, "93C7DEA8-98E6-4C07-96B7-D215800D0ECB");

        /// Determines whether this controller should be obstructed by other controllers or able to move through them.
        using ControllerFilter = AZStd::function<bool(const physx::PxController&, const physx::PxController&)>;

        /// Called when another object has been identified as potentially obstructing the controller's path, but before
        /// an exact intersection test has been performed (if the ePREFILTER flag is set in the controller's filter flags).
        using ObjectPreFilter = AZStd::function<physx::PxQueryHitType::Enum(const physx::PxFilterData&,
            const physx::PxShape*,const physx::PxRigidActor*, physx::PxHitFlags&)>;

        /// Called after an exact intersection test has identified another object as obstructing the controller's path
        /// (if the ePOSTFILTER flag is set in the controller's filter flags).
        using ObjectPostFilter = AZStd::function<physx::PxQueryHitType::Enum(const physx::PxFilterData&,
            const physx::PxQueryHit&)>;

        /// Called when the controller collides with another object.
        using OnShapeHit = AZStd::function<void(const physx::PxControllerShapeHit&)>;

        /// Called when the controller collides with another controller.
        using OnControllerHit = AZStd::function<void(const physx::PxControllersHit&)>;

        /// Called when the controller collides with an obstacle.
        using OnObstacleHit = AZStd::function<void(const physx::PxControllerObstacleHit&)>;

        /// Determines whether the controller should be able to ride on other objects or should slide.
        using ObjectRidingBehavior = AZStd::function<physx::PxControllerBehaviorFlags(const physx::PxShape&,
            const physx::PxActor&)>;

        /// Determines whether the controller should slide when standing on another character.
        using ControllerRidingBehavior = AZStd::function<physx::PxControllerBehaviorFlags(const physx::PxController&)>;

        /// Determines whether the controller should be able to ride on obstacles or should slide.
        using ObstacleRidingBehavior = AZStd::function<physx::PxControllerBehaviorFlags(const physx::PxObstacle&)>;

        /// Sets the function which determines whether this controller should be obstructed by other controllers or
        /// able to move through them.
        void SetControllerFilter(ControllerFilter controllerFilter);

        /// Sets the function which is called when another object has been identified as potentially obstructing the
        /// controller's path, but before an exact intersection test has been performed.
        /// The function will only be called if the ePREFILTER flag is set in the controller's filter flags.
        void SetObjectPreFilter(ObjectPreFilter objectPreFilter);

        /// Sets the function which is called after an exact intersection test has identified another object as
        /// obstructing the controller's path.
        /// The function will only be called if the ePOSTFILTER flag is set in the controller's filter flags.
        void SetObjectPostFilter(ObjectPostFilter objectPostFilter);

        /// Sets the function which is called when the controller collides with another object.
        void SetOnShapeHit(OnShapeHit onShapeHit);

        /// Sets the function which is called when the controller collides with another controller.
        void SetOnControllerHit(OnControllerHit onControllerHit);

        /// Sets the function which is called when the controller collides with an obstacle.
        void SetOnObstacleHit(OnObstacleHit onObstacleHit);

        /// Sets the function which determines whether the controller should be able to ride on other objects or should slide.
        void SetObjectRidingBehavior(ObjectRidingBehavior objectRidingBehavior);

        /// Sets the function which determines whether the controller should slide when standing on another character.
        void SetControllerRidingBehavior(ControllerRidingBehavior controllerRidingBehavior);

        /// Sets the function which determines whether the controller should be able to ride on obstacles or should slide.
        void SetObstacleRidingBehavior(ObstacleRidingBehavior obstacleRidingBehavior);

        // physx::PxControllerFilterCallback
        bool filter(const physx::PxController& controllerA, const physx::PxController& controllerB) override;

        // physx::PxQueryFilterCallback
        physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape,
            const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override;

#if (PX_PHYSICS_VERSION_MAJOR == 5)
        physx::PxQueryHitType::Enum postFilter(
            const physx::PxFilterData& filterData,
            const physx::PxQueryHit& hit,
            const physx::PxShape* shape,
            const physx::PxRigidActor* actor) override;
#endif

        physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override;

        // physx::PxUserControllerHitReport
        void onShapeHit(const physx::PxControllerShapeHit& hit) override;
        void onControllerHit(const physx::PxControllersHit& hit) override;
        void onObstacleHit(const physx::PxControllerObstacleHit& hit) override;

        // physx::PxControllerBehaviorCallback
        physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor) override;
        physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController& controller) override;
        physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle& obstacle) override;

    private:
        ControllerFilter m_controllerFilter;
        ObjectPreFilter m_objectPreFilter;
        ObjectPostFilter m_objectPostFilter;
        OnShapeHit m_onShapeHit;
        OnControllerHit m_onControllerHit;
        OnObstacleHit m_onObstacleHit;
        ObjectRidingBehavior m_objectRidingBehavior;
        ControllerRidingBehavior m_controllerRidingBehavior;
        ObstacleRidingBehavior m_obstacleRidingBehavior;
    };

    class CharacterController
        : public Physics::Character
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterController, AZ::SystemAllocator);
        AZ_RTTI(PhysX::CharacterController, "{A75A7D19-BC21-4F7E-A3D9-05031D2DFC94}", Physics::Character);
        static void Reflect(AZ::ReflectContext* context);

        CharacterController() = default;
        CharacterController(physx::PxController* pxController,
            AZStd::unique_ptr<CharacterControllerCallbackManager> callbackManager,
            AzPhysics::SceneHandle sceneHandle);
        ~CharacterController();

        //! Character Controller can be only enabled and disabled once after creation.
        //! And after being disabled it cannot be enabled again, it has to be destroyed and re-created.
        //! This is because the way PhysX controller works, it doesn't allow the state of having physics disabled, so being enabled/disabled is linked to be created/destroyed.
        //! @{
        void EnablePhysics(const Physics::CharacterConfiguration& configuration);
        void DisablePhysics();
        //! @}

        CharacterControllerCallbackManager* GetCallbackManager();
        void SetFilterFlags(physx::PxQueryFlags filterFlags);

        // Physics::Character
        AZ::Vector3 GetBasePosition() const override;
        void SetBasePosition(const AZ::Vector3& position) override;
        AZ::Vector3 GetCenterPosition() const override;
        float GetStepHeight() const override;
        void SetStepHeight(float stepHeight) override;
        AZ::Vector3 GetUpDirection() const override;
        void SetUpDirection(const AZ::Vector3& upDirection) override;
        float GetSlopeLimitDegrees() const override;
        void SetSlopeLimitDegrees(float slopeLimitDegrees) override;
        float GetMaximumSpeed() const override;
        void SetMaximumSpeed(float maximumSpeed) override;
        AZ::Vector3 GetVelocity() const override;
        AzPhysics::CollisionLayer GetCollisionLayer() const override;
        AzPhysics::CollisionGroup GetCollisionGroup() const override;
        void SetCollisionLayer(const AzPhysics::CollisionLayer& layer) override;
        void SetCollisionGroup(const AzPhysics::CollisionGroup& group) override;
        AZ::Crc32 GetColliderTag() const override;
        void AddVelocityForTick(const AZ::Vector3& velocity) override;
        void AddVelocityForPhysicsTimestep(const AZ::Vector3& velocity) override;
        void ResetRequestedVelocityForTick() override;
        void ResetRequestedVelocityForPhysicsTimestep() override;
        void Move(const AZ::Vector3& requestedMovement, float deltaTime) override;
        void ApplyRequestedVelocity(float deltaTime) override;
        void SetRotation(const AZ::Quaternion& rotation) override;
        void AttachShape(AZStd::shared_ptr<Physics::Shape> shape) override;

        // AzPhysics::SimulatedBody
        AZ::EntityId GetEntityId() const override;
        AzPhysics::Scene* GetScene() override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

        // CharacterController specific
        void Resize(float height);
        float GetHeight() const;
        void SetHeight(float height);
        float GetRadius() const;
        void SetRadius(float radius);
        float GetHalfSideExtent() const;
        void SetHalfSideExtent(float halfSideExtent);
        float GetHalfForwardExtent() const;
        void SetHalfForwardExtent(float halfForwardExtent);

    private:
        void SetFilterDataAndShape(const Physics::CharacterConfiguration& characterConfig);
        void SetUserData(const Physics::CharacterConfiguration& characterConfig);
        void SetActorName(const AZStd::string& name = "Character Controller");
        void SetMinimumMovementDistance(float distance);
        void SetTag(const AZStd::string& tag);
        void CreateShadowBody(const Physics::CharacterConfiguration& configuration);
        void DestroyShadowBody();
        void RemoveControllerFromScene();
        void UpdateFilterLayerAndGroup(AzPhysics::CollisionLayer collisionLayer, AzPhysics::CollisionGroup collisionGroup);

        physx::PxController* m_pxController = nullptr; ///< The underlying PhysX controller.
        float m_minimumMovementDistance = 0.0f; ///< To avoid jittering, the controller will not attempt to move distances below this.
        AZ::Vector3 m_requestedVelocityForTick = AZ::Vector3::CreateZero(); ///< Used to accumulate velocity requests which last for a tick.
        AZ::Vector3 m_requestedVelocityForPhysicsTimestep =
            AZ::Vector3::CreateZero(); ///< Used to accumulate velocity requests which last for a physics timestep.
        AZ::Vector3 m_observedVelocity = AZ::Vector3::CreateZero(); ///< Velocity observed in the simulation, may not match desired.
        PhysX::ActorData m_actorUserData; ///< Used to populate the user data on the PxActor associated with the controller.
        physx::PxFilterData m_filterData; ///< Controls filtering for collisions with other objects and scene queries.
        physx::PxControllerFilters m_pxControllerFilters; ///< Controls which objects the controller interacts with when moving.
        AZStd::shared_ptr<Physics::Shape> m_shape; ///< The generic physics API shape associated with the controller.
        AzPhysics::RigidBody* m_shadowBody = nullptr; ///< A kinematic-synchronised rigid body used to store additional colliders.
        AzPhysics::SimulatedBodyHandle m_shadowBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!<A handle to the shadow body.
        AZStd::string m_name = "Character Controller"; ///< Name to set on the PhysX actor associated with the controller.
        AZ::Crc32 m_colliderTag; ///< Tag used to identify the collider associated with the controller.
        float m_maximumSpeed = 100.0f; ///< If the accumulated requested velocity for a tick exceeds this magnitude, it will be clamped.
        AZStd::unique_ptr<CharacterControllerCallbackManager>
            m_callbackManager; ///< Manages callbacks for collision filtering, collision notifications, and handling riding on objects.
        AZ::Quaternion m_orientation; ///< The orientation of the character.
    };

    //! Example implementation of controller-controller filtering callback.
    //! This example causes controllers to impede each other's movement based on their collision filters.
    bool CollisionLayerBasedControllerFilter(const physx::PxController& controllerA, const physx::PxController& controllerB);

    //! Example implementation of controller-object filtering callback.
    //! This example causes static and kinematic bodies to impede the character based on collision layers.
    physx::PxQueryHitType::Enum CollisionLayerBasedObjectPreFilter(
        const physx::PxFilterData& filterData,
        const physx::PxShape* shape,
        const physx::PxRigidActor* actor,
        [[maybe_unused]] physx::PxHitFlags& queryFlags);
} // namespace PhysX
