/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "NavigationComponent.h"
#include "EditorNavigationUtil.h"

#include <IPathfinder.h>
#include <MathConversion.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Interface/Interface.h>
#ifdef LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#endif

namespace LmbrCentral
{
    // Behavior Context forwarder for NavigationComponentNotificationBus
    class BehaviorNavigationComponentNotificationBusHandler : public NavigationComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER_WITH_DOC(BehaviorNavigationComponentNotificationBusHandler,"{6D060202-06BA-470E-8F6B-E1982360C752}",AZ::SystemAllocator
            , OnSearchingForPath, ({"RequestId","Navigation request Id"})
            , OnTraversalStarted, ({"RequestId","Navigation request Id"})
            , OnTraversalPathUpdate, ({"RequestId","Navigation request Id"},{"NextPathPosition","Next path position"},{"InflectionPosition","Next inflection position"})
            , OnTraversalInProgress, ({"RequestId","Navigation request Id"},{"Distance","Distance remaining"})
            , OnTraversalComplete, ({"RequestId","Navigation request Id"})
            , OnTraversalCancelled, ({"RequestId","Navigation request Id"}));

        void OnSearchingForPath(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnSearchingForPath, requestId);
        }

        void OnTraversalStarted(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalStarted, requestId);
        }

        void OnTraversalPathUpdate(PathfindRequest::NavigationRequestId requestId, const AZ::Vector3& nextPathPosition, const AZ::Vector3& inflectionPosition) override
        {
            Call(FN_OnTraversalPathUpdate, requestId, nextPathPosition, inflectionPosition);
        }

        void OnTraversalInProgress(PathfindRequest::NavigationRequestId requestId, float distanceRemaining) override
        {
            Call(FN_OnTraversalInProgress, requestId, distanceRemaining);
        }

        void OnTraversalComplete(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalComplete, requestId);
        }

        void OnTraversalCancelled(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalCancelled, requestId);
        }
    };


    PathfindRequest::NavigationRequestId PathfindResponse::s_nextRequestId = kInvalidRequestId;

    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    bool NavigationComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 4)
        {
            // "Move Physically" changed to "Movement Method"
            constexpr const char* movePhysicallyName = "Move Physically";
            constexpr const char* movementMethodName = "Movement Method";
            int movePhysicallyIndex = classElement.FindElement(AZ_CRC(movePhysicallyName));
            if (movePhysicallyIndex != -1)
            {
                bool movePhysically = false;
                classElement.GetSubElement(movePhysicallyIndex).GetData(movePhysically);
                classElement.RemoveElement(movePhysicallyIndex);

                if (movePhysically)
                {
                    classElement.AddElementWithData(context, movementMethodName, NavigationComponentRequests::MovementMethod::Physics);
                }
                else
                {
                    classElement.AddElementWithData(context, movementMethodName, NavigationComponentRequests::MovementMethod::Transform);
                }
            }
        }

        return true;
    }

    void NavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NavigationComponent, AZ::Component>()
                ->Version(4, &NavigationComponentVersionConverter)
                ->Field("Agent Type", &NavigationComponent::m_agentType)
                ->Field("Agent Speed", &NavigationComponent::m_agentSpeed)
                ->Field("Agent Radius", &NavigationComponent::m_agentRadius)
                ->Field("Arrival Distance Threshold", &NavigationComponent::m_arrivalDistanceThreshold)
                ->Field("Repath Threshold", &NavigationComponent::m_repathThreshold)
                ->Field("Movement Method", &NavigationComponent::m_movementMethod)
                ->Field("Allow Vertical Navigation", &NavigationComponent::m_allowVerticalNavigation);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NavigationComponent>(
                    "Navigation", "The Navigation component provides basic pathfinding and pathfollowing services to an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Navigation.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Navigation.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ai/navigation/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_agentSpeed, "Agent Speed",
                        "The speed of the agent while navigating ")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &NavigationComponent::m_agentType, "Agent Type",
                        "Describes the type of the Entity for navigation purposes. ")
#ifdef LMBR_CENTRAL_EDITOR
                    ->Attribute(AZ::Edit::Attributes::StringList, &NavigationComponent::PopulateAgentTypeList)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &NavigationComponent::HandleAgentTypeChanged)
#endif

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_agentRadius, "Agent Radius",
                        "Radius of this Navigation Agent")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_arrivalDistanceThreshold,
                        "Arrival Distance Threshold", "Describes the distance from the end point that an entity needs to be before its movement is to be stopped and considered complete")
                    ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_repathThreshold,
                        "Repath Threshold", "Describes the distance from its previously known location that a target entity needs to move before a new path is calculated")
                    ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &NavigationComponent::m_movementMethod,
                        "Movement Method", "Indicates the method used to move the entity, the default 'Transform' method will modify the position using the TransformBus")
                    ->Attribute(AZ::Edit::Attributes::EnumValues,
                        AZStd::vector<AZ::Edit::EnumConstant<NavigationComponentRequests::MovementMethod>>
                        {
                            AZ::Edit::EnumConstant<NavigationComponentRequests::MovementMethod>(NavigationComponentRequests::MovementMethod::Transform,
                                "Transform"),
                                AZ::Edit::EnumConstant<NavigationComponentRequests::MovementMethod>(NavigationComponentRequests::MovementMethod::Physics,
                                "Physics"),
                                AZ::Edit::EnumConstant<NavigationComponentRequests::MovementMethod>(NavigationComponentRequests::MovementMethod::Custom,
                                "Custom")
                        })

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &NavigationComponent::m_allowVerticalNavigation,
                        "Allow Vertical Navigation", "Indicates whether vertical navigation is allowed or if navigation is constrained to the X and Y plane");

            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<NavigationComponentRequestBus>("NavigationComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Event("FindPathToEntity", &NavigationComponentRequestBus::Events::FindPathToEntity, { { {"EntityId","The entity to follow"} } })
                ->Event("FindPathToPosition", &NavigationComponentRequestBus::Events::FindPathToPosition, { { {"Position","The position to navigate to"} } })
                ->Event("Stop", &NavigationComponentRequestBus::Events::Stop, { { {"RequestId","The request Id of the navigation process to stop"} } })
                ->Event("GetAgentSpeed", &NavigationComponentRequestBus::Events::GetAgentSpeed)
                ->Event("SetAgentSpeed", &NavigationComponentRequestBus::Events::SetAgentSpeed, { { {"Speed","The agent speed in meters per second"} } })
                ->Event("GetAgentMovementMethod", &NavigationComponentRequestBus::Events::GetAgentMovementMethod)
                ->Event("SetAgentMovementMethod", &NavigationComponentRequestBus::Events::SetAgentMovementMethod, { { {"Method","The movement method: Transform, Physics or Custom"} } });

            behaviorContext->EBus<NavigationComponentNotificationBus>("NavigationComponentNotificationBus")
                ->Handler<BehaviorNavigationComponentNotificationBusHandler>();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Pathfind response
    //! @param navComponent The navigation component being serviced by this response
    PathfindResponse::PathfindResponse()
        : m_requestId(kInvalidRequestId)
        , m_pathfinderRequestId(kInvalidRequestId)
        , m_responseStatus(PathfindResponse::Status::Uninitialized)
        , m_navigationComponent(nullptr)
        , m_previousAgentVelocity(AZ::Vector3::CreateZero())
        , m_pathFollower(nullptr)
        , m_nextPathPosition(AZ::Vector3::CreateZero())
        , m_inflectionPosition(AZ::Vector3::CreateZero())
    {
    }

    void PathfindResponse::SetupForNewRequest(NavigationComponent* ownerComponent, const PathfindRequest& request)
    {
        AZ_Assert(ownerComponent, "Invalid parent component.");

        m_navigationComponent = ownerComponent;
        m_request = request;
        m_requestId = ++s_nextRequestId;
        m_currentDestination = request.GetDestinationLocation();
        m_previousAgentVelocity = AZ::Vector3::CreateZero();

        // Reset State information
        m_pathfinderRequestId = kInvalidRequestId;
        m_currentPath.reset();

        // Setup pathfollower instance.
        PathFollowerParams params;
        params.endAccuracy = m_navigationComponent->GetArrivalDistance();
        params.normalSpeed = m_navigationComponent->GetAgentSpeed();
        params.passRadius = m_navigationComponent->GetAgentRadius();
        params.minSpeed = params.normalSpeed * 0.8f;
        params.maxSpeed = params.normalSpeed * 1.2f;
        params.stopAtEnd = true;
        params.use2D = !m_navigationComponent->GetAllowVerticalNavigation();
        m_pathFollower = nullptr;

        // Disconnect from any notifications from earlier requests
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();

        SetStatus(Status::Initialized);

        // If this request is to follow a moving entity then connect to the transform notification bus for the target
        if (m_request.HasTargetEntity())
        {
            SetStatus(Status::WaitingForTargetEntity);
            AZ::TransformNotificationBus::Handler::BusConnect(m_request.GetTargetEntityId());
            AZ::EntityBus::Handler::BusConnect(m_request.GetTargetEntityId());
        }
    }

    void PathfindResponse::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (!m_navigationComponent)
        {
            return;
        }

        if ((m_responseStatus == Status::TraversalStarted) || (m_responseStatus == Status::TraversalInProgress))
        {
            auto delta = (world.GetTranslation() - GetCurrentDestination()).GetLength();

            if (delta > m_navigationComponent->m_repathThreshold)
            {
                m_currentDestination = world.GetTranslation();
                SetPathfinderRequestId(m_navigationComponent->RequestPath());
            }
        }
    }

    void PathfindResponse::OnEntityActivated(const AZ::EntityId&)
    {
        // Get the target entity's position
        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(entityTransform, m_request.GetTargetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        m_currentDestination = entityTransform.GetTranslation();

        if (m_responseStatus == Status::WaitingForTargetEntity)
        {
            AZ::EntityBus::Handler::BusDisconnect();
            m_navigationComponent->FindPathImpl();
        }
    }

    void PathfindResponse::OnEntityDeactivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void PathfindResponse::SetOwningComponent(NavigationComponent* navComponent)
    {
        m_navigationComponent = navComponent;
    }

    const PathfindRequest& PathfindResponse::GetRequest() const
    {
        return m_request;
    }

    PathfindRequest::NavigationRequestId PathfindResponse::GetRequestId() const
    {
        return m_requestId;
    }

    PathfindResponse::PathfinderRequestId PathfindResponse::GetPathfinderRequestId() const
    {
        return m_pathfinderRequestId;
    }

    void PathfindResponse::SetPathfinderRequestId(PathfinderRequestId pathfinderRequestId)
    {
        m_pathfinderRequestId = pathfinderRequestId;
    }

    const AZ::Vector3& PathfindResponse::GetCurrentDestination() const
    {
        return m_currentDestination;
    }

    PathfindResponse::Status PathfindResponse::GetStatus() const
    {
        return m_responseStatus;
    }

    void PathfindResponse::SetStatus(Status status)
    {
        m_responseStatus = status;

        // If the traversal was cancelled or completed and the request was following an entity
        if ((status >= Status::TraversalComplete) && m_request.HasTargetEntity())
        {
            // Disconnect from any notifications on the transform bust
            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }
    }

    void PathfindResponse::SetCurrentPath(const INavPathPtr& currentPath)
    {
        m_currentPath = currentPath;

        if (m_pathFollower)
        {
            m_pathFollower->AttachToPath(m_currentPath.get());
        }
    }

    INavPathPtr PathfindResponse::GetCurrentPath()
    {
        return m_currentPath;
    }

    void PathfindResponse::Reset()
    {
        PathfindResponse::Status lastResponseStatus = GetStatus();

        // If there is already a Request being serviced
        if (lastResponseStatus > PathfindResponse::Status::Initialized
            && lastResponseStatus < PathfindResponse::Status::TraversalComplete)
        {
            // If the pathfinding request was still being serviced by the pathfinder
            if (lastResponseStatus >= PathfindResponse::Status::SearchingForPath
                && lastResponseStatus <= PathfindResponse::Status::TraversalInProgress)
            {
                // and If the request was a valid one
                if (GetRequestId() != PathfindResponse::kInvalidRequestId)
                {
                    // Cancel that request with the pathfinder
                    IMNMPathfinder* pathFinder = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
                    if (pathFinder)
                    {
                        pathFinder->CancelPathRequest(GetPathfinderRequestId());
                    }
                }
            }

            // Indicate that traversal on this request was cancelled
            SetStatus(PathfindResponse::Status::TraversalCancelled);

            // Inform every listener on this entity that traversal was cancelled.
            NavigationComponentNotificationBus::Event(m_navigationComponent->GetEntityId(),
                &NavigationComponentNotificationBus::Events::OnTraversalCancelled, GetRequestId());
        }

        m_pathFollower.reset();
    }

    const AZ::Vector3& PathfindResponse::GetLastKnownAgentVelocity() const
    {
        return m_previousAgentVelocity;
    }

    void PathfindResponse::SetLastKnownAgentVelocity(const AZ::Vector3& newVelocity)
    {
        m_previousAgentVelocity = newVelocity;
    }

    const AZ::Vector3& PathfindResponse::GetNextPathPosition() const
    {
        return m_nextPathPosition;
    }

    void PathfindResponse::SetNextPathPosition(const AZ::Vector3& newPosition)
    {
        m_nextPathPosition = newPosition;
    }

    const AZ::Vector3& PathfindResponse::GetInflectionPosition() const
    {
        return m_inflectionPosition;
    }

    void PathfindResponse::SetInflectionPosition(const AZ::Vector3& newPosition)
    {
        m_inflectionPosition = newPosition;
    }

    IPathFollowerPtr PathfindResponse::GetPathFollower()
    {
        return m_pathFollower;
    }

    //////////////////////////////////////////////////////////////////////////

    NavigationComponent::NavigationComponent()
        : m_agentSpeed(1.f)
        , m_agentRadius(4.f)
        , m_arrivalDistanceThreshold(0.25f)
        , m_repathThreshold(1.f)
        , m_movementMethod(NavigationComponentRequests::MovementMethod::Physics)
        , m_usesCharacterPhysics(false)
        , m_allowVerticalNavigation(false)
        , m_sceneStartSimHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            float fixedDeltatime
            )
            {
                this->MoveEntity(fixedDeltatime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components))
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    void NavigationComponent::Init()
    {
        m_lastResponseCache.SetOwningComponent(this);
        const INavigationSystem* navigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (navigationSystem)
        {
            m_agentTypeId = navigationSystem->GetAgentTypeID(m_agentType.c_str());
        }
    }

    void NavigationComponent::Activate()
    {
        const AZ::EntityId& entityId = GetEntityId();

        NavigationComponentRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);

        if (m_movementMethod == NavigationComponentRequests::MovementMethod::Physics)
        {
            bool usesLegacyCharacterPhysics = false;


            const bool usesAZCharacterPhysics = Physics::CharacterRequestBus::FindFirstHandler(entityId) != nullptr;
            m_usesCharacterPhysics = usesLegacyCharacterPhysics || usesAZCharacterPhysics;

            AZ_Warning("NavigationComponent",
                usesAZCharacterPhysics || Physics::RigidBodyRequestBus::FindFirstHandler(entityId),
                "Entity %s cannot be moved physically because it is missing a physics component", GetEntity()->GetName().c_str());
        }

        AZ::TransformBus::EventResult(m_entityTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
    }

    void NavigationComponent::Deactivate()
    {
        NavigationComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        Reset();
    }
#ifdef LMBR_CENTRAL_EDITOR
    float NavigationComponent::CalculateAgentNavigationRadius(const char* agentTypeName)
    {
        float agentRadius = -1.0f;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            agentRadius, &AzToolsFramework::EditorRequests::Bus::Events::CalculateAgentNavigationRadius
            , agentTypeName);

        return agentRadius;
    }

    const char* NavigationComponent::GetDefaultAgentNavigationTypeName()
    {
        const char* agentTypeName = "";
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            agentTypeName, &AzToolsFramework::EditorRequests::Bus::Events::GetDefaultAgentNavigationTypeName);

        return agentTypeName;
    }

    AZStd::vector<AZStd::string> NavigationComponent::PopulateAgentTypeList()
    {
        if (m_agentType.size() == 0)
        {
            // If no previously stored agent type select a default one (usually on component added)
            m_agentType = GetDefaultAgentNavigationTypeName();
        }
        HandleAgentTypeChanged();
        return LmbrCentral::PopulateAgentTypeList();
    }

    AZ::u32 NavigationComponent::HandleAgentTypeChanged()
    {
        float agentRadius = CalculateAgentNavigationRadius(m_agentType.c_str());
        if (agentRadius >= 0.0f)
        {
            m_agentRadius = agentRadius;
        }
        else
        {
            AZ_Error("Editor", false, "Unable to find navigation radius data for agent type '%s'", m_agentType.c_str());
        }
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }
#endif
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // NavigationComponentRequestBus::Handler interface implementation

    PathfindRequest::NavigationRequestId NavigationComponent::FindPath(const PathfindRequest& request)
    {
        // If neither the position nor the destination has been set
        if (!(request.HasTargetEntity() || request.HasTargetLocation()))
        {
            // Return an invalid id to indicate that the request is bad
            return PathfindResponse::kInvalidRequestId;
        }

        // Reset the Navigation component to deal with a new pathfind request
        Reset();

        m_lastResponseCache.SetupForNewRequest(this, request);

        if (!request.HasTargetEntity())
        {
            FindPathImpl();
        }

        return m_lastResponseCache.GetRequestId();
    }

    void NavigationComponent::FindPathImpl()
    {
        // Request for a path
        PathfindResponse::PathfinderRequestId pathfinderRequestID = RequestPath();
        m_lastResponseCache.SetPathfinderRequestId(pathfinderRequestID);

        if (pathfinderRequestID != MNM::Constants::eQueuedPathID_InvalidID)
        {
            // Indicate that the path is being looked for
            m_lastResponseCache.SetStatus(PathfindResponse::Status::SearchingForPath);

            // Inform every listener on this entity about the "Searching For Path" event
            NavigationComponentNotificationBus::Event(m_entity->GetId(),
                &NavigationComponentNotificationBus::Events::OnSearchingForPath, m_lastResponseCache.GetRequestId());
        }
        else
        {
            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

            // Inform every listener on this entity about the "Traversal cancelled" event
            NavigationComponentNotificationBus::Event(m_entity->GetId(),
                &NavigationComponentNotificationBus::Events::OnTraversalCancelled, m_lastResponseCache.GetRequestId());
        }
    }

    PathfindResponse::PathfinderRequestId NavigationComponent::RequestPath()
    {
        // Create a new pathfind request
        MNMPathRequest pathfinderRequest;

        // 1. Set the current entity's position as the start location
        pathfinderRequest.startLocation = AZVec3ToLYVec3(m_entityTransform.GetTranslation());

        // 2. Set the requested destination
        pathfinderRequest.endLocation = AZVec3ToLYVec3(m_lastResponseCache.GetCurrentDestination());

        // 3. Set the type of the Navigation agent
        pathfinderRequest.agentTypeID = m_agentTypeId;

        // 4. Set the callback
        pathfinderRequest.resultCallback = AZStd::bind(&NavigationComponent::OnPathResult, this,
                                                        AZStd::placeholders::_1, AZStd::placeholders::_2);

        // 5. Request the path.
        IMNMPathfinder* pathFinder = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        return pathFinder ? pathFinder->RequestPathTo(this, pathfinderRequest) : 0;
    }

    void NavigationComponent::OnPathResult(const MNM::QueuedPathID& pathfinderRequestId, MNMPathRequestResult& result)
    {
        // If the pathfinding result is for the latest pathfinding request (Otherwise ignore)
        if (pathfinderRequestId == m_lastResponseCache.GetPathfinderRequestId())
        {
            if (result.HasPathBeenFound() &&
                (m_lastResponseCache.GetRequestId() != PathfindResponse::kInvalidRequestId))
            {
                m_lastResponseCache.SetCurrentPath(result.pPath->Clone());

                // If this request was in fact looking for a path (and this isn't just a path update request)
                if (m_lastResponseCache.GetStatus() == PathfindResponse::Status::SearchingForPath)
                {
                    // Set the status of this request
                    m_lastResponseCache.SetStatus(PathfindResponse::Status::PathFound);

                    // Inform every listener on this entity that a path has been found
                    bool shouldPathBeTraversed = true;

                    NavigationComponentNotificationBus::EventResult(shouldPathBeTraversed, m_entity->GetId(),
                        &NavigationComponentNotificationBus::Events::OnPathFound, m_lastResponseCache.GetRequestId(),
                        m_lastResponseCache.GetCurrentPath());

                    if (shouldPathBeTraversed)
                    {
                        // Connect to physics bus if appropriate, else tick bus
                        if ((m_movementMethod == NavigationComponentRequests::MovementMethod::Physics))
                        {
                            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
                            {
                                AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                                sceneInterface->RegisterSceneSimulationStartHandler(sceneHandle, m_sceneStartSimHandler);
                            }
                        }
                        else
                        {
                            AZ::TickBus::Handler::BusConnect();
                        }

                        //  Set the status of this request
                        m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalStarted);

                        // Inform every listener on this entity that traversal is in progress
                        NavigationComponentNotificationBus::Event(m_entity->GetId(),
                            &NavigationComponentNotificationBus::Events::OnTraversalStarted,
                            m_lastResponseCache.GetRequestId());
                    }
                    else
                    {
                        // Set the status of this request
                        m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

                        // Inform every listener on this entity that a path could not be found
                        NavigationComponentNotificationBus::Event(m_entity->GetId(),
                        &NavigationComponentNotificationBus::Events::OnTraversalCancelled,
                        m_lastResponseCache.GetRequestId());
                    }
                }
            }
            else
            {
                // Set the status of this request
                m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

                // Inform every listener on this entity that a path could not be found
                NavigationComponentNotificationBus::Event(m_entity->GetId(),
                    &NavigationComponentNotificationBus::Events::OnTraversalCancelled,
                    m_lastResponseCache.GetRequestId());
            }
        }
    }

    void NavigationComponent::Stop(PathfindRequest::NavigationRequestId requestId)
    {
        if ((m_lastResponseCache.GetRequestId() == requestId)
            && requestId != PathfindResponse::kInvalidRequestId)
        {
            Reset();
        }
    }

    float NavigationComponent::GetAgentSpeed()
    {
        return m_agentSpeed;
    }

    void NavigationComponent::SetAgentSpeed(float agentSpeed)
    {
        m_agentSpeed = agentSpeed;

        IPathFollowerPtr pathFollower = m_lastResponseCache.GetPathFollower();

        if (pathFollower)
        {
            PathFollowerParams currentParams = pathFollower->GetParams();
            currentParams.normalSpeed = agentSpeed;
            currentParams.minSpeed = currentParams.normalSpeed * 0.8f;
            currentParams.maxSpeed = currentParams.normalSpeed * 1.2f;

            pathFollower->SetParams(currentParams);
        }
    }

    NavigationComponentRequests::MovementMethod NavigationComponent::GetAgentMovementMethod()
    {
        return m_movementMethod;
    }

    void NavigationComponent::SetAgentMovementMethod(NavigationComponentRequests::MovementMethod movementMethod)
    {
        m_movementMethod = movementMethod;
    }

    void NavigationComponent::MoveEntity(float deltaTime)
    {
        // If there isn't a valid path
        if (!m_lastResponseCache.GetCurrentPath())
        {
            // Come back next frame
            return;
        }

        AZ::Vector3 currentVelocity = AZ::Vector3::CreateZero();
        float mass = 0.f;

        if (m_movementMethod == NavigationComponentRequests::MovementMethod::Physics)
        {
            {
                Physics::RigidBodyRequestBus::EventResult(currentVelocity, GetEntityId(),
                    &Physics::RigidBodyRequestBus::Events::GetLinearVelocity);

                Physics::RigidBodyRequestBus::EventResult(mass, GetEntityId(),
                    &Physics::RigidBodyRequestBus::Events::GetMass);
            }

            m_lastResponseCache.SetLastKnownAgentVelocity(currentVelocity);
        }

        // Update path-following and extract desired velocity.
        AZ::Vector3 nextPathPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 inflectionPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 targetVelocity = AZ::Vector3::CreateZero();
        float distanceToEnd = 0.f;

        auto pathFollower = m_lastResponseCache.GetPathFollower();
        if (pathFollower)
        {
            const AZ::Vector3 agentPosition = m_entityTransform.GetTranslation();
            const AZ::Vector3 agentVelocity = m_lastResponseCache.GetLastKnownAgentVelocity();

            PathFollowResult result;

            [[maybe_unused]] const bool arrived = pathFollower->Update(
                result,
                AZVec3ToLYVec3(agentPosition),
                AZVec3ToLYVec3(agentVelocity),
                deltaTime);

            nextPathPosition = LYVec3ToAZVec3(result.followTargetPos);
            inflectionPosition = LYVec3ToAZVec3(result.inflectionPoint);
            targetVelocity = LYVec3ToAZVec3(result.velocityOut);
            distanceToEnd = result.distanceToEnd;
        }

        if (targetVelocity == AZ::Vector3::CreateZero())
        {
            if (m_movementMethod == NavigationComponentRequests::MovementMethod::Physics)
            {
                if (!m_usesCharacterPhysics)
                {
                    Physics::RigidBodyRequestBus::Event(GetEntityId(),
                        &Physics::RigidBodyRequestBus::Events::SetLinearVelocity, AZ::Vector3::CreateZero());
                }
            }

            // Set the status of this request
            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalComplete);

            // Reset the pathfinding component
            Reset();

            // Inform every listener on this entity that the path has been finished
            NavigationComponentNotificationBus::Event(m_entity->GetId(),
                &NavigationComponentNotificationBus::Events::OnTraversalComplete, m_lastResponseCache.GetRequestId());
        }
        else
        {
            if (m_movementMethod == NavigationComponentRequests::MovementMethod::Custom)
            {
                if (!nextPathPosition.IsClose(m_lastResponseCache.GetNextPathPosition()) ||
                    !inflectionPosition.IsClose(m_lastResponseCache.GetInflectionPosition()))
                {
                    m_lastResponseCache.SetNextPathPosition(nextPathPosition);
                    m_lastResponseCache.SetInflectionPosition(inflectionPosition);

                    // when using the custom movement method we just update the path and rely on
                    // the user to move the entity
                    NavigationComponentNotificationBus::Event(m_entity->GetId(),
                        &NavigationComponentNotificationBus::Events::OnTraversalPathUpdate,
                        m_lastResponseCache.GetRequestId(),
                        nextPathPosition, inflectionPosition);

                }
            }
            else if (m_movementMethod == NavigationComponentRequests::MovementMethod::Physics)
            {
                {
                    AZ::Vector3 forceRequired = (targetVelocity - currentVelocity) * mass;
                    forceRequired.SetZ(0);

                    if (m_usesCharacterPhysics)
                    {
                        Physics::CharacterRequestBus::Event(GetEntityId(),
                            &Physics::CharacterRequestBus::Events::AddVelocity, targetVelocity);
                    }
                    else
                    {
                        Physics::RigidBodyRequestBus::Event(GetEntityId(),
                            &Physics::RigidBodyRequestBus::Events::ApplyLinearImpulse, forceRequired);
                    }
                }
            }
            else
            {
                // Set the position of the entity
                AZ::Transform newEntityTransform = m_entityTransform;
                AZ::Vector3 movementDelta = targetVelocity * deltaTime;
                AZ::Vector3 currentPosition = m_entityTransform.GetTranslation();
                AZ::Vector3 newPosition = m_entityTransform.GetTranslation() + movementDelta;
                newEntityTransform.SetTranslation(newPosition);
                AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, newEntityTransform);

                m_lastResponseCache.SetLastKnownAgentVelocity(targetVelocity);
            }

            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalInProgress);

            NavigationComponentNotificationBus::Event(m_entity->GetId(),
                &NavigationComponentNotificationBus::Events::OnTraversalInProgress, m_lastResponseCache.GetRequestId(),
                distanceToEnd);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::TickBus::Handler implementation

    void NavigationComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        MoveEntity(deltaTime);
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::TransformNotificationBus::Handler implementation
    // If the transform on the entity has changed
    void NavigationComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_entityTransform = world;
    }

    //////////////////////////////////////////////////////////////////////////
    // NavigationComponent implementation

    void NavigationComponent::Reset()
    {
        m_lastResponseCache.Reset();

        // Disconnect from tick bus and physics bus
        AZ::TickBus::Handler::BusDisconnect();
        m_sceneStartSimHandler.Disconnect();
    }

    PathfindRequest::NavigationRequestId NavigationComponent::FindPathToEntity(AZ::EntityId targetEntityId)
    {
        PathfindRequest request;
        request.SetTargetEntityId(targetEntityId);
        return FindPath(request);
    }

    PathfindRequest::NavigationRequestId NavigationComponent::FindPathToPosition(const AZ::Vector3& destination)
    {
        PathfindRequest request;
        request.SetDestinationLocation(destination);
        return FindPath(request);
    }
} // namespace LmbrCentral
