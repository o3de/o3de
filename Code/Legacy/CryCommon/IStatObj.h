/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Cry_Math.h"
#include "Cry_Geo.h"
#include "IMaterial.h"

// General forward declaration.
struct SRenderingPassInfo;

//////////////////////////////////////////////////////////////////////////
// Type of static sub object.
//////////////////////////////////////////////////////////////////////////
enum EStaticSubObjectType
{
    STATIC_SUB_OBJECT_MESH,         // This simple geometry part of the multi-sub object geometry.
    STATIC_SUB_OBJECT_HELPER_MESH,  // Special helper mesh, not rendered usually, used for broken pieces.
};

// used for on-CPU voxelization
struct SRayHitInfo
{
    SRayHitInfo()
    {
        memset(this, 0, sizeof(*this));
    }
    //////////////////////////////////////////////////////////////////////////
    // Input parameters.
    Vec3    inReferencePoint;
    Ray     inRay;

    //////////////////////////////////////////////////////////////////////////
    // Output parameters.
    Vec3 vHitPos;
    Vec3 vHitNormal;

    // More inputs
    bool    bInFirstHit;
    bool    bUseCache;
};

// Summary:
//     Interface to hold static object data
struct IStatObj
{
    //////////////////////////////////////////////////////////////////////////
    // SubObject
    //////////////////////////////////////////////////////////////////////////
    struct SSubObject
    {
        EStaticSubObjectType nType;
        Matrix34 localTM;     // Local transformation matrix, relative to parent.
        IStatObj* pStatObj;   // Static object for sub part of CGF.
    };
    //////////////////////////////////////////////////////////////////////////

    virtual ~IStatObj() {}
    // Description:
    //     Provide access to the faces, vertices, texture coordinates, normals and
    //     colors of the object used later for CRenderMesh construction.
    // Return Value:
    //
    // Summary:
    //     Get the object source geometry
    virtual struct IIndexedMesh* GetIndexedMesh(bool bCreateIfNone = false) = 0;

    // Summary:
    //     Get the bounding box
    // Arguments:
    //     Mins - Position of the bottom left close corner of the bounding box
    //     Maxs - Position of the top right far corner of the bounding box
    virtual AABB GetAABB() = 0;

    // Description:
    //     Returns the LOD object, if present.
    // Arguments:
    //     nLodLevel - Level of the LOD
    //     bReturnNearest - if true will return nearest available LOD to nLodLevel.
    // Return Value:
    //     A static object with the desired LOD. The value NULL will be return if there isn't any LOD object for the level requested.
    // Summary:
    //     Get the LOD object
    virtual IStatObj* GetLodObject(int nLodLevel, bool bReturnNearest = false) = 0;

    // Summary:
    //     Returns a pointer to the object
    // Return Value:
    //     A pointer to the current object, which is simply done like this "return this;"
    virtual struct IStatObj* GetIStatObj() { return this; }

    //////////////////////////////////////////////////////////////////////////
    // Interface to the Sub Objects.
    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //    Retrieve number of sub-objects.
    virtual int GetSubObjectCount() const = 0;
    // Summary:
    //    Retrieve sub object by index, where 0 <= nIndex < GetSubObjectCount()
    virtual SSubObject* GetSubObject(int nIndex) = 0;

    // Intersect ray with static object.
    // Ray must be in object local space.
    virtual bool RayIntersection(SRayHitInfo& hitInfo, IMaterial* pCustomMtl = nullptr) = 0;
};
