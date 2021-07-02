/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_INAVIGATIONSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_INAVIGATIONSYSTEM_H
#pragma once

#include <AzCore/std/functional.h>

#include <IMNM.h>
#include <physinterface.h>

struct IOffMeshNavigationManager;

enum ENavigationIDTag
{
    MeshIDTag = 0,
    AgentTypeIDTag,
    VolumeIDTag,
};

template <ENavigationIDTag T>
struct TNavigationID
{
    explicit TNavigationID(uint32 _id = 0)
        : id(_id) {}

    TNavigationID& operator=(const TNavigationID& other) { id = other.id; return *this; }

    ILINE operator uint32() const { return id; }
    ILINE bool operator==(const TNavigationID& other) const { return id == other.id; }
    ILINE bool operator!=(const TNavigationID& other) const { return id != other.id; }
    ILINE bool operator<(const TNavigationID& other) const { return id < other.id; }

private:
    uint32 id;
};


typedef TNavigationID<MeshIDTag> NavigationMeshID;
typedef TNavigationID<AgentTypeIDTag> NavigationAgentTypeID;
typedef TNavigationID<VolumeIDTag> NavigationVolumeID;
typedef AZStd::function<void(NavigationAgentTypeID, NavigationMeshID, uint32)> NavigationMeshChangeCallback;
typedef AZStd::function<bool(IPhysicalEntity&, uint32&)> NavigationMeshEntityCallback;

struct INavigationSystemUser
{
    virtual ~INavigationSystemUser() {}

    virtual void Reset() = 0;
    virtual void UpdateForSynchronousOrAsynchronousReadingOperation() = 0;
    virtual void UpdateForSynchronousWritingOperations() = 0;
    virtual void CompleteRunningTasks() = 0;
};

struct INavigationSystem
{
    enum ENavigationEvent
    {
        MeshReloaded = 0,
        MeshReloadedAfterExporting,
        NavigationCleared,
    };

    enum EAccessbilityDir
    {
        AccessibilityToward,
        AccessibilityAway,
    };

    struct INavigationSystemListener
    {
        // <interfuscator:shuffle>
        virtual ~INavigationSystemListener(){}
        virtual void OnNavigationEvent(const ENavigationEvent event) = 0;
        // </interfuscator:shuffle>
    };

    enum WorkingState
    {
        Idle = 0,
        Working,
    };


    struct CreateAgentTypeParams
    {
        CreateAgentTypeParams(const Vec3& _voxelSize = Vec3(0.1f), uint16 _radiusVoxelCount = 4,
            uint16 _climbableVoxelCount = 4, uint16 _heightVoxelCount = 18,
            [[maybe_unused]] uint16 _maxWaterDepthVoxelCount = 6)
            : voxelSize(_voxelSize)
            , climbableInclineGradient(1.0f)
            , climbableStepRatio(0.75f)
            , radiusVoxelCount(_radiusVoxelCount)
            , climbableVoxelCount(_climbableVoxelCount)
            , heightVoxelCount(_heightVoxelCount)
            , maxWaterDepthVoxelCount()
        {
        }

        Vec3 voxelSize;

        float climbableInclineGradient;
        float climbableStepRatio;

        uint16 radiusVoxelCount;
        uint16 climbableVoxelCount;
        uint16 heightVoxelCount;
        uint16 maxWaterDepthVoxelCount;
    };

    struct CreateMeshParams
    {
        CreateMeshParams(const Vec3& _origin = Vec3(ZERO), const Vec3i& _tileSize = Vec3i(8), const uint32 _tileCount = 1024)
            : origin(_origin)
            , tileSize(_tileSize)
            , tileCount(_tileCount)
        {
        }

        Vec3 origin;
        Vec3i tileSize;
        uint32 tileCount;
    };

    // <interfuscator:shuffle>
    virtual ~INavigationSystem() {}
    virtual NavigationAgentTypeID CreateAgentType(const char* name, const CreateAgentTypeParams& params) = 0;
    virtual NavigationAgentTypeID GetAgentTypeID(const char* name) const = 0;
    virtual NavigationAgentTypeID GetAgentTypeID(size_t index) const = 0;
    virtual const char* GetAgentTypeName(NavigationAgentTypeID agentTypeID) const = 0;
    virtual size_t GetAgentTypeCount() const = 0;

    virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params) = 0;
    virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params, NavigationMeshID requestedID) = 0;
    virtual void DestroyMesh(NavigationMeshID meshID) = 0;

    virtual void SetMeshEntityCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshEntityCallback& callback) = 0;
    virtual void AddMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback) = 0;
    virtual void RemoveMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback) = 0;

    virtual void SetMeshBoundaryVolume(NavigationMeshID meshID, NavigationVolumeID volumeID) = 0;
    virtual NavigationVolumeID CreateVolume(Vec3* vertices, size_t vertexCount, float height) = 0;
    virtual NavigationVolumeID CreateVolume(Vec3* vertices, size_t vertexCount, float height, NavigationVolumeID requestedID) = 0;
    virtual void DestroyVolume(NavigationVolumeID volumeID) = 0;
    virtual void SetVolume(NavigationVolumeID volumeID, Vec3* vertices, size_t vertexCount, float height) = 0;
    virtual bool ValidateVolume(NavigationVolumeID volumeID) = 0;
    virtual NavigationVolumeID GetVolumeID(NavigationMeshID meshID) = 0;

    virtual void SetExclusionVolume(const NavigationAgentTypeID* agentTypeIDs, size_t agentTypeIDCount, NavigationVolumeID volumeID) = 0;

    virtual NavigationMeshID GetMeshID(const char* name, NavigationAgentTypeID agentTypeID) const = 0;
    virtual const char* GetMeshName(NavigationMeshID meshID) const = 0;
    virtual void SetMeshName(NavigationMeshID meshID, const char* name) = 0;

    virtual WorkingState GetState() const = 0;
    virtual WorkingState Update(bool blocking = false) = 0;
    virtual void PauseNavigationUpdate() = 0;
    virtual void RestartNavigationUpdate() = 0;

    virtual size_t QueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb) = 0;
    virtual void ProcessQueuedMeshUpdates() = 0;

    virtual void Clear() = 0;
    // ClearAndNotify it's used when the listeners need to be notified about
    // the performed clear operation.
    virtual void ClearAndNotify() = 0;
    virtual bool ReloadConfig() = 0;
    virtual void DebugDraw() = 0;
    virtual void Reset() = 0;

    virtual void WorldChanged(const AABB& aabb) = 0;

    virtual void SetDebugDisplayAgentType(NavigationAgentTypeID agentTypeID) = 0;
    virtual NavigationAgentTypeID GetDebugDisplayAgentType() const = 0;

    //Returns impact mesh ID and point if segment intersects
    virtual std::tuple<bool, NavigationMeshID, Vec3> RaycastWorld(const Vec3& segP0, const Vec3& segP1) const = 0;

    virtual bool GetClosestPointInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location, float vrange, float hrange, Vec3* meshLocation, float minIslandArea = 0.f) const = 0;

    //Returns nav mesh ID at the specified location based on the passed in agent type
    virtual NavigationMeshID GetEnclosingMeshID(NavigationAgentTypeID agentTypeID, const Vec3& location) const = 0;

    virtual bool IsLocationValidInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location) const = 0;

    //Test to see if the specified location is within the navmesh.  The point has to be weithin downRange and upRange offset from location
    virtual bool IsLocationContainedWithinTriangleInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location, float downRange, float upRange) const = 0;

    //Returns a list of triangle centers within the specified AABB and navmesh
    virtual size_t GetTriangleCenterLocationsInMesh(const NavigationMeshID meshID, const Vec3& location, const AABB& searchAABB, Vec3* centerLocations, size_t maxCenterLocationCount, float minIslandArea = 0.f) const = 0;

    // Returns all borders (unconnected edges) in the specified AABB.  There are 3 Vec3's
    // per border edge, vert 0, vert 1, and a normal pointing out from the edge.  You can
    // pass null for pBorders to return the total number of borders (multiply this by 3 to
    // get the total number of Vec3's you need to pass in).
    virtual size_t GetTriangleBorders(const NavigationMeshID meshID, const AABB& aabb, Vec3* pBorders, size_t maxBorderCount, float minIslandArea = 0.f) const = 0;

    //For Hunt - gets triangle centers, and island ids - this is used to compute spawn points for an area
    virtual size_t GetTriangleInfo(const NavigationMeshID meshID, const AABB& aabb, Vec3* centerLocations, uint32* islandids, size_t max_count, float minIslandArea = 0.f) const = 0;

    //returns island id of the triangle at the current position
    virtual MNM::GlobalIslandID GetGlobalIslandIdAtPosition(const NavigationAgentTypeID agentID, const Vec3& location) = 0;

    virtual bool ReadFromFile(const char* fileName, bool bAfterExporting) = 0;
    virtual bool SaveToFile(const char* fileName) const = 0;

    virtual void RegisterListener(INavigationSystemListener* pListener, const char* name = NULL) = 0;
    virtual void UnRegisterListener(INavigationSystemListener* pListener) = 0;

    virtual void RegisterUser(INavigationSystemUser* pExtension, const char* name = NULL) = 0;
    virtual void UnRegisterUser(INavigationSystemUser* pExtension) = 0;

    virtual void RegisterArea(const char* shapeName) = 0;
    virtual void UnRegisterArea(const char* shapeName) = 0;
    virtual bool IsAreaPresent(const char* shapeName) = 0;
    virtual NavigationVolumeID GetAreaId(const char* shapeName) const = 0;
    virtual void SetAreaId(const char* shapeName, NavigationVolumeID id) = 0;
    virtual void UpdateAreaNameForId(const NavigationVolumeID id, const char* newShapeName) = 0;

    virtual void StartWorldMonitoring() = 0;
    virtual void StopWorldMonitoring() = 0;

    virtual bool IsInUse() const = 0;

    virtual void CalculateAccessibility() = 0;

    virtual uint32 GetTileIdWhereLocationIsAtForMesh(NavigationMeshID meshID, const Vec3& location) = 0;
    virtual void GetTileBoundsForMesh(NavigationMeshID meshID, uint32 tileID, AABB& bounds) const = 0;

    virtual MNM::TriangleID GetTriangleIDWhereLocationIsAtForMesh(const NavigationAgentTypeID agentID, const Vec3& location) = 0;

    virtual const IOffMeshNavigationManager& GetIOffMeshNavigationManager() const = 0;
    virtual IOffMeshNavigationManager& GetIOffMeshNavigationManager() = 0;
    virtual void ComputeAccessibility(const Vec3& seedPos, NavigationAgentTypeID agentTypeId = NavigationAgentTypeID(0), float range = 0.f, EAccessbilityDir dir = AccessibilityAway) = 0;
    virtual bool TryGetAgentRadiusData(const char* agentType, Vec3& voxelSize, uint16& radiusInVoxels) const = 0;
    // </interfuscator:shuffle>
};



#endif // CRYINCLUDE_CRYCOMMON_INAVIGATIONSYSTEM_H
