/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LmbrCentral/Ai/NavigationComponentBus.h>

// Cry pathfinding system includes
#include <INavigationSystem.h>

// Component factory
#include <AzCore/RTTI/RTTI.h>

// Component utilization
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>

// Other buses used
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

// Data and containers
#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

// Forward-declare legacy AI interfaces to avoid an include dependency on IPathfinder.h.
class IPathFollower;
class INavPath;
using IPathFollowerPtr = AZStd::shared_ptr<IPathFollower>;
using INavPathPtr = AZStd::shared_ptr<INavPath>;

namespace LmbrCentral
{
    class NavigationComponent;

    /**
    * Represents the response to any pathfinding request.
    * Stores the original request and the current state
    * along with relevant pathfinding data
    */
    class PathfindResponse
        : private AZ::TransformNotificationBus::Handler
        , private AZ::EntityBus::Handler
    {
    public:

        enum class Status
        {
            Uninitialized,
            Initialized,
            WaitingForTargetEntity,
            SearchingForPath,
            PathFound,
            TraversalStarted,
            TraversalInProgress,
            TraversalComplete,
            TraversalCancelled
        };

        using PathfinderRequestId = AZ::u32;

        PathfindResponse();

        void SetOwningComponent(NavigationComponent* navComponent);

        const PathfindRequest& GetRequest() const;
        PathfindRequest::NavigationRequestId GetRequestId() const;
        PathfinderRequestId GetPathfinderRequestId() const;
        void SetPathfinderRequestId(PathfinderRequestId pathfinderRequestId);

        const AZ::Vector3& GetCurrentDestination() const;

        Status GetStatus() const;
        void SetStatus(Status status);

        void SetCurrentPath(const INavPathPtr& currentPath);
        INavPathPtr GetCurrentPath();

        void Reset();

        /**
        * Sets up a response for a newly received request
        */
        void SetupForNewRequest(NavigationComponent* ownerComponent, const PathfindRequest& request);

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus handler
        /// Called when the local transform of the entity has changed.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDeactivated(const AZ::EntityId&) override;
        ////////////////////////////////////////////////////////////////////////

        const AZ::Vector3& GetLastKnownAgentVelocity() const;
        void SetLastKnownAgentVelocity(const AZ::Vector3& newVelocity);

        const AZ::Vector3& GetNextPathPosition() const;
        void SetNextPathPosition(const AZ::Vector3& newPosition);
        
        const AZ::Vector3& GetInflectionPosition() const;
        void SetInflectionPosition(const AZ::Vector3& newPosition);

        IPathFollowerPtr GetPathFollower();
        
        //! Invalid request id
        static const PathfindRequest::NavigationRequestId kInvalidRequestId = 0;

    private:

        //! The request that created this response
        PathfindRequest m_request;

        //////////////////////////////////////////////////////////////////////
        // Response members

        /** Represents the destination that the entity is currently trying to reach.
        * This may be different than the original destination.
        * This change generally happens when the Component Entity is asked to pathfind to
        * another entity that may be moving
        */
        AZ::Vector3 m_currentDestination;

        /** The identifier for this request
        * Does not change for any given request, is used by the requester and other components
        * to identify this pathfinding query uniquely
        */
        PathfindRequest::NavigationRequestId m_requestId;

        /** The identifier used by the pathfinder for queries pertaining to this request
        * May change during the lifetime of any particular request, generally in response to situations that
        * necessitate an update in the path. Following an entity is a prime example, the entity being followed
        * may move and new pathfinding queries may be generated and pushed to the pathfinding system to make sure
        * this entity pathfinds properly. In such a scenario, this id changes.
        */
        PathfinderRequestId m_pathfinderRequestId;

        //! Stores the status of this request
        Status m_responseStatus;

        //! Points back to the navigation component that is handling this request / response
        NavigationComponent* m_navigationComponent;

        //! Last known velocity of the agent
        AZ::Vector3 m_previousAgentVelocity;

        //! Next position in path to travel to
        AZ::Vector3 m_nextPathPosition;

        //! Inflection position (where the path turns) past the next position
        AZ::Vector3 m_inflectionPosition;

        IPathFollowerPtr m_pathFollower;
        INavPathPtr m_currentPath;

        //////////////////////////////////////////////////////////////////////

        class NullPathObstacles : public IPathObstacles
        {
        public:

            bool IsPathIntersectingObstacles(const NavigationMeshID /*meshID*/, const Vec3& /*start*/, const Vec3& /*end*/, float /*radius*/) const override { return false; }
            bool IsPointInsideObstacles(const Vec3& /*position*/) const override { return false; }
            bool IsLineSegmentIntersectingObstaclesOrCloseToThem(const Lineseg& /*linesegToTest*/, float /*maxDistanceToConsiderClose*/) const override { return false; }
        };

        NullPathObstacles m_pathObstacles;

        //! the current request id
        static PathfindRequest::NavigationRequestId s_nextRequestId;
    };

    /*!
    * The Navigation component provides basic pathfinding and path following services to an Entity.
    * It serves AI or other Game Logic by accepting navigation commands and dispatching per-frame
    * movement requests to the Physics component in order to follow the calculated path.
    */
    class NavigationComponent
        : public AZ::Component
        , public NavigationComponentRequestBus::Handler
        , public IAIPathAgent
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(NavigationComponent, "{92284847-9BB3-4CF0-9017-F7E5CEDF3B7B}")
        NavigationComponent();
        friend void PathfindResponse::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/);
        friend void PathfindResponse::OnEntityActivated(const AZ::EntityId&);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // NavigationComponentRequests::Bus::Handler interface implementation
        PathfindRequest::NavigationRequestId FindPath(const PathfindRequest& request) override;
        PathfindRequest::NavigationRequestId FindPathToEntity(AZ::EntityId targetEntityId) override;
        PathfindRequest::NavigationRequestId FindPathToPosition(const AZ::Vector3& destination) override;
        void Stop(PathfindRequest::NavigationRequestId requestId) override;
        float GetAgentSpeed() override;
        void SetAgentSpeed(float agentSpeed) override;
        NavigationComponentRequests::MovementMethod GetAgentMovementMethod() override;
        void SetAgentMovementMethod(NavigationComponentRequests::MovementMethod movementMethod) override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus listener

        /// Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

        //////////////////////////////////////////////////////////////////////////////////

        float GetArrivalDistance() const        { return m_arrivalDistanceThreshold; }
        float GetAgentRadius() const            { return m_agentRadius; }
        bool GetAllowVerticalNavigation() const { return m_allowVerticalNavigation;  }

    private:
#ifdef LMBR_CENTRAL_EDITOR
        AZStd::vector<AZStd::string> PopulateAgentTypeList();
        AZ::u32 HandleAgentTypeChanged();
        float CalculateAgentNavigationRadius(const char* agentTypeName);
        const char* GetDefaultAgentNavigationTypeName();
#endif
        void FindPathImpl();

        // Nav Component settings

        /**
        * Describes the "type" of the Entity for navigation purposes.
        * This type is used to select which navmesh this entity will follow in a scenario where multiple navmeshes are available
        */
        AZStd::string m_agentType;

        //! The speed of at which the agent should move
        float m_agentSpeed;

        //! Describes the radius of this entity for navigation purposes
        float m_agentRadius;

        //! Describes the distance from the end point that an entity needs to be before its movement is to be stopped and considered complete
        float m_arrivalDistanceThreshold;

        //! Describes the distance from its previously known location that a target entity needs to move before a new path is calculated
        float m_repathThreshold;

        //! Indicates whether the entity moves under physics or by modifying the Entity Transform
        bool m_movesPhysically;

        //! Indicates whether the entity uses legacy physics
        bool m_usesLegacyPhysics;

        //! Indicates whether the entity being moved is a character
        bool m_usesCharacterPhysics;

        //! Indicates whether vertical navigation is allowed 
        bool m_allowVerticalNavigation;

        //! Indicates how the agent is moved
        NavigationComponentRequests::MovementMethod m_movementMethod;

        // Runtime data

        //! Stores the transform of the entity this component is attached to
        AZ::Transform m_entityTransform;

        //! Cache the last response (and request) received by the Navigation Component
        PathfindResponse m_lastResponseCache;

        //! The Navigation Agent Type identifier used by the Navigation system
        NavigationAgentTypeID   m_agentTypeId;

        /**
        * Uses the data in "m_lastResponseCache" to request a path from the pathfinder
        */
        PathfindResponse::PathfinderRequestId RequestPath();

        /**
         * Compute required entity velocity and move by setting the position or applying an impulse
         */
        void MoveEntity(float deltaTime);

        /**
        * Resets the Navigation Component and prepares it to process a new pathfinding request
        * Also cancels any pathfinding operations in progress
        */
        void Reset();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("NavigationService", 0xf31e77fe));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
            dependent.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
            dependent.push_back(AZ_CRC("PhysXCharacterControllerService", 0x428de4fa));
        }

        //////////////////////////////////////////////////////////////////////////
        // This component will require the services of the transform component in
        // the short term and the physics component in the long term
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

        AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneStartSimHandler;
    protected:

        void OnPathResult(const MNM::QueuedPathID& requestId, MNMPathRequestResult& result);

        //// IAIPathAgent
        const char* GetPathAgentName() const override { return m_entity->GetName().c_str(); }
        void GetPathAgentNavigationBlockers(NavigationBlockers&, const ::PathfindRequest*) override {}
        AZ::u16 GetPathAgentType() const override { return 0; }
        Vec3 GetPathAgentPos() const override { return Vec3(); }
        float GetPathAgentPassRadius() const override { return 0.f; }
        Vec3 GetPathAgentVelocity() const override { return ZERO; }
        void SetPathToFollow(const char*) override {}
        void SetPathAttributeToFollow(bool) override {}
        void SetPFBlockerRadius(int, float) override {}
        bool GetValidPositionNearby(const Vec3&, Vec3&) const override { return false; }
        bool GetTeleportPosition(Vec3&) const override { return false; }
        class IPathFollower* GetPathFollower() const override { return nullptr; }
        bool IsPointValidForAgent(const Vec3&, AZ::u32) const override { return true; };
        //// ~IAIPathAgent
    };
} // namespace LmbrCentral
