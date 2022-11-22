/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Pathfinder Interface.

#pragma once

struct IAIPathAgent;

#include <INavigationSystem.h>
#include <IMNM.h>
#include <SerializeFwd.h>
#include <Cry_Geo.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <limits>
#include <list>

// Hacks to deal with windows header inclusion.
#ifdef GetObject
#undef GetObject
#endif
#ifdef GetUserName
#undef GetUserName
#endif
#ifdef DrawText
#undef DrawText
#endif
#ifdef LoadLibrary
#undef LoadLibrary
#endif
#ifdef max
#undef max
#endif

/* WARNING: These interfaces and structures are soon to be deprecated.
                        Use at your own risk of having to change your code later!
*/


// Passing through navigational SO methods.
enum ENavSOMethod
{
    nSOmNone,                           // not passing or not passable
    nSOmAction,                     // execute an AI action
    nSOmPriorityAction,     // execute a higher priority AI action
    nSOmStraight,                   // just pass straight
    nSOmSignalAnimation,    // play signal animation
    nSOmActionAnimation,    // play action animation
    nSOmLast
};

// Indication of (a) what a graph node represents and (b) what kind of graph node an AI entity
// can navigate. In the latter case it can be used as a bit mask.
// Copied from CryCommon/IAISystem.h which has been deleted.
enum ENavigationType
{
    NAV_UNSET              = 1 << 0,
    NAV_TRIANGULAR         = 1 << 1,
    NAV_WAYPOINT_HUMAN     = 1 << 2,
    NAV_WAYPOINT_3DSURFACE = 1 << 3,
    NAV_FLIGHT             = 1 << 4,
    NAV_VOLUME             = 1 << 5,
    NAV_ROAD               = 1 << 6,
    NAV_SMARTOBJECT        = 1 << 7,
    NAV_FREE_2D            = 1 << 8,
    NAV_CUSTOM_NAVIGATION    = 1 << 9,
    NAV_MAX_VALUE          = NAV_CUSTOM_NAVIGATION
};

//====================================================================
// NavigationBlocker
// Represents an object that might be blocking a link. Each blocker is
// assumed to be spherical, with the position centred around the floor
// so that links can intersect it.
//====================================================================
struct NavigationBlocker
{
    /// pos and radius define the sphere.
    /// costAddMod is a fixed cost (in m) associated with the blocker obscuring a link - a value of 0 has
    /// no effect - a value of 10 would make the link effectively 10m longer than it is.
    /// costMultMod is the cost modification factor - a value of 0 has no effect - a value of 10 would make the link
    /// 10x more costly. -ve disables the link
    /// radialDecay indicates if the cost modifiers should decay linearly to 0 over the radius of the sphere
    /// directional indicates if the cost should be unaffected for motion in a radial direction
    NavigationBlocker(const Vec3& pos, float radius, float _costAddMod, float _costMultMod, bool _radialDecay, bool _directional)
        : sphere(pos, radius)
        , costAddMod(_costAddMod)
        , costMultMod(_costMultMod)
        , restrictedLocation(false)
        , radialDecay(_radialDecay)
        , directional(_directional) {}

    /// Just to allow std::vector::resize(0)
    NavigationBlocker()
        : sphere(Vec3(0, 0, 0), 0.0f)
        , costAddMod(0)
        , costMultMod(0)
        , radialDecay(false) {AZ_Assert(false, "Should never get called"); }

    ::Sphere sphere;
    bool radialDecay;
    bool directional;

    /// absolute cost added to any link going through this blocker (useful for small blockers)
    float costAddMod;
    /// multiplier for link costs going through this blocker (0 means no extra cost, 1 means to double etc)
    float costMultMod;

    // info to speed up the intersection checks
    /// If this is true then the blocker is small enough that it only affects the
    /// nav type it resides in. If false then it affects everything.
    bool restrictedLocation;
    ENavigationType navType;
    union Location
    {
        struct // similar for other nav types when there's info to go in them
        {
            // no node because the node "areas" can overlap so it's not useful
            int nBuildingID;
        } waypoint;
    };
    /// Only gets used if restrictedLocation = true
    Location location;
};

using NavigationBlockers = AZStd::vector<NavigationBlocker>;

//====================================================================
// PathPointDescriptor
//====================================================================
struct PathPointDescriptor
{
    struct SmartObjectNavData
        : public _i_reference_target_t
    {
        unsigned fromIndex;
        unsigned toIndex;
    };
    typedef _smart_ptr< SmartObjectNavData > SmartObjectNavDataPtr;

    struct OffMeshLinkData
    {
        OffMeshLinkData()
            : meshID(0)
            , offMeshLinkID(0)
        {}

        uint32 meshID;
        MNM::OffMeshLinkID offMeshLinkID;
    };

    PathPointDescriptor(ENavigationType _navType = NAV_UNSET, const Vec3& _vPos = ZERO)
        : vPos(_vPos)
        , navType(_navType)
        , navTypeCustomId(0)
        , iTriId(0)
        , pSONavData(0)
        , navSOMethod(nSOmNone)
    {}

    PathPointDescriptor(const Vec3& _vPos)
        : vPos(_vPos)
        , navType(NAV_UNSET)
        , navTypeCustomId(0)
        , iTriId(0)
        , pSONavData(0)
        , navSOMethod(nSOmNone)
    {}

    bool IsEquivalent(const PathPointDescriptor& other) const
    {
        return (navType == other.navType) && vPos.IsEquivalent(other.vPos, 0.01f);
    }

    static bool ArePointsEquivalent(const PathPointDescriptor& point1, const PathPointDescriptor& point2)
    {
        return point1.IsEquivalent(point2);
    }

    Vec3 vPos;
    ENavigationType navType;
    uint16 navTypeCustomId;

    uint32 iTriId;
    OffMeshLinkData offMeshLinkData;

    SmartObjectNavDataPtr pSONavData;

    ENavSOMethod navSOMethod;
};

struct PathfindingExtraConstraint
{
    enum EExtraConstraintType
    {
        ECT_MAXCOST,
        ECT_MINDISTFROMPOINT,
        ECT_AVOIDSPHERE,
        ECT_AVOIDCAPSULE
    };

    EExtraConstraintType type;

    union UConstraint
    {
        struct SConstraintMaxCost
        {
            float maxCost;
        };
        struct SConstraintMinDistFromPoint
        {
            float px, py, pz; // Can't use Vec3 as it has a constructor.
            float minDistSq;
        };
        struct SConstraintAvoidSphere
        {
            float px, py, pz; // Can't use Vec3 as it has a constructor.
            float minDistSq;
        };
        struct SConstraintAvoidCapsule
        {
            float px, py, pz;
            float qx, qy, qz;
            float minDistSq;
        };
        SConstraintMaxCost maxCost;
        SConstraintMinDistFromPoint minDistFromPoint;
        SConstraintAvoidSphere avoidSphere;
        SConstraintAvoidCapsule avoidCapsule;
    };
    UConstraint constraint;
};

using PathfindingExtraConstraints = AZStd::vector<PathfindingExtraConstraint>;

struct PathfindRequest
{
    enum ERequestType
    {
        TYPE_ACTOR,
        TYPE_RAW,
    };
    ERequestType type;

    unsigned startIndex;
    unsigned endIndex;
    Vec3 startPos;
    Vec3 startDir;
    Vec3 endPos;
    /// endDir magnitude indicates the tendency to line up at the end of the path -
    /// magnitude should be between 0 and 1
    Vec3 endDir;
    bool bSuccess;
    IAIPathAgent* pRequester;
    int nForceTargetBuildingId;
    bool allowDangerousDestination;
    float endTol;
    float endDistance;
    // as a result of RequestPathInDirection or RequestPathTo
    bool isDirectional;

    /// This gets set to false if the path end position doesn't match the requested end position
    /// (e.g. in the event of a partial path, or if the destination is in forbidden)
    bool bPathEndIsAsRequested;

    int id;
    float passRadius;

    PathfindingExtraConstraints extraConstraints;

    PathfindRequest(ERequestType _type)
        : type(_type)
        , startIndex(0)
        , endIndex(0)
        , pRequester(0)
        , bPathEndIsAsRequested(false)
        , allowDangerousDestination(false)
        , endTol(std::numeric_limits<float>::max())
        , endDistance(0)
        , nForceTargetBuildingId(-1)
        , isDirectional(false)
        , id(-1)
        , passRadius(0.0f)
    {
    }
};

struct PathFollowerParams
{
    PathFollowerParams()
        : normalSpeed(0.0f)
        , pathRadius(0.0f)
        , pathLookAheadDist(1.0f)
        , maxAccel(0.0f)
        , maxDecel(1.0f)
        , minSpeed(0.0f)
        , maxSpeed(10.0f)
        , endAccuracy(0.2f)
        , endDistance(0.0f)
        , stopAtEnd(true)
        , use2D(true)
        , isVehicle(false)
        , isAllowedToShortcut(true)
        , passRadius(0.5f)
    {}

    // OLD: Remove this when possible, Animation to take over majority of logic
    float normalSpeed; ///< normal entity speed
    float pathRadius; ///< max deviation allowed from the path
    float pathLookAheadDist; ///< how far we look ahead along the path - normally the same as pathRadius
    float maxAccel; ///< maximum acceleration of the entity
    float maxDecel; ///< maximum deceleration of the entity
    float minSpeed; ///< minimum output speed (unless it's zero on path end etc)
    float maxSpeed; ///< maximum output speed

    // KEEP: Additions and useful state for new impl.
    float endAccuracy; ///< How close to the end point the agent must be to finish pathing
    float endDistance; ///< stop this much before the end
    bool stopAtEnd; ///< aim to finish the path by reaching the end position (stationary), or simply overshoot
    bool use2D; ///< follow in 2 or 3D
    bool isVehicle;
    bool isAllowedToShortcut;

    /// The minimum radius of the agent for navigating
    float passRadius;
};


struct PathFollowResult
{
    struct SPredictedState
    {
        SPredictedState() {}
        SPredictedState(const Vec3& p, const Vec3& v)
            : pos(p)
            , vel(v) {}
        Vec3 pos;
        Vec3 vel;
    };
    typedef AZStd::vector<SPredictedState> TPredictedStates;

    // OLD: Obsolete & to be replaced by new impl.

    /// maximum time to predict out to - actual prediction may not go this far
    float desiredPredictionTime;
    /// the first element in predictedStates will be now + predictionDeltaTime, etc
    float predictionDeltaTime;
    /// if this is non-zero then on output the prediction will be placed into it
    TPredictedStates* predictedStates;

    PathFollowResult()
        : predictionDeltaTime(0.1f)
        , predictedStates(0)
        , desiredPredictionTime(0)
        , distanceToEnd(0.f)
        , reachedEnd(false)
        , followTargetPos(0)
        , inflectionPoint(0) {}


    float distanceToEnd;
    bool reachedEnd;
    Vec3 velocityOut;

    // NEW: Replaces data above

    // NOTE: If the turningPoint and inflectionPoint are equal, they represent the end of the path.
    /// The furthest point on the path we can move directly towards without colliding with anything
    Vec3 followTargetPos;
    /// The next point on the path beyond the follow target that deviates substantially from a straight-line path
    Vec3 inflectionPoint;

    /// The maximum distance the agent can safely move in a straight line beyond the turning point
    //  float maxOverrunDistance;
};



// Intermediary and minimal interface to use the pathfinder without requiring an AI object
// TODO: Fix the long function names. At the moment they collide with IAIObject base.
//           The alternative would be to make IAIObject derive from IAIPathAgent.
struct IAIPathAgent
{
    virtual ~IAIPathAgent(){}
    virtual const char* GetPathAgentName() const = 0;
    virtual unsigned short GetPathAgentType() const = 0;

    virtual float GetPathAgentPassRadius() const = 0;
    virtual Vec3 GetPathAgentPos() const = 0;
    virtual Vec3 GetPathAgentVelocity() const = 0;

    // This cannot easily be const, but has no side-effects
    virtual void GetPathAgentNavigationBlockers(NavigationBlockers& blockers, const PathfindRequest* pRequest) = 0;

    virtual void SetPathToFollow(const char* pathName) = 0;
    virtual void SetPathAttributeToFollow(bool bSpline) = 0;

    //Path finding avoids blocker type by radius.
    virtual void SetPFBlockerRadius(int blockerType, float radius) = 0;

    virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const = 0;
    virtual bool GetTeleportPosition(Vec3& teleportPos) const = 0;

    virtual class IPathFollower* GetPathFollower() const = 0;

    virtual bool IsPointValidForAgent(const Vec3& pos, uint32 flags) const = 0;
};

typedef std::list<PathPointDescriptor> TPathPoints;

struct SNavPathParams
{
    SNavPathParams(const Vec3& _start = Vec3_Zero, const Vec3& _end = Vec3_Zero,
        const Vec3& _startDir = Vec3_Zero, const Vec3& _endDir = Vec3_Zero,
        int _nForceBuildingID = -1, bool _allowDangerousDestination = false, float _endDistance = 0.0f,
        bool _continueMovingAtEnd = false, bool _isDirectional = false)
        : start(_start)
        , end(_end)
        , startDir(_startDir)
        , endDir(_endDir)
        , nForceBuildingID(_nForceBuildingID)
        , allowDangerousDestination(_allowDangerousDestination)
        , precalculatedPath(false)
        , inhibitPathRegeneration(false)
        , continueMovingAtEnd(_continueMovingAtEnd)
        , endDistance(_endDistance)
        , isDirectional(_isDirectional)
    {}

    Vec3 start;
    Vec3 end;
    Vec3 startDir;
    Vec3 endDir;
    int nForceBuildingID;
    bool allowDangerousDestination;
    /// if path is precalculated it should not be regenerated, and also some things like steering
    /// will be disabled
    bool precalculatedPath;
    /// sometimes it is necessary to disable a normal path from getting regenerated
    bool inhibitPathRegeneration;
    bool continueMovingAtEnd;
    bool isDirectional;
    float endDistance; // The requested cut distance of the path, positive value means distance from path end, negative value means distance from path start.
    NavigationMeshID meshID;

    void Clear()
    {
        start = Vec3_Zero;
        end = Vec3_Zero;
        startDir = Vec3_Zero;
        endDir = Vec3_Zero;
        allowDangerousDestination = false;
        precalculatedPath = false;
        inhibitPathRegeneration = false;
        continueMovingAtEnd = false;
        isDirectional = false;
        endDistance = 0.0f;
        meshID = NavigationMeshID(0);
    }

    void Serialize(TSerialize ser)
    {
        ser.Value("start", start);
        ser.Value("end", end);
        ser.Value("startDir", startDir);
        ser.Value("endDir", endDir);
        ser.Value("nForceBuildingID", nForceBuildingID);
        ser.Value("allowDangerousDestination", allowDangerousDestination);
        ser.Value("precalculatedPath", precalculatedPath);
        ser.Value("inhibitPathRegeneration", inhibitPathRegeneration);
        ser.Value("continueMovingAtEnd", continueMovingAtEnd);
        ser.Value("isDirectional", isDirectional);
        ser.Value("endDistance", endDistance);

        if (ser.IsReading())
        {
            uint32 meshIdAsUint32;
            ser.Value("meshID", meshIdAsUint32);
            meshID = NavigationMeshID(meshIdAsUint32);
        }
        else
        {
            uint32 meshIdAsUint32 = (uint32) meshID;
            ser.Value("meshID", meshIdAsUint32);
        }
    }
};

class INavPath
{
public:
    virtual ~INavPath(){}
    virtual void Release() = 0;
    virtual void CopyTo(INavPath* pRecipient) const = 0;
    virtual AZStd::shared_ptr<INavPath> Clone() const = 0;

    virtual NavigationMeshID GetMeshID() const = 0;

    virtual int GetVersion() const = 0;
    virtual void SetVersion(int version) = 0;

    virtual void SetParams(const SNavPathParams& params) = 0;
    virtual const SNavPathParams& GetParams() const = 0;
    virtual SNavPathParams& GetParams() = 0;
    virtual const TPathPoints& GetPath() const = 0;
    virtual void SetPathPoints(const TPathPoints& points) = 0;

    virtual float   GetPathLength(bool twoD) const = 0;
    virtual void PushFront(const PathPointDescriptor& newPathPoint, bool force = false) = 0;
    virtual void PushBack(const PathPointDescriptor& newPathPoint, bool force = false) = 0;
    virtual void Clear(const char* contextName) = 0;
    virtual bool Advance(PathPointDescriptor& nextPathPoint) = 0;

    virtual bool GetPathEndIsAsRequested() const = 0;
    virtual void SetPathEndIsAsRequested(bool value) = 0;

    virtual bool Empty() const = 0;

    virtual const PathPointDescriptor* GetLastPathPoint() const = 0;
    virtual const PathPointDescriptor* GetPrevPathPoint() const = 0;
    virtual const PathPointDescriptor* GetNextPathPoint() const = 0;
    virtual const PathPointDescriptor* GetNextNextPathPoint() const = 0;

    virtual const Vec3& GetNextPathPos(const Vec3& defaultPos = Vec3_Zero) const = 0;
    virtual const Vec3& GetLastPathPos(const Vec3& defaultPos = Vec3_Zero) const = 0;

    virtual bool GetPosAlongPath(Vec3& posOut, float dist, bool twoD, bool extrapolateBeyondEnd, ENavigationType* nextPointType = NULL) const = 0;
    virtual float GetDistToPath(Vec3& pathPosOut, float& distAlongPathOut, const Vec3& pos, float dist, bool twoD) const = 0;
    virtual float GetDistToSmartObject(bool twoD) const = 0;
    virtual PathPointDescriptor::SmartObjectNavDataPtr GetLastPathPointAnimNavSOData() const = 0;
    virtual void SetPreviousPoint(const PathPointDescriptor& previousPoint) = 0;

    virtual AABB GetAABB(float dist) const = 0;

    virtual bool GetPathPropertiesAhead(float distAhead, bool twoD, Vec3& posOut, Vec3& dirOut, float* invROut, float& lowestPathDotOut, bool scaleOutputWithDist) const = 0;

    virtual void SetEndDir(const Vec3& endDir) = 0;
    virtual const Vec3& GetEndDir() const = 0;

    virtual bool UpdateAndSteerAlongPath(Vec3& dirOut, float& distToEndOut, float& distToPathOut, bool& isResolvingSticking, Vec3& pathDirOut, Vec3& pathAheadDirOut, Vec3& pathAheadPosOut, Vec3 currentPos, const Vec3& currentVel, float lookAhead, float pathRadius, float dt, bool resolveSticking, bool twoD) = 0;

    virtual void TrimPath(float length, bool twoD) = 0;
    virtual float GetDiscardedPathLength() const = 0;
    virtual float UpdatePathPosition(Vec3 agentPos, float pathLookahead, bool twoD, bool allowPathToFinish) = 0;
    virtual Vec3 CalculateTargetPos(Vec3 agentPos, float lookAhead, float minLookAheadAlongPath, float pathRadius, bool twoD) const = 0;

    virtual void Draw(const Vec3& drawOffset = ZERO) const = 0;
    virtual void Dump(const char* name) const = 0;

};
using INavPathPtr = AZStd::shared_ptr<INavPath>;

enum EMNMPathResult
{
    eMNMPR_NoPathFound = 0,
    eMNMPR_Success,
};

struct MNMPathRequestResult
{
    MNMPathRequestResult()
        : cost(0.f)
        , id(0)
        , result(eMNMPR_NoPathFound)
    {}

    ILINE bool HasPathBeenFound() const
    {
        return result == eMNMPR_Success;
    }

    INavPathPtr pPath;
    float cost;
    uint32 id;
    EMNMPathResult result;
};

struct IPathObstacles
{
    virtual ~IPathObstacles() {}

    virtual bool IsPathIntersectingObstacles(const NavigationMeshID meshID, const Vec3& start, const Vec3& end, float radius) const = 0;
    virtual bool IsPointInsideObstacles(const Vec3& position) const = 0;
    virtual bool IsLineSegmentIntersectingObstaclesOrCloseToThem(const Lineseg& linesegToTest, float maxDistanceToConsiderClose) const = 0;
};

class IPathFollower
{
public:
    virtual ~IPathFollower(){}

    virtual void Release() = 0;

    virtual void Reset() = 0;

    /// This attaches us to a particular path (pass 0 to detach)
    virtual void AttachToPath(INavPath* navPath) = 0;

    virtual void SetParams(const PathFollowerParams& params) = 0;

    /// Just view the params
    virtual const PathFollowerParams& GetParams() const = 0;
    virtual PathFollowerParams& GetParams() = 0;

    // Advances the follow target along the path as far as possible while ensuring the follow
    // target remains reachable. Returns true if the follow target is reachable, false otherwise.
    virtual bool Update(PathFollowResult& result, const Vec3& curPos, const Vec3& curVel, float dt) = 0;

    /// Advances the current state in terms of position - effectively pretending that the follower
    /// has gone further than it has.
    virtual void Advance(float distance) = 0;

    /// Returns the distance from the lookahead to the end, plus the distance from the position passed in
    /// to the LA if pCurPos != 0
    virtual float GetDistToEnd(const Vec3* pCurPos) const = 0;

    /// Returns the distance along the path from the current look-ahead position to the
    /// first smart object path segment. If there's no path, or no smart objects on the
    /// path, then std::numeric_limits<float>::max() will be returned
    virtual float GetDistToSmartObject() const = 0;
    virtual float GetDistToNavType(ENavigationType navType) const = 0;

    /// Returns a point on the path some distance ahead. actualDist is set according to
    /// how far we looked - may be less than dist if we'd reach the end
    virtual Vec3 GetPathPointAhead(float dist, float& actualDist) const = 0;

    virtual void Draw(const Vec3& drawOffset = ZERO) const = 0;

    virtual void Serialize(TSerialize ser) = 0;

    // Checks ability to walk along a piecewise linear path starting from the current position
    // (useful for example when animation would like to deviate from the path)
    virtual bool CheckWalkability(const Vec2* path, const size_t length) const = 0;

    // Can the pathfollower cut corners if there is space to do so? (default: true)
    virtual bool GetAllowCuttingCorners() const = 0;

    // Sets whether or not the pathfollower is allowed to cut corners if there is space to do so. (default: true)
    virtual void SetAllowCuttingCorners(const bool allowCuttingCorners) = 0;
};

namespace MNM
{
    typedef unsigned int QueuedPathID;

    namespace Constants
    {
        enum EQueuedPathID
        {
            eQueuedPathID_InvalidID = 0,
        };
    }
}

struct MNMPathRequest
{
    using Callback = AZStd::function<void(const MNM::QueuedPathID&, MNMPathRequestResult&)>;

    MNMPathRequest()
        : resultCallback(0)
        , startLocation(ZERO)
        , endLocation(ZERO)
        , endDirection(Vec3(0, 1, 0))
        , agentTypeID(NavigationAgentTypeID())
        , forceTargetBuildingId(0)
        , endTolerance(0.0f)
        , endDistance(0.0f)
        , allowDangerousDestination(false)
        , beautify(true)
    {
    }

    MNMPathRequest(const Vec3& start, const Vec3& end, const Vec3& _endDirection, int _forceTargetBuildingId,
        float _endTolerance, float _endDistance, [[maybe_unused]] bool _allowDangerousDestination, const Callback& callback,
        const NavigationAgentTypeID& _agentTypeID)
        : resultCallback(callback)
        , startLocation(start)
        , endLocation(end)
        , endDirection(_endDirection)
        , agentTypeID(_agentTypeID)
        , forceTargetBuildingId(_forceTargetBuildingId)
        , endTolerance(_endTolerance)
        , endDistance(_endDistance)
        , allowDangerousDestination(false)
        , beautify(true)
    {
    }

    Callback resultCallback;

    Vec3    startLocation;
    Vec3    endLocation;
    Vec3    endDirection;

    NavigationAgentTypeID agentTypeID;

    //Set beautify to false if you don't want to "beautify" the path (make it little less jagged, and more curvy)
    bool    beautify;

    int     forceTargetBuildingId;
    float   endTolerance;
    float   endDistance;
    bool    allowDangerousDestination;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct IMNMPathfinder
{
    virtual ~IMNMPathfinder() {};

    //Request a path (look at MNMPathRequest for relevant request info).  This request is queued and processed in a seperate
    //thread.  The path result is sent to the callback function specified in the request.  Returns an ID so you can cancel
    //the request.
    virtual MNM::QueuedPathID RequestPathTo(const IAIPathAgent* pRequester, const MNMPathRequest& request) = 0;
    // Returns a four-tuple: triangle ID and three vertices
    virtual const std::tuple<uint32, Vec3, Vec3, Vec3> GetCurrentNavTriangle(const IAIPathAgent* pRequester, NavigationAgentTypeID agentTypeID) const = 0;

    //Cancel a requested path by ID
    virtual void CancelPathRequest(MNM::QueuedPathID requestId) = 0;

    virtual bool CheckIfPointsAreOnStraightWalkableLine(const NavigationMeshID& meshID, const Vec3& source, const Vec3& destination, float heightOffset = 0.2f) const = 0;
};
