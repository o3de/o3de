/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "smartptr.h"           // TYPEDEF_AUTOPTR
#include "IMaterial.h"

// forward declarations
//////////////////////////////////////////////////////////////////////
struct ShadowMapFrustum;
struct SRenderingPassInfo;
struct SRendItemSorter;
struct IShader;
struct SPhysGeomArray;
struct CStatObj;

class CRenderObject;
class CDLight;
class IReadStream;
class CRenderObject;
class CLodValue;


TYPEDEF_AUTOPTR(IReadStream);

//! Interface to non animated object
struct phys_geometry;
struct IChunkFile;

// General forward declaration.
class CRenderObject;
struct SMeshLodInfo;

#include "CryHeaders.h"
#include "Cry_Math.h"
#include "Cry_Geo.h"
#include "IPhysics.h"

#define MAX_STATOBJ_LODS_NUM 6

//////////////////////////////////////////////////////////////////////////
// Type of static sub object.
//////////////////////////////////////////////////////////////////////////
enum EStaticSubObjectType
{
    STATIC_SUB_OBJECT_MESH,         // This simple geometry part of the multi-sub object geometry.
    STATIC_SUB_OBJECT_HELPER_MESH,  // Special helper mesh, not rendered usually, used for broken pieces.
    STATIC_SUB_OBJECT_POINT,
    STATIC_SUB_OBJECT_DUMMY,
    STATIC_SUB_OBJECT_XREF,
    STATIC_SUB_OBJECT_CAMERA,
    STATIC_SUB_OBJECT_LIGHT,
};

//////////////////////////////////////////////////////////////////////////
// Flags that can be set on static object.
//////////////////////////////////////////////////////////////////////////
enum EStaticObjectFlags
{
    STATIC_OBJECT_HIDDEN  =    BIT(0), // When set static object will not be displayed.
    STATIC_OBJECT_CLONE     =      BIT(1), // specifies whether this object was cloned for modification
    STATIC_OBJECT_GENERATED  = BIT(2), // tells that the object was generated procedurally (breakable obj., f.i.)
    STATIC_OBJECT_CANT_BREAK = BIT(3), // StatObj has geometry unsuitable for procedural breaking
    STATIC_OBJECT_DEFORMABLE = BIT(4), // StatObj can be procedurally smeared (using SmearStatObj)
    STATIC_OBJECT_COMPOUND   = BIT(5), // StatObj has subobject meshes
    STATIC_OBJECT_MULTIPLE_PARENTS = BIT(6), // Child StatObj referenced by several parents

    // Collisions
    STATIC_OBJECT_NO_PLAYER_COLLIDE = BIT(10),

    // Special flags.
    STATIC_OBJECT_SPAWN_ENTITY    = BIT(20), // StatObj spawns entity when broken.
    STATIC_OBJECT_NO_AUTO_HIDEPOINTS = BIT(22), // Do not generate AI auto hide points around object if it's dynamic
    STATIC_OBJECT_DYNAMIC                   = BIT(23), // mesh data should be kept in system memory (and yes, the name *is* an oxymoron)
};

#define HIT_NO_HIT (-1)
#define HIT_UNKNOWN (-2)

#define HIT_OBJ_TYPE_BRUSH   0
#define HIT_OBJ_TYPE_TERRAIN 1
#define HIT_OBJ_TYPE_VISAREA 2

// used for on-CPU voxelization
struct SRayHitTriangle
{
    SRayHitTriangle() { ZeroStruct(*this); }
    Vec3 v[3];
    Vec2 t[3];
    ColorB c[3];
    Vec3 n;
    _smart_ptr<IMaterial> pMat;
    uint8 nTriArea;
    uint8 nOpacity;
    uint8 nHitObjType;
};

struct SRayHitInfo
{
    SRayHitInfo()
    {
        memset(this, 0, sizeof(*this));
        nHitTriID = HIT_UNKNOWN;
    }
    //////////////////////////////////////////////////////////////////////////
    // Input parameters.
    Vec3    inReferencePoint;
    Ray     inRay;
    bool    bInFirstHit;
    bool        inRetTriangle;
    bool    bUseCache;
    bool    bOnlyZWrite;
    bool    bGetVertColorAndTC;
    float   fMaxHitDistance; // When not 0, only hits with closer distance will be registered.
    Vec3        vTri0;
    Vec3        vTri1;
    Vec3        vTri2;
    float       fMinHitOpacity;

    //////////////////////////////////////////////////////////////////////////
    // Output parameters.
    float fDistance; // Distance from reference point.
    Vec3 vHitPos;
    Vec3 vHitNormal;
    int  nHitMatID; // Material Id that was hit.
    int  nHitTriID; // Triangle Id that was hit.
    int  nHitSurfaceID; // Material Id that was hit.
    struct IRenderMesh* pRenderMesh;
    struct IStatObj* pStatObj;
    Vec2 vHitTC;
    Vec4 vHitColor;
    Vec4 vHitTangent;
    Vec4 vHitBitangent;
    PodArray<SRayHitTriangle>* pHitTris;
};

enum EFileStreamingStatus
{
    ecss_NotLoaded,
    ecss_InProgress,
    ecss_Ready
};

// Interface for streaming of objects like CStatObj.
struct IStreamable
{
    struct SInstancePriorityInfo
    {
        int   nRoundId;
        float fMaxImportance;
    };

    IStreamable()
    {
        ZeroStruct(m_arrUpdateStreamingPrioriryRoundInfo);
        m_eStreamingStatus = ecss_NotLoaded;
        fCurImportance = 0;
        m_nSelectedFrameId = 0;
        m_nStatsInUse = 0;
    }

    bool UpdateStreamingPrioriryLowLevel(float fImportance, int nRoundId, bool bFullUpdate)
    {
        bool bRegister = false;

        if (m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId != nRoundId)
        {
            if (!m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId)
            {
                bRegister = true;
            }

            m_arrUpdateStreamingPrioriryRoundInfo[1] = m_arrUpdateStreamingPrioriryRoundInfo[0];

            m_arrUpdateStreamingPrioriryRoundInfo[0].nRoundId = nRoundId;
            m_arrUpdateStreamingPrioriryRoundInfo[0].fMaxImportance = fImportance;
        }
        else
        {
            m_arrUpdateStreamingPrioriryRoundInfo[0].fMaxImportance = max(m_arrUpdateStreamingPrioriryRoundInfo[0].fMaxImportance, fImportance);
        }

        if (bFullUpdate)
        {
            m_arrUpdateStreamingPrioriryRoundInfo[1] = m_arrUpdateStreamingPrioriryRoundInfo[0];
            m_arrUpdateStreamingPrioriryRoundInfo[1].nRoundId--;
        }

        return bRegister;
    }

    // <interfuscator:shuffle>
    virtual ~IStreamable(){}
    virtual void StartStreaming(bool bFinishNow, IReadStream_AutoPtr* ppStream) = 0;
    virtual int GetStreamableContentMemoryUsage(bool bJustForDebug = false) = 0;
    virtual void ReleaseStreamableContent() = 0;
    virtual void GetStreamableName(string& sName) = 0;
    virtual uint32 GetLastDrawMainFrameId() = 0;
    virtual bool IsUnloadable() const = 0;

    SInstancePriorityInfo m_arrUpdateStreamingPrioriryRoundInfo[2];
    float fCurImportance;
    EFileStreamingStatus m_eStreamingStatus;
    uint32 m_nSelectedFrameId : 31;
    uint32 m_nStatsInUse : 1;
};

struct SMeshBoneMapping_uint8;
struct SSpine;
struct SMeshColor;

// Summary:
//     Interface to hold static object data
struct IStatObj
    : public IStreamable
{
    //! Loading flags
    enum ELoadingFlags
    {
        ELoadingFlagsPreviewMode = BIT(0),
        ELoadingFlagsForceBreakable = BIT(1),
        ELoadingFlagsIgnoreLoDs = BIT(2),
        ELoadingFlagsTessellate = BIT(3), // if e_StatObjTessellation enabled
        ELoadingFlagsJustGeometry = BIT(4), // for streaming, to avoid parsing all chunks
    };

    //////////////////////////////////////////////////////////////////////////
    // SubObject
    //////////////////////////////////////////////////////////////////////////
    struct SSubObject
    {
        SSubObject() { bShadowProxy = 0; }

        EStaticSubObjectType nType;
        string name;
        string properties;
        int nParent;          // Index of the parent sub object, if there`s hierarchy between them.
        Matrix34 tm;          // Transformation matrix.
        Matrix34 localTM;     // Local transformation matrix, relative to parent.
        IStatObj* pStatObj;   // Static object for sub part of CGF.
        Vec3 helperSize;      // Size of the helper (if helper).
        struct IRenderMesh* pWeights; // render mesh with a single deformation weights stream
        unsigned int bIdentityMatrix : 1; // True if sub object matrix is identity.
        unsigned int bHidden : 1; // True if sub object is hidden
        unsigned int bShadowProxy : 1; // Child StatObj has 'shadowproxy' in name
        unsigned int nBreakerJoints : 8; // number of joints that can switch this part to a broken state

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
            pSizer->AddObject(properties);
        }
    };
    //////////////////////////////////////////////////////////////////////////

    // Statistics information about this object.
    struct SStatistics
    {
        int nVertices;
        int nVerticesPerLod[MAX_STATOBJ_LODS_NUM];
        int nIndices;
        int nIndicesPerLod[MAX_STATOBJ_LODS_NUM];
        int nMeshSize;
        int nMeshSizeLoaded;
        int nPhysProxySize;
        int nPhysProxySizeMax;
        int nPhysPrimitives;
        int nDrawCalls;
        int nLods;
        int nSubMeshCount;
        int nNumRefs;
        bool bSplitLods; // Lods split between files.

        // Optional texture sizer.
        ICrySizer* pTextureSizer;
        ICrySizer* pTextureSizer2;

        SStatistics() { Reset(); }

        void Reset()
        {
            pTextureSizer = NULL;
            pTextureSizer2 = NULL;
            nVertices = 0;
            nIndices = 0;
            nMeshSize = 0;
            nMeshSizeLoaded = 0;
            nNumRefs = 0;
            nPhysProxySize = 0;
            nPhysPrimitives = 0;
            nDrawCalls = 0;
            nLods = 0;
            nSubMeshCount = 0;
            bSplitLods = false;
            ZeroStruct(nVerticesPerLod);
            ZeroStruct(nIndicesPerLod);
        }
    };

    // <interfuscator:shuffle>
    // Description:
    //     Increase the reference count of the object.
    // Summary:
    //     Notifies that the object is being used
    virtual int AddRef() = 0;

    // Description:
    //     Decrease the reference count of the object. If the reference count
    //     reaches zero, the object will be deleted from memory.
    // Summary:
    //     Notifies that the object is no longer needed
    virtual int Release() = 0;

    // Description:
    //     Set static object flags.
    // Arguments:
    //     nFlags - flags to set, a combination of EStaticObjectFlags values.
    virtual void SetFlags(int nFlags) = 0;

    // Description:
    //     Retrieve flags set on the static object.
    virtual int GetFlags() const = 0;

    // Sets the default object indicator
    virtual void SetDefaultObject(bool state) = 0;

    // Description:
    //     Retrieves the internal flag m_nVehicleOnlyPhysics.
    virtual unsigned int  GetVehicleOnlyPhysics() = 0;

    // Description:
    //     Retrieves the internal flag m_nIdMaterialBreakable.
    virtual int  GetIDMatBreakable() = 0;

    // Description:
    //     Retrieves the internal flag m_bBreakableByGame.
    virtual unsigned int  GetBreakableByGame() = 0;

    // Description:
    //     Provide access to the faces, vertices, texture coordinates, normals and
    //     colors of the object used later for CRenderMesh construction.
    // Return Value:
    //
    // Summary:
    //     Get the object source geometry
    virtual struct IIndexedMesh* GetIndexedMesh(bool bCreateIfNone = false) = 0;

    // Description:
    //     Create an empty indexed mesh ready to be filled with data
    //      If an indexed mesh already exists it is returned
    // Return Value:
    //     An empty indexed mesh or the existing indexed mesh
    virtual struct IIndexedMesh* CreateIndexedMesh() = 0;

    //! Access to rendering geometry for indoor engine ( optimized vert arrays, lists of shader pointers )
    virtual struct IRenderMesh* GetRenderMesh() = 0;

    // Description:
    //     Returns the physical representation of the object.
    // Arguments:
    //     nType - one of PHYS_GEOM_TYPE_'s, or an explicit slot index
    // Return Value:
    //     A pointer to a phys_geometry structure.
    // Summary:
    //     Get the physic representation
    virtual phys_geometry* GetPhysGeom(int nType = PHYS_GEOM_TYPE_DEFAULT) = 0;

    // Description:
    //       Updates rendermesh's vertices, normals, and tangents with the data provided
    // Summary:
    //       Updates vertices in the range [iVtx0..iVtx0+nVtx-1], vertices are in their original order
    //       (as they are physicalized). Clones the object if necessary to make the modifications
    // Return Value:
    //     modified IStatObj (a clone or this one, if it's already a clone)
    virtual IStatObj* UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0, int nVtx, int* pVtxMap = 0, float rscale = 1.f) = 0;

    // Description:
    //       Skins rendermesh's vertices based on skeleton vertices
    // Summary:
    //       Skins vertices based on mtxSkelToMesh[pSkelVtx[i]]
    //       Clones the object if necessary to make the modifications
    // Return Value:
    //     modified IStatObj (a clone or this one, if it's already a clone)
    virtual IStatObj* SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34& mtxSkelToMesh) = 0;

    // Description:
    //     Sets and replaces the physical representation of the object.
    // Arguments:
    //       pPhysGeom - A pointer to a phys_geometry class.
    //     nType - Pass 0 to set the physic geometry or pass 1 to set the obstruct geometry
    // Summary:
    //     Set the physic representation
    virtual void SetPhysGeom(phys_geometry* pPhysGeom, int nType = 0) = 0;

    // Description:
    //     Returns a tetrahedral lattice, if any (used for breakable objects)
    virtual ITetrLattice* GetTetrLattice() = 0;

    virtual float GetAIVegetationRadius() const = 0;
    virtual void SetAIVegetationRadius(float radius) = 0;

    // Description:
    //     Set default material for the geometry.
    // Arguments:
    //     pMaterial - A valid pointer to the material.
    virtual void SetMaterial(_smart_ptr<IMaterial> pMaterial) = 0;

    // Description:
    //     Returns default material of the geometry.
    // Arguments:
    //     nType - Pass 0 to get the physic geometry or pass 1 to get the obstruct geometry
    // Return Value:
    //     A pointer to a phys_geometry class.
    virtual _smart_ptr<IMaterial> GetMaterial() = 0;
    virtual const _smart_ptr<IMaterial> GetMaterial() const = 0;

    // Return Value:
    //     A Vec3 object containing the bounding box.
    // Summary:
    //     Get the minimal bounding box component
    virtual Vec3 GetBoxMin() = 0;

    // Return Value:
    //     A Vec3 object containing the bounding box.
    // Summary:
    //     Get the minimal bounding box component
    virtual Vec3 GetBoxMax() = 0;

    // Return Value:
    //     A Vec3 object containing the bounding box center.
    // Summary:
    //     Get the center of bounding box
    virtual const Vec3 GetVegCenter() = 0;

    // Arguments:
    //     Minimum bounding box component
    // Summary:
    //     Set the minimum bounding box component
    virtual void    SetBBoxMin(const Vec3& vBBoxMin) = 0;

    // Arguments:
    //     Minimum bounding box component
    // Summary:
    //     Set the minimum bounding box component
    virtual void    SetBBoxMax(const Vec3& vBBoxMax) = 0;

    // Summary:
    //     Get the object radius
    // Return Value:
    //     A float containing the radius
    virtual float GetRadius() = 0;

    // Description:
    //     Reloads one or more component of the object. The possible flags
    //     are FRO_SHADERS, FRO_TEXTURES and FRO_GEOMETRY.
    // Arguments:
    //     nFlags - One or more flag which indicate which element of the object
    //     to reload
    // Summary:
    //     Reloads the object
    virtual void Refresh(int nFlags) = 0;

    // Description:
    //     Registers the object elements into the renderer.
    // Arguments:
    //     rParams   - Render parameters
    //     nLogLevel - Level of the LOD
    // Summary:
    //     Renders the object
    virtual void Render(const struct SRendParams& rParams, const SRenderingPassInfo& passInfo) = 0;

    // Summary:
    //     Get the bounding box
    // Arguments:
    //     Mins - Position of the bottom left close corner of the bounding box
    //     Maxs - Position of the top right far corner of the bounding box
    virtual AABB GetAABB() = 0;

    // Summary:
    //     Generate a random point in object.
    // Arguments:
    //     eForm - Object aspect to generate on (surface, volume, etc)
    virtual float GetExtent(EGeomForm eForm) = 0;
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const = 0;

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
    virtual IStatObj* GetLowestLod() = 0;
    virtual int FindNearesLoadedLOD(int nLodIn, bool bSearchUp = false) = 0;
    virtual int FindHighestLOD(int nBias) = 0;

    virtual bool LoadCGF(const char* filename, bool bLod, unsigned long nLoadingFlags, const void* pData, const int nDataSize) = 0;
    virtual void DisableStreaming() = 0;
    virtual void TryMergeSubObjects(bool bFromStreaming) = 0;
    virtual bool IsUnloadable() const = 0;
    virtual void SetCanUnload(bool value) = 0;

    virtual string& GetFileName() = 0;
    virtual const string& GetFileName() const = 0;

    virtual const string& GetCGFNodeName() const = 0;

    // Summary:
    //     Returns the filename of the object
    // Return Value:
    //     A null terminated string which contain the filename of the object.
    virtual const char* GetFilePath() const = 0;

    // Summary:
    //     Set the filename of the object
    // Arguments:
    //     szFileName - New filename of the object
    // Return Value:
    //       None
    virtual void SetFilePath(const char* szFileName) = 0;

    // Summary:
    //     Returns the name of the geometry
    // Return Value:
    //     A null terminated string which contains the name of the geometry
    virtual const char* GetGeoName() = 0;

    // Summary:
    //     Sets the name of the geometry
    virtual void SetGeoName(const char* szGeoName) = 0;

    // Summary:
    //     Compares if another object is the same
    // Arguments:
    //     szFileName - Filename of the object to compare
    //     szGeomName - Geometry name of the object to compare (optional)
    // Return Value:
    //     A boolean which equals to true in case both object are the same, or false in the opposite case.
    virtual bool IsSameObject(const char* szFileName, const char* szGeomName) = 0;

    // Description:
    //     Will return the position of the helper named in the argument. The
    //     helper should have been specified during the exporting process of
    //     the cgf file.
    // Arguments:
    //     szHelperName - A null terminated string holding the name of the helper
    // Return Value:
    //     A Vec3 object which contains the position.
    // Summary:
    //     Gets the position of a specified helper
    virtual Vec3 GetHelperPos(const char* szHelperName) = 0;

    // Summary:
    //     Gets the transformation matrix of a specified helper, see GetHelperPos
    virtual const Matrix34& GetHelperTM(const char* szHelperName) = 0;

    //! Tell us if the object is not found
    virtual bool IsDefaultObject() = 0;

    // Summary:
    //     Free the geometry data
    virtual void FreeIndexedMesh() = 0;

    // Pushes the underlying tree of objects into the given Sizer object for statistics gathering
    virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;

    //DOC-IGNORE-BEGIN
    //! used for sprites
    virtual float& GetRadiusVert() = 0;

    //! used for sprites
    virtual float& GetRadiusHors() = 0;
    //DOC-IGNORE-END

    // Summary:
    //     Determines if the object has physics capabilities
    virtual bool IsPhysicsExist() = 0;

    // Summary:
    //     Returns a pointer to the object
    // Return Value:
    //     A pointer to the current object, which is simply done like this "return this;"
    virtual struct IStatObj* GetIStatObj() { return this; }

    // Summary:
    //     Invalidates geometry inside IStatObj, will mark hosted IIndexedMesh as invalid.
    // Arguments:
    //     bPhysics - if true will also recreate physics for indexed mesh.
    virtual void Invalidate(bool bPhysics = false, float tolerance = 0.05f) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Interface to the Sub Objects.
    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //    Retrieve number of sub-objects.
    virtual int GetSubObjectCount() const = 0;
    // Summary:
    //    Sets number of sub-objects.
    virtual void SetSubObjectCount(int nCount) = 0;
    // Summary:
    //    Retrieve sub object by index, where 0 <= nIndex < GetSubObjectCount()
    virtual IStatObj::SSubObject* GetSubObject(int nIndex) = 0;
    // Summary:
    //    Check if this object is sub object of another IStatObj.
    virtual bool IsSubObject() const = 0;
    // Summary:
    //    Retrieve parent static object, only relevant when this IStatObj is Sub-object.
    virtual IStatObj* GetParentObject() const = 0;
    // Summary:
    //    Retrieve the static object, from which this one was cloned (if that is the case)
    virtual IStatObj* GetCloneSourceObject() const = 0;
    // Summary:
    //    Find sub-pbject by name.
    virtual IStatObj::SSubObject* FindSubObject(const char* sNodeName) = 0;
    //    Find sub-object by name (including spaces, comma and semi-colon.
    virtual IStatObj::SSubObject* FindSubObject_CGA(const char* sNodeName) = 0;

    // Summary:
    //    Find object by full name (use all the characters)
    virtual IStatObj::SSubObject* FindSubObject_StrStr(const char* sNodeName) = 0;
    //    Remove Sub-Object.
    virtual bool RemoveSubObject(int nIndex) = 0;
    //    Copy Sub-Object.
    virtual bool CopySubObject(int nToIndex, IStatObj* pFromObj, int nFromIndex) = 0;
    //    adds a new sub object
    virtual IStatObj::SSubObject& AddSubObject(IStatObj* pStatObj) = 0;

    // Summary:
    //      Adds subobjects to pent, meshes as parts, joint helpers as breakable joints
    virtual int PhysicalizeSubobjects(IPhysicalEntity* pent, const Matrix34* pMtx, float mass, float density = 0.0f, int id0 = 0, strided_pointer<int> pJointsIdMap = 0, const char* szPropsOverride = 0) = 0;
    // Summary:
    //      Adds all phys geometries to pent, assigns ids starting from id; takes mass and density from the StatObj properties if not set in pgp
    //    for compound objects calls PhysicalizeSubobjects
    //    returns the physical id of the last physicalized part
    virtual int Physicalize(IPhysicalEntity* pent, pe_geomparams* pgp, int id = 0, const char* szPropsOverride = 0) = 0;

    virtual bool IsDeformable() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Save contents of static object to the CGF file.
    //////////////////////////////////////////////////////////////////////////
    // Save object to the CGF file.
    // Arguments:
    //    sFilename
    //          Filename of the CGF file.
    //          Note that the function fails if pOutChunkFile is NULL and the path
    //          to the file does not exist on the drive. You can call
    //          CFileUtil::CreatePath() before SaveToCGF() call to create all
    //          folders that do not exist yet.
    //    pOutChunkFile
    //          Optional output parameter. If it is specified then the file will not be written to
    //          the drive but instead the function returns a pointer to the IChunkFile interface with
    //          filled CGF chunks. Caller of the function is responsible to call Release method
    //          of IChunkFile to release it later.
    virtual bool SaveToCGF(const char* sFilename, IChunkFile** pOutChunkFile = NULL, bool bHavePhysicalProxy = false) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //    Clones static geometry, Makes an exact copy of the Static object and the contained geometry.
    //virtual IStatObj* Clone(bool bCloneChildren=true, bool nDynamic=false) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //    Clones static geometry, Makes an exact copy of the Static object and the contained geometry.
    virtual IStatObj* Clone(bool bCloneGeometry, bool bCloneChildren, bool bMeshesOnly) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Summary:
    //    makes sure that both objects have one-to-one vertex correspondance
    //    sets MorphBuddy for this object's render mesh
    //    returns 0 if failed (due to objects having no vertex maps most likely)
    virtual int SetDeformationMorphTarget(IStatObj* pDeformed) = 0;

    // chages the weights of the deformation morphing according to point, radius, and strength
    // (radius==0 updates all weights of all vertices)
    //  if the object is a compound object, updates the weights of its subobjects that have deformation morphs; clones the object if necessary
    //   otherwise, updates the weights passed as a pWeights param
    virtual IStatObj* DeformMorph(const Vec3& pt, float r, float strength, IRenderMesh* pWeights = 0) = 0;

    // hides all non-physicalized geometry, clones the object if necessary
    virtual IStatObj* HideFoliage() = 0;

    // serializes the StatObj's mesh into a stream
    virtual int Serialize(TSerialize ser) = 0;

    // Get object properties as loaded from CGF.
    virtual const char* GetProperties() = 0;

    // Get physical properties specified for object.
    virtual bool GetPhysicalProperties(float& mass, float& density) = 0;

    // Returns the last B operand for this object as A, along with its relative scale
    virtual IStatObj* GetLastBooleanOp(float& scale) = 0;

    // Intersect ray with static object.
    // Ray must be in object local space.
    virtual bool RayIntersection(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0) = 0;

    // Intersect lineseg with static object. Works on dedi server as well.
    // Lineseg must be in object local space. Returns the hit position and the surface type id of the point hit.
    virtual bool LineSegIntersection(const Lineseg& lineSeg, Vec3& hitPos, int& surfaceTypeId) = 0;

    // Debug Draw this static object.
    virtual void DebugDraw(const struct SGeometryDebugDrawInfo& info, float fExtrdueScale = 0.01f) = 0;

    // Fill statistics about the level.
    virtual void GetStatistics(SStatistics& stats) = 0;

    // Returns initial hide mask
    virtual uint64 GetInitialHideMask() = 0;

    // Updates hide mask as new mask = (mask & maskAND) | maskOR
    virtual uint64 UpdateInitialHideMask(uint64 maskAND = 0ul - 1ul, uint64 maskOR = 0) = 0;

    // Set the filename of the mesh of the next state (for example damaged version)
    virtual void SetStreamingDependencyFilePath(const char* szFileName) = 0;

    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo) = 0;

    // Returns the distance for the first LOD switch. Used for brushes and vegetation.
    virtual float GetLodDistance() const = 0;

    // Returns true if the mesh has been stripped
    virtual bool IsMeshStrippedCGF() const = 0;

    virtual void LoadLowLODs(bool bUseStreaming, unsigned long nLoadingFlags) = 0;

    // Indicates if lods have been loaded
    virtual bool AreLodsLoaded() const = 0;

    // Indicates if a garbage check should be done
    virtual bool CheckGarbage() const = 0;

    // Sets state of check garbage flags
    virtual void SetCheckGarbage(bool val) = 0;

    // Returns the number of child references
    virtual int CountChildReferences() const = 0;

    // Returns the user count
    virtual int GetUserCount() const = 0;

    // Shutdown
    virtual void ShutDown() = 0;

    virtual int GetMaxUsableLod() const = 0;
    virtual int GetMinUsableLod() const = 0;

    virtual SMeshBoneMapping_uint8* GetBoneMapping() const = 0;

    virtual int GetSpineCount() const = 0;
    virtual SSpine* GetSpines() const = 0;

    virtual IStatObj* GetLodLevel0() = 0;
    virtual void SetLodLevel0(IStatObj* lod) = 0;
    virtual _smart_ptr<CStatObj>* GetLods() = 0;
    virtual int GetLoadedLodsNum() = 0;

    virtual bool UpdateStreamableComponents(float fImportance, const Matrix34A& objMatrix, bool bFullUpdate, int nNewLod) = 0;

    virtual void RenderInternal(CRenderObject* pRenderObject, uint64 nSubObjectHideMask, const CLodValue& lodValue, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw) = 0;
    virtual void RenderObjectInternal(CRenderObject* pRenderObject, int nLod, uint8 uLodDissolveRef, bool dissolveOut, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw) = 0;
    virtual void RenderSubObject(CRenderObject* pRenderObject, int nLod, int nSubObjId, const Matrix34A& renderTM, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw) = 0;
    virtual void RenderSubObjectInternal(CRenderObject* pRenderObject, int nLod, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw) = 0;
    virtual void RenderRenderMesh(CRenderObject* pObj, struct SInstancingInfo* pInstInfo, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;

    virtual SPhysGeomArray& GetArrPhysGeomInfo() = 0;
    virtual bool IsLodsAreLoadedFromSeparateFile() = 0;

    virtual void StartStreaming(bool bFinishNow, IReadStream_AutoPtr* ppStream) = 0;
    virtual void UpdateStreamingPrioriryInternal(const Matrix34A& objMatrix, float fDistance, bool bFullUpdate) = 0;

    virtual void SetMerged(bool state) = 0;

    virtual int GetRenderMeshMemoryUsage() const = 0;
    virtual void SetLodObject(int nLod, IStatObj* pLod) = 0;
    virtual int GetLoadedTrisCount() const = 0;
    virtual int GetRenderTrisCount() const = 0;
    virtual int GetRenderMatIds() const = 0;

    virtual bool IsUnmergable() const = 0;
    virtual void SetUnmergable(bool state) = 0;

    virtual int GetSubObjectMeshCount() const = 0;
    virtual void SetSubObjectMeshCount(int count) = 0;
    virtual void CleanUnusedLods() = 0;

    virtual AZStd::vector<SMeshColor>& GetClothData() = 0;
};
