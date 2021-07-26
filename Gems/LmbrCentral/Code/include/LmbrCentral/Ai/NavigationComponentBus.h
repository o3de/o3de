/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ISerialize.h>
#include <IPathfinder.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/bitset.h>
#include <MathConversion.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace LmbrCentral
{
    /**
    * Represents a request as submitted by a user of this component, can be used to configure the pathfinding
    * queries by providing overrides for some default values as entered in the editor.
    */
    class PathfindRequest
    {
    public:

        using NavigationRequestId = AZ::u32;

        ///////////////////////////////////////////////////////////////////////////
        // Destination

        void SetDestinationLocation(const Vec3& destination)
        {
            SetDestinationLocation(LYVec3ToAZVec3(destination));
        }

        void SetDestinationLocation(const AZ::Vector3& destination)
        {
            if (!m_requestMask[RequestBits::DestinationEntity])
            {
                m_destination = destination;
                m_requestMask.set(RequestBits::DestinationPosition);
            }
        }

        AZ::Vector3 GetDestinationLocation() const
        {
            // If there is a target entity or a destination position
            if (m_requestMask[RequestBits::DestinationEntity]
                || m_requestMask[RequestBits::DestinationPosition])
            {
                return m_destination;
            }
            else
            {
                return AZ::Vector3::CreateZero();
            }
        }

        void SetTargetEntityId(AZ::EntityId targetEntity)
        {
            if (m_requestMask[RequestBits::DestinationPosition])
            {
                m_requestMask.reset(RequestBits::DestinationPosition);
            }

            m_targetEntityId = targetEntity;
            m_requestMask.set(RequestBits::DestinationEntity);

            // Get the target entity's position
            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(entityTransform, m_targetEntityId, AZ::TransformBus, GetWorldTM);
            m_destination = entityTransform.GetTranslation();
        }

        AZ::EntityId GetTargetEntityId() const
        {
            if (m_requestMask[RequestBits::DestinationEntity])
            {
                return m_targetEntityId;
            }
            else
            {
                return AZ::EntityId();
            }
        }

        ///////////////////////////////////////////////////////////////////////////

        void SetArrivalDistanceThreshold(float arrivalDistanceThreshold)
        {
            m_arrivalDistanceThreshold = arrivalDistanceThreshold;
            m_requestMask.set(RequestBits::ArrivalDistanceThreshold);
        }

        float GetArrivalDistanceThreshold() const
        {
            return m_arrivalDistanceThreshold;
        }

        PathfindRequest()
            : m_arrivalDistanceThreshold(0)
            , m_destination(AZ::Vector3::CreateZero())
        {
        }

        bool HasTargetEntity() const
        {
            return m_requestMask[RequestBits::DestinationEntity];
        }

        bool HasTargetLocation() const
        {
            return m_requestMask[RequestBits::DestinationPosition];
        }

        bool HasOverrideArrivalDistance() const
        {
            return m_requestMask[RequestBits::ArrivalDistanceThreshold];
        }

    private:

        enum RequestBits
        {
            DestinationPosition,
            DestinationEntity,
            ArrivalDistanceThreshold
        };
        /** Mask used to identify the values that were set / not set in any given request
        * bit 0 : Vector 3 of the destination location
        * bit 1 : AZ::Entity that is to be followed
        * Note: Bit 0 and 1 are mutually exclusive , if both are set bit 1 takes precedence
                So if both a Destination Entity and Location are set, only the Destination Entity is used
        * bit 2 : The override Arrival distance threshold
        */
        AZStd::bitset<8> m_requestMask;

        //! Destination as set by the requester
        AZ::Vector3 m_destination;

        //! Destination entity
        AZ::EntityId m_targetEntityId;

        // Request configuration parameters
        //! Distance from target at which path traversing is considered complete
        float m_arrivalDistanceThreshold;
    };

    /*!
    * NavigationComponentRequests
    * Requests serviced by the Navigation component.
    */
    class NavigationComponentRequests
        : public AZ::ComponentBus
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides (Configuring this Ebus)

        // Only one component on a entity can implement these events
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~NavigationComponentRequests() {}

        enum class MovementMethod
        {
            Transform,
            Physics,
            Custom
        };

        /** Finds a path as per the provided request configuration
        * @param request Allows the issuer of the request to override one, all or none of the pathfinding configuration defaults for this entity
        * @return Returns a unique identifier to this pathfinding request
        */
        virtual PathfindRequest::NavigationRequestId FindPath(const PathfindRequest& /*request*/) { return 0; }

        /** Creates a path finding request to navigate towards the specified entity.
        * @param entityId EntityId of the entity we want to navigate towards.
        * @return Returns a unique identifier to this pathfinding request
        */
        virtual PathfindRequest::NavigationRequestId FindPathToEntity(AZ::EntityId /*entityId*/) { return 0; }

        /** Creates a path finding request to navigate towards the specified position.
        * @param destination World position we want to navigate to.
        * @return Returns a unique identifier to this pathfinding request
        */
        virtual PathfindRequest::NavigationRequestId FindPathToPosition(const AZ::Vector3& /*destination*/) = 0;


        /** Stops all pathfinding operations for the given requestId
        * The id is primarily used to make sure that the request being cancelled is in-fact the
        * request that is currently being processed. If the requestId given is different from the
        * id of the current request, the stop command can be safely ignored
        * @param requestId Used to identify the request that is being cancelled
        */
        virtual void Stop(PathfindRequest::NavigationRequestId requestId) = 0;
     
        /* Returns the current AI Agent's speed
        *  @return Returns the current agent's speed as a float
        */
        virtual float GetAgentSpeed() = 0;

        /* Updates the AI Agent's speed
        *  @param agentSpeed specifies the new agent speed as a float
        */
        virtual void SetAgentSpeed(float agentSpeed) = 0;

        /* Returns the current AI movement method 
        *  @return Returns the current agent's movement method 
        */
        virtual MovementMethod GetAgentMovementMethod() = 0;

        /* Updates the AI Agent's movement method 
        *  @param movementMethod specifies the new agent movement method 
        */
        virtual void SetAgentMovementMethod(MovementMethod movementMethod) = 0;
    };

    // Bus to service the Navigation component event group
    using NavigationComponentRequestBus = AZ::EBus<NavigationComponentRequests>;

    /*!
    * NavigationComponentNotifications
    * Notifications sent by the Navigation component.
    */
    class NavigationComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides (Configuring this Ebus)
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //////////////////////////////////////////////////////////////////////////

        virtual ~NavigationComponentNotifications() {}

        /**
        * Indicates that the pathfinding request has been submitted to the navigation system
        * @param requestId Id of the request for which path is being searched
        */
        virtual void OnSearchingForPath(PathfindRequest::NavigationRequestId /*requestId*/) {}

        /**
        * Indicates that a path has been found for the indicated request
        * @param requestId Id of the request for which path has been found
        * @param currentPath The path that was calculated by the Pathfinder
        * @return boolean value indicating whether this path is to be traversed or not
        */
        virtual bool OnPathFound(PathfindRequest::NavigationRequestId /*requestId*/, AZStd::shared_ptr<const INavPath> /*currentPath*/)
        {
            return true;
        };

        /**
        * Indicates that traversal for the indicated request has started
        * @param requestId Id of the request for which traversal has started
        */
        virtual void OnTraversalStarted(PathfindRequest::NavigationRequestId /*requestId*/) {}

        /**
        * Indicates that traversal for the indicated request has started
        * @param requestId Id of the request for which traversal is in progress
        * @param distanceRemaining remaining distance in this path
        */
        virtual void OnTraversalInProgress(PathfindRequest::NavigationRequestId /*requestId*/, float /*distanceRemaining*/) {}

        /**
        * Indicates that the path for the traversal has updated.  If the 
        * nextPathPosition and inflectionPosition are equal, they represent 
        * the end of the path.
        * @param requestId Id of the request for which traversal is in progress
        * @param nextPathPosition furthest point on the path we can move to without colliding with anything 
        * @param inflectionPosition next point on the path beyond nextPathPoint that deviates from a straight-line path
        */
        virtual void OnTraversalPathUpdate(PathfindRequest::NavigationRequestId /*requestId*/, const AZ::Vector3& /*nextPathPosition*/, const AZ::Vector3& /*inflectionPosition*/) {}

        /**
        * Indicates that traversal for the indicated request has completed successfully
        * @param requestId Id of the request for which traversal has finished
        */
        virtual void OnTraversalComplete(PathfindRequest::NavigationRequestId /*requestId*/) {}

        /**
        * Indicates that traversal for the indicated request was cancelled before successful completion
        * @param requestId Id of the request for which traversal was cancelled
        */
        virtual void OnTraversalCancelled(PathfindRequest::NavigationRequestId /*requestId*/) {}
    };

    using NavigationComponentNotificationBus = AZ::EBus<NavigationComponentNotifications>;
} // namespace LmbrCentral
