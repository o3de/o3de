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

#pragma once

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

namespace Physics
{
    static AZ::Crc32 DefaultPhysicsWorldId = AZ_CRC("AZPhysicalWorld", 0x18f33e24);
    static AZ::Crc32 EditorPhysicsWorldId = AZ_CRC("EditorWorld", 0x8d93f191);

    class RigidBody;
    class WorldBody;
    class WorldEventHandler;
    class ITriggerEventCallback;

    //! Default world configuration.
    class WorldConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldConfiguration, "{3C87DF50-AD02-4746-B19F-8B7453A86243}")
        static void Reflect(AZ::ReflectContext* context);

        virtual ~WorldConfiguration() = default;

        AZ::Crc32 GetCcdVisibility() const;

        AZ::Aabb m_worldBounds = AZ::Aabb::CreateFromMinMax(-AZ::Vector3(1000.f, 1000.f, 1000.f), AZ::Vector3(1000.f, 1000.f, 1000.f));
        float m_maxTimeStep = 1.f / 20.f;
        float m_fixedTimeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
        AZ::Vector3 m_gravity = AZ::Vector3(0.f, 0.f, -9.81f);
        void* m_customUserData = nullptr;
        AZ::u64 m_raycastBufferSize = 32; //!< Maximum number of hits that will be returned from a raycast.
        AZ::u64 m_sweepBufferSize = 32; //!< Maximum number of hits that can be returned from a shapecast.
        AZ::u64 m_overlapBufferSize = 32; //!< Maximum number of overlaps that can be returned from an overlap query.
        bool m_enableCcd = false; //!< Enables continuous collision detection in the world.
        AZ::u32 m_maxCcdPasses = 1; //!< Maximum number of continuous collision detection passes.
        bool m_enableCcdResweep = true; //!< Use a more accurate but more expensive continuous collision detection method.
        bool m_enableActiveActors = false; //!< Enables pxScene::getActiveActors method.
        bool m_enablePcm = true; //!< Enables the persistent contact manifold algorithm to be used as the narrow phase algorithm.
        bool m_kinematicFiltering = true; //!< Enables filtering between kinematic/kinematic  objects.
        bool m_kinematicStaticFiltering = true; //!< Enables filtering between kinematic/static objects.
        float m_bounceThresholdVelocity = 2.0f; //!< Relative velocity below which colliding objects will not bounce.

        bool operator==(const WorldConfiguration& other) const;
        bool operator!=(const WorldConfiguration& other) const;
    private:
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        AZ::u32 OnMaxTimeStepChanged();
        float GetFixedTimeStepMax() const;
    };

    //! Callback for unbounded world queries. These are queries which don't require
    //! building the entire result vector, and so saves memory for very large numbers of hits.
    //! Called with '{ hit }' repeatedly until there are no more hits, then called with '{}', then never called again.
    //! Returns 'true' to continue processing more hits, or 'false' otherwise. If the function ever returns
    //! 'false', it is unspecified if the finalizing call '{}' occurs.
    template<class HitType>
    using HitCallback = AZStd::function<bool(AZStd::optional<HitType>&&)>;

    //! Physics world.
    class World
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Crc32;
        using MutexType = AZStd::recursive_mutex;

        AZ_CLASS_ALLOCATOR(World, AZ::SystemAllocator, 0);
        AZ_RTTI(World, "{61832612-9F5C-4A2E-8E11-00655A6DDDD2}");

        virtual ~World() = default;

        virtual void Update(float deltaTime) = 0;

        //! Start the simulation process. This will spawn physics jobs.
        virtual void StartSimulation(float deltaTime) = 0;

        //! Complete the simulation process. This will wait for the simulation jobs to complete, swap the buffers and process events.
        virtual void FinishSimulation() = 0;

        //! Perform a raycast in the world returning the closest object that intersected.
        virtual RayCastHit RayCast(const RayCastRequest& request) = 0;

        //! Perform a raycast in the world returning all objects that intersected. 
        virtual AZStd::vector<Physics::RayCastHit> RayCastMultiple(const RayCastRequest& request) = 0;

        //! Perform a shapecast in the world returning the closest object that intersected.
        virtual RayCastHit ShapeCast(const ShapeCastRequest& request) = 0;

        //! Perform a shapecast in the world returning all objects that intersected.
        virtual AZStd::vector<RayCastHit> ShapeCastMultiple(const ShapeCastRequest& request) = 0;

        //! Perform a spherecast in the world returning the closest object that intersected.
        Physics::RayCastHit SphereCast(float radius, 
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance, 
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform a spherecast in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> SphereCastMultiple(float radius,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform a boxcast in the world returning the closest object that intersected.
        Physics::RayCastHit BoxCast(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform a boxcast in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> BoxCastMultiple(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform a capsule in the world returning all objects that intersected.
        Physics::RayCastHit CapsuleCast(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform a capsule in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> CapsuleCastMultiple(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup collisionGroup = AzPhysics::CollisionGroup::All,
            FilterCallback filterCallback = nullptr);

        //! Perform an overlap query returning all objects that overlapped.
        virtual AZStd::vector<OverlapHit> Overlap(const OverlapRequest& request) = 0;

        //! Perform an unbounded overlap query, calling the provided callback for each
        virtual void OverlapUnbounded(const OverlapRequest& request, const HitCallback<OverlapHit>& cb) = 0;

        //! Perform an overlap sphere query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapSphere(float radius, const AZ::Transform& pose, OverlapFilterCallback filterCallback = nullptr);

        //! Perform an overlap box query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapBox(const AZ::Vector3& dimensions, const AZ::Transform& pose, OverlapFilterCallback filterCallback = nullptr);

        //! Perform an overlap capsule query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapCapsule(float height, float radius, const AZ::Transform& pose, OverlapFilterCallback filterCallback = nullptr);

        //! Registers a pair of world bodies for which collisions should be suppressed.
        virtual void RegisterSuppressedCollision(const WorldBody& body0, const WorldBody& body1) = 0;

        //! Unregisters a pair of world bodies for which collisions should be suppressed.
        virtual void UnregisterSuppressedCollision(const WorldBody& body0, const WorldBody& body1) = 0;

        virtual void AddBody(WorldBody& body) = 0;
        virtual void RemoveBody(WorldBody& body) = 0;

        virtual AZ::Crc32 GetNativeType() const { return AZ::Crc32(); }
        virtual void* GetNativePointer() const { return nullptr; }

        virtual void SetSimFunc(std::function<void(void*)> func) = 0;

        virtual void SetEventHandler(WorldEventHandler* eventHandler) = 0;

        virtual AZ::Vector3 GetGravity() const = 0;
        virtual void SetGravity(const AZ::Vector3& gravity) = 0;

        virtual void SetMaxDeltaTime(float maxDeltaTime) = 0;
        virtual void SetFixedDeltaTime(float fixedDeltaTime) = 0;

        virtual void DeferDelete(AZStd::unique_ptr<WorldBody> worldBody) = 0;

        //! @brief Similar to SetEventHandler, relevant for Touch Bending.
        //!
        //! SetEventHandler is useful to catch onTrigger events when the bodies
        //! involved were created with the standard physics Components attached to
        //! entities. On the other hand, this method was added since Touch Bending, and it is useful
        //! for the touch bending simulator to catch onTrigger events of Actors that
        //! don't have valid AZ:EntityId.
        //! 
        //! @param triggerCallback Pointer to the callback object that will get the On
        //! @returns Nothing.
        virtual void SetTriggerEventCallback(ITriggerEventCallback* triggerCallback) = 0;

        //! Returns this world's ID.
        virtual AZ::Crc32 GetWorldId() const = 0;
    };

    using WorldRequestBus = AZ::EBus<World>;
    using WorldRequests = World;

    //! Broadcasts notifications for a specific Physics::World.
    //! This bus is addressed on the id of the world.
    //! Subscribe to the bus using Physics::DefaultPhysicsWorldId for the default world,
    //! or Physics::EditorPhysicsWorldId for the editor world.
    class WorldNotifications
        : public AZ::EBusTraits
    {
    public:
        enum PhysicsTickOrder
        {
            Physics = 0, //!< The physics system itself. Should always be first.
            Animation = 100, //!< Animation system (ragdolls).
            Components = 200, //!< C++ components (force region).
            Scripting = 300, //!< Scripting systems (script canvas).
            Audio = 400, //!< Audio systems (occlusion).
            Default = 1000 //!< All other systems (Game code).
        };

        virtual ~WorldNotifications() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Crc32;
        using MutexType = AZStd::recursive_mutex;

        //! Broadcast before each world simulation tick.
        //! Each tick may include multiple fixed timestep subticks. For example, if the fixed timestep was set at 10ms,
        //! and a game tick of 25ms occurred, 2 fixed timestep subticks would be performed this tick, and the
        //! remaining 5ms would be accumulated for the subsequent tick. So, in this example, the events fired would be:
        //! OnPrePhysicsTick (for the whole 25ms tick)
        //! OnPrePhysicsSubtick (for the first 10ms subtick)
        //! OnPostPhysicsSubtick (for the first 10ms subtick)
        //! OnPrePhysicsSubtick (for the second 10ms subtick)
        //! OnPostPhysicsSubtick (for the second 10ms subtick)
        //! OnPostPhysicsTick (for the whole 25ms tick)
        //! @param deltaTime The duration of the tick as a whole (which may contain multiple fixed timestep subticks).
        virtual void OnPrePhysicsTick([[maybe_unused]] float deltaTime) {}

        //! Broadcast before each fixed timestep subtick.
        //! @param fixedDeltaTime The duration for fixed timestep subticks.
        virtual void OnPrePhysicsSubtick([[maybe_unused]] float fixedDeltaTime) {}

        //! Broadcast after each fixed timestep subtick.
        //! @param fixedDeltaTime The duration for fixed timestep subticks.
        virtual void OnPostPhysicsSubtick([[maybe_unused]] float fixedDeltaTime) {}

        //! Broadcast after each world simulation tick.
        //! Each tick may include multiple fixed timestep subticks.
        //! @param deltaTime The duration of the tick as a whole (which may contain multiple fixed timestep subticks).
        virtual void OnPostPhysicsTick([[maybe_unused]] float deltaTime) {}

        //! Event fired when the gravity for a world is changed.
        //! @param gravity The world's new value for gravity acceleration.
        virtual void OnGravityChanged([[maybe_unused]] const AZ::Vector3& gravity) {}

        //! Specified the order in which a handler receives WorldNotification events.
        //! Users subscribing to this bus can override this function to change
        //! the order events are received relative to other systems.
        //! @return a value specifying this handler'S relative order.
        virtual int GetPhysicsTickOrder() { return Default; }

        //! Determines the order in which handlers receive events. 
        struct BusHandlerOrderCompare
        {
            //! Compare function used to control physics update order.
            //! @param left an instance of the handler to compare.
            //! @param right another instance of the handler to compare.
            //! @return True if the priority of left is greater than right, false otherwise.
            AZ_FORCE_INLINE bool operator()(WorldNotifications* left, WorldNotifications* right) const 
            { 
                return left->GetPhysicsTickOrder() < right->GetPhysicsTickOrder();
            }
        };
    };

    using WorldNotificationBus = AZ::EBus<WorldNotifications>;
} // namespace Physics
