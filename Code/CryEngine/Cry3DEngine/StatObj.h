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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRY3DENGINE_STATOBJ_H
#define CRYINCLUDE_CRY3DENGINE_STATOBJ_H
#pragma once

#if !defined(CONSOLE)
#   define TRACE_CGF_LEAKS
#   define SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS
#endif

class CIndexedMesh;
class CRenderObject;
class CContentCGF;
struct CNodeCGF;
struct CMaterialCGF;
struct phys_geometry;
struct IIndexedMesh;

#include "../Cry3DEngine/Cry3DEngineBase.h"
#include "CryArray.h"

#include <IStatObj.h>
#include <IStreamEngine.h>
#include "RenderMeshUtils.h"
#include "GeomQuery.h"
#include <AzCore/Jobs/LegacyJobExecutor.h>

#define MAX_PHYS_GEOMS_TYPES 4

struct SDeformableMeshData
{
    IGeometry* pInternalGeom;
    int* pVtxMap;
    unsigned int* pUsedVtx;
    int* pVtxTri;
    int* pVtxTriBuf;
    float* prVtxValency;
    Vec3* pPrevVtx;
    float kViscosity;
};

struct SSpine
{
    ~SSpine() { delete[] pVtx; delete[] pVtxCur; delete[] pSegDim; delete[] pStiffness; delete[] pDamping; delete[] pThickness; }
    SSpine() : bActive(false), pVtx(nullptr), pVtxCur(nullptr), pSegDim(nullptr),
        pStiffness(nullptr), pDamping(nullptr), pThickness(nullptr),
        nVtx(0), len(0.0f), navg(0.0f, 0.0f, 0.0f), idmat(0), iAttachSpine(0), iAttachSeg(0) {}

    bool bActive;
    Vec3* pVtx;
    Vec3* pVtxCur;
    Vec4* pSegDim;

    /// Per bone UDP for stiffness, damping and thickness for touch bending vegetation
    float* pStiffness;
    float* pDamping;
    float* pThickness;

    int nVtx;
    float len;
    Vec3 navg;
    int idmat;
    int iAttachSpine;
    int iAttachSeg;
};

struct SClothTangentVtx
{
    int ivtxT; // for each vertex, specifies the iThisVtx->ivtxT edge, which is the closest to the vertex's tangent vector
    Vec3 edge; // that edge's projection on the vertex's normal basis
    int sgnNorm; // sign of phys normal * normal from the basis
};

struct SSkinVtx
{
    int bVolumetric;
    int idx[4];
    float w[4];
    Matrix33 M;
};

struct SDelayedSkinParams
{
    Matrix34 mtxSkelToMesh;
    IGeometry* pPhysSkel;
    float r;
};

struct SPhysGeomThunk
{
    phys_geometry* pgeom;
    int type;
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const
    {
        //      pSizer->AddObject(pgeom);
    }
};

struct SPhysGeomArray
{
    phys_geometry* operator[](int idx) const
    {
        if (idx < PHYS_GEOM_TYPE_DEFAULT)
        {
            return idx < (int)m_array.size() ? m_array[idx].pgeom : 0;
        }
        else
        {
            int i;
            for (i = m_array.size() - 1; i >= 0 && m_array[i].type != idx; i--)
            {
                ;
            }
            return i >= 0 ? m_array[i].pgeom : 0;
        }
    }
    void SetPhysGeom(phys_geometry* pgeom, int idx = PHYS_GEOM_TYPE_DEFAULT, int type = PHYS_GEOM_TYPE_DEFAULT)
    {
        int i;
        if (idx < PHYS_GEOM_TYPE_DEFAULT)
        {
            i = idx, idx = type;
        }
        else
        {
            for (i = 0; i < (int)m_array.size() && m_array[i].type != idx; i++)
            {
                ;
            }
        }
        if (pgeom)
        {
            if (i >= (int)m_array.size())
            {
                m_array.resize(i + 1);
            }
            m_array[i].pgeom = pgeom;
            m_array[i].type = idx;
        }
        else if (i < (int)m_array.size())
        {
            m_array.erase(m_array.begin() + i);
        }
    }
    int GetGeomCount() { return m_array.size(); }
    int GetGeomType(int idx) { return idx >= PHYS_GEOM_TYPE_DEFAULT ? idx : m_array[idx].type; }
    std::vector<SPhysGeomThunk> m_array;
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_array);
    }
};


struct SSyncToRenderMeshContext
{
    Vec3* vmin, *vmax;
    int iVtx0;
    int nVtx;
    strided_pointer<Vec3> pVtx;
    int* pVtxMap;
    int mask;
    float rscale;
    SClothTangentVtx* ctd;
    strided_pointer<Vec3> pMeshVtx;
    strided_pointer<SPipTangents> pTangents;
    strided_pointer<Vec3> pNormals; // TODO: change Vec3 to SPipNormal
    CStatObj* pObj;
    AZ::LegacyJobExecutor jobExecutor;

    void Set(Vec3* _vmin, Vec3* _vmax, int _iVtx0, int _nVtx, strided_pointer<Vec3> _pVtx, int* _pVtxMap
        , int _mask, float _rscale, SClothTangentVtx* _ctd, strided_pointer<Vec3> _pMeshVtx
        , strided_pointer<SPipTangents> _pTangents, strided_pointer<Vec3> _pNormals, CStatObj* _pObj)
    {
        vmin = _vmin;
        vmax = _vmax;
        iVtx0 = _iVtx0;
        nVtx = _nVtx;
        pVtx = _pVtx;
        pVtxMap = _pVtxMap;
        mask = _mask;
        rscale = _rscale;
        ctd = _ctd;
        pMeshVtx = _pMeshVtx;
        pTangents = _pTangents;
        pNormals = _pNormals;
        pObj = _pObj;
    }
};

struct CStatObj
    : public IStatObj
    , public IStreamCallback
    , public stl::intrusive_linked_list_node<CStatObj>
    , public Cry3DEngineBase
{
    CStatObj();
    ~CStatObj();

public:
    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    volatile int m_nUsers; // reference counter

    uint32 m_nLastDrawMainFrameId;

    _smart_ptr<IRenderMesh> m_pRenderMesh;
#ifdef SERVER_CHECKS
    CMesh* m_pMesh;     // Used by the dedicated server where the render mesh doesn't exist
#endif

    CryCriticalSection m_streamingMeshLock;
    _smart_ptr<IRenderMesh> m_pStreamedRenderMesh;
    _smart_ptr<IRenderMesh> m_pMergedRenderMesh;

    // Used by hierarchical breaking to hide sub-objects that initially must be hidden.
    uint64 m_nInitialSubObjHideMask;

    CIndexedMesh* m_pIndexedMesh;
    volatile int m_lockIdxMesh;

    string m_szFileName;
    string m_szGeomName;
    string m_szProperties;
    string m_szStreamingDependencyFilePath;

    int m_nLoadedTrisCount;
    int m_nLoadedVertexCount;
    int m_nRenderTrisCount;
    int m_nRenderMatIds;
    float m_fGeometricMeanFaceArea;
    float m_fLodDistance;

    // Default material.
    _smart_ptr<IMaterial> m_pMaterial;

    float m_fObjectRadius;
    float m_fRadiusHors;
    float m_fRadiusVert;

    Vec3 m_vBoxMin, m_vBoxMax, m_vVegCenter;

    SPhysGeomArray m_arrPhysGeomInfo;
    ITetrLattice* m_pLattice;
    IStatObj* m_pLastBooleanOp;
    float m_lastBooleanOpScale;

    _smart_ptr<CStatObj>* m_pLODs;
    IStatObj* m_pLod0; // Level 0 stat object. (Pointer to the original object of the LOD)
    unsigned int m_nMinUsableLod0 : 8;  // What is the minimal LOD that can be used as LOD0.
    unsigned int m_nMaxUsableLod0 : 8; // What is the maximal LOD that can be used as LOD0.
    unsigned int m_nMaxUsableLod : 8;  // What is the maximal LOD that can be used.
    unsigned int m_nLoadedLodsNum : 8;  // How many lods loaded.


    string m_cgfNodeName;

    //////////////////////////////////////////////////////////////////////////
    // Externally set flags from enum EStaticObjectFlags.
    //////////////////////////////////////////////////////////////////////////
    int m_nFlags;

    //////////////////////////////////////////////////////////////////////////
    // Internal Flags.
    //////////////////////////////////////////////////////////////////////////
    unsigned int m_bCheckGarbage : 1;
    unsigned int m_bCanUnload : 1;
    unsigned int m_bLodsLoaded : 1;
    unsigned int m_bDefaultObject : 1;
    unsigned int m_bOpenEdgesTested : 1;
    unsigned int m_bSubObject : 1;       // This is sub object.
    unsigned int m_bVehicleOnlyPhysics : 1;  // Object can be used for collisions with vehicles only
    unsigned int m_bBreakableByGame : 1; // material is marked as breakable by game
    unsigned int m_bSharesChildren : 1; // means its subobjects belong to another parent statobj
    unsigned int m_bHasDeformationMorphs : 1;
    unsigned int m_bTmpIndexedMesh : 1; // indexed mesh is temporary and can be deleted after MakeRenderMesh
    unsigned int m_bUnmergable : 1; // Set if sub objects cannot be merged together to the single render merge.
    unsigned int m_bMerged : 1; // Set if sub objects merged together to the single render merge.
    unsigned int m_bMergedLODs : 1; // Set if m_pLODs were created while merging LODs
    unsigned int m_bLowSpecLod0Set : 1;
    unsigned int m_bHaveOcclusionProxy : 1; // If this stat object or its childs have occlusion proxy.
    unsigned int m_bLodsAreLoadedFromSeparateFile : 1;
    unsigned int m_bNoHitRefinement : 1; // doesn't refine bullet hits against rendermesh
    unsigned int m_bDontOccludeExplosions : 1; // don't act as an explosion occluder in physics
    unsigned int m_hasClothTangentsData : 1;
    unsigned int m_hasSkinInfo : 1;
    unsigned int m_bMeshStrippedCGF : 1; // This CGF was loaded from the Mesh Stripped CGF, (in Level Cache)
    unsigned int m_isDeformable : 1; // This cgf is deformable in the sense that it has a special renderpath
    unsigned int m_isProxyTooBig : 1;
    unsigned int m_bHasStreamOnlyCGF : 1;

    int m_idmatBreakable;   // breakable id for the physics
    //////////////////////////////////////////////////////////////////////////

    // streaming
    int m_nRenderMeshMemoryUsage;
    int m_nMergedMemoryUsage;
    int m_arrRenderMeshesPotentialMemoryUsage[2];
    IReadStreamPtr m_pReadStream;

#if !defined (_RELEASE)
    static float s_fStreamingTime;
    static int s_nBandwidth;
    float m_fStreamingStart;
#endif

    //////////////////////////////////////////////////////////////////////////

    uint16* m_pMapFaceToFace0;
    union
    {
        SClothTangentVtx* m_pClothTangentsData;
        SSkinVtx* m_pSkinInfo;
    };
    SDelayedSkinParams* m_pDelayedSkinParams;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Bendable Foliage.
    //////////////////////////////////////////////////////////////////////////
    SSpine* m_pSpines;
    int m_nSpines;
    struct SMeshBoneMapping_uint8* m_pBoneMapping;
    std::vector<uint16> m_chunkBoneIds;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // for debug purposes
    //////////////////////////////////////////////////////////////////////////
#ifdef TRACE_CGF_LEAKS
    string  m_sLoadingCallstack;
#endif

private:
    //////////////////////////////////////////////////////////////////////////
    // Sub objects.
    //////////////////////////////////////////////////////////////////////////
    std::vector<SSubObject> m_subObjects;
    CStatObj* m_pParentObject;  // Parent object (Must not be smart pointer).
    CStatObj* m_pClonedSourceObject;  // If this is cloned object, pointer to original source object (Must not be smart pointer).
    int m_nSubObjectMeshCount;
    int m_nNodeCount;

    CGeomExtents m_Extents;                     // Cached extents for random pos generation.

    //////////////////////////////////////////////////////////////////////////
    // Special AI/Physics parameters.
    //////////////////////////////////////////////////////////////////////////
    float m_aiVegetationRadius;
    float m_phys_mass;
    float m_phys_density;

    //////////////////////////////////////////////////////////////////////////
    // used only in the editor
    //////////////////////////////////////////////////////////////////////////
#ifdef SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS
    float* m_pHeightmap;
    int m_nHeightmapSize;
    float m_fOcclusionAmount;
#endif

    SSyncToRenderMeshContext* m_pAsyncUpdateContext;

    //////////////////////////////////////////////////////////////////////////
    // Cloth data
    //////////////////////////////////////////////////////////////////////////
    AZStd::vector<SMeshColor> m_clothData;

    //////////////////////////////////////////////////////////////////////////
    // METHODS.
    //////////////////////////////////////////////////////////////////////////
public:
    virtual void SetDefaultObject(bool state) override { m_bDefaultObject = state; }

    //////////////////////////////////////////////////////////////////////////
    // Fast non virtual access functions.
    ILINE IStatObj::SSubObject& SubObject(int nIndex) { return m_subObjects[nIndex]; };
    ILINE int SubObjectCount() const { return m_subObjects.size(); };
    //////////////////////////////////////////////////////////////////////////

    virtual void SetCanUnload(bool value) override { m_bCanUnload = value; }
    virtual bool IsUnloadable() const { return m_bCanUnload; }
    virtual bool IsUnmergable() const { return m_bUnmergable; }
    virtual void SetUnmergable(bool state) { m_bUnmergable = state; }

    void DisableStreaming();

    virtual bool AreLodsLoaded() const override { return m_bLodsLoaded; }
    virtual SPhysGeomArray& GetArrPhysGeomInfo() override { return m_arrPhysGeomInfo; }

    IIndexedMesh* GetIndexedMesh(bool bCreatefNone = false);
    IIndexedMesh* CreateIndexedMesh();
    void ReleaseIndexedMesh(bool bRenderMeshUpdated = false);
    ILINE const Vec3 GetVegCenter() { return m_vVegCenter; }
    ILINE float GetRadius() { return m_fObjectRadius; }

    virtual void SetFlags(int nFlags) override { m_nFlags = nFlags; };
    virtual int GetFlags() const override { return m_nFlags; };
    virtual bool IsLodsAreLoadedFromSeparateFile() override { return m_bLodsAreLoadedFromSeparateFile; }

    virtual int GetSubObjectMeshCount() const override { return m_nSubObjectMeshCount; }
    virtual void SetSubObjectMeshCount(int count) { m_nSubObjectMeshCount = count; }

    virtual unsigned int GetVehicleOnlyPhysics() { return m_bVehicleOnlyPhysics; };
    virtual int GetIDMatBreakable() { return m_idmatBreakable; };
    virtual unsigned int GetBreakableByGame() { return m_bBreakableByGame; };

    //Note: This function checks both the children and root data
    //It should really be 'has any deformable objects' 
    //Should eventually be refactored as part of an eventual statobj refactor.
    virtual bool IsDeformable() override;

    // Loader
    bool LoadCGF(const char* filename, bool bLod, unsigned long nLoadingFlags, const void* pData, const int nDataSize);
    bool LoadCGF_Int(const char* filename, bool bLod, unsigned long nLoadingFlags, const void* pData, const int nDataSize);

    //////////////////////////////////////////////////////////////////////////
    void SetMaterial(_smart_ptr<IMaterial> pMaterial);
    _smart_ptr<IMaterial> GetMaterial() { return m_pMaterial; }
    const _smart_ptr<IMaterial> GetMaterial() const { return m_pMaterial; }
    //////////////////////////////////////////////////////////////////////////

    void RenderInternal(CRenderObject* pRenderObject, uint64 nSubObjectHideMask, const CLodValue& lodValue, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw);
    void RenderObjectInternal(CRenderObject* pRenderObject, int nLod, uint8 uLodDissolveRef, bool dissolveOut, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw);
    void RenderSubObject(CRenderObject* pRenderObject, int nLod,
        int nSubObjId, const Matrix34A& renderTM, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw);
    void RenderSubObjectInternal(CRenderObject* pRenderObject, int nLod, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, bool forceStaticDraw);
    virtual void Render(const SRendParams& rParams, const SRenderingPassInfo& passInfo);
    void RenderRenderMesh(CRenderObject* pObj, struct SInstancingInfo* pInstInfo, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);
    phys_geometry* GetPhysGeom(int nGeomType = PHYS_GEOM_TYPE_DEFAULT) { return m_arrPhysGeomInfo[nGeomType]; }
    void SetPhysGeom(phys_geometry* pPhysGeom, int nGeomType = PHYS_GEOM_TYPE_DEFAULT)
    {
        m_arrPhysGeomInfo.SetPhysGeom(pPhysGeom, nGeomType);
    }
    ITetrLattice* GetTetrLattice() { return m_pLattice; }

    float GetAIVegetationRadius() const { return m_aiVegetationRadius; }
    void SetAIVegetationRadius(float radius) { m_aiVegetationRadius = radius; }

    //! Refresh object ( reload shaders or/and object geometry )
    virtual void Refresh(int nFlags);

    IRenderMesh* GetRenderMesh() { return m_pRenderMesh; };
    void SetRenderMesh(IRenderMesh* pRM);

    const char* GetFilePath() const { return (m_szFileName); }
    void SetFilePath(const char* szFileName) { m_szFileName = szFileName; }
    const char* GetGeoName() { return (m_szGeomName); }
    void SetGeoName(const char* szGeoName) { m_szGeomName = szGeoName; }
    bool IsSameObject(const char* szFileName, const char* szGeomName);

    //set object's min/max bbox
    void  SetBBoxMin(const Vec3& vBBoxMin) { m_vBoxMin = vBBoxMin; }
    void  SetBBoxMax(const Vec3& vBBoxMax) { m_vBoxMax = vBBoxMax; }
    Vec3 GetBoxMin() { return m_vBoxMin; }
    Vec3 GetBoxMax() { return m_vBoxMax; }
    AABB GetAABB() { return AABB(m_vBoxMin, m_vBoxMax); }
    AABB GetAABB() const { return AABB(m_vBoxMin, m_vBoxMax); }

    virtual float GetExtent(EGeomForm eForm);
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;

    virtual Vec3 GetHelperPos(const char* szHelperName);
    virtual const Matrix34& GetHelperTM(const char* szHelperName);

    virtual float& GetRadiusVert() override { return m_fRadiusVert; }
    virtual float& GetRadiusHors() override { return m_fRadiusHors; }

    virtual int AddRef();

    virtual int Release();
    int GetNumRefs() const { return m_nUsers; }

    virtual bool IsDefaultObject() { return (m_bDefaultObject); }

    int GetLoadedTrisCount() const override { return m_nLoadedTrisCount; }
    int GetRenderTrisCount() const override { return m_nRenderTrisCount; }
    int GetRenderMatIds() const override { return m_nRenderMatIds; }

    // Load LODs
    void SetLodObject(int nLod, IStatObj* pLod) override;
    bool LoadLowLODS_Prep(bool bUseStreaming, unsigned long nLoadingFlags);
    IStatObj* LoadLowLODS_Load(int nLodLevel, bool bUseStreaming, unsigned long nLoadingFlags, const void* pData, int nDataLen);
    void LoadLowLODS_Finalize(int nLoadedLods, IStatObj* loadedLods[MAX_STATOBJ_LODS_NUM]);
    void LoadLowLODs(bool bUseStreaming, unsigned long nLoadingFlags);

    // Free render resources for unused upper LODs.
    virtual void CleanUnusedLods() override;

    virtual void FreeIndexedMesh();
    bool RenderDebugInfo(CRenderObject* pObj, const SRenderingPassInfo& passInfo);

    //! Release method.
    void GetMemoryUsage(class ICrySizer* pSizer) const;

    void ShutDown();
    void Init();

    //  void CheckLoaded();
    IStatObj* GetLodObject(int nLodLevel, bool bReturnNearest = false) override;
    IStatObj* GetLowestLod() override;
    _smart_ptr<CStatObj>* GetLods() override { return m_pLODs; }
    int GetLoadedLodsNum() override { return m_nLoadedLodsNum; }
    void SetMerged(bool state)  override { m_bMerged = state; }
    int GetRenderMeshMemoryUsage() const override { return m_nRenderMeshMemoryUsage; }
    int FindNearesLoadedLOD(int nLodIn, bool bSearchUp = false) override;
    int FindHighestLOD(int nBias) override;

    // interface IStreamCallback -----------------------------------------------------

    void StreamAsyncOnComplete(IReadStream* pStream, unsigned nError) override;
    void StreamOnComplete(IReadStream* pStream, unsigned nError) override;

    // -------------------------------------------------------------------------------

    void StartStreaming(bool bFinishNow, IReadStream_AutoPtr* ppStream) override;
    void UpdateStreamingPrioriryInternal(const Matrix34A& objMatrix, float fDistance, bool bFullUpdate);

    void MakeCompiledFileName(char* szCompiledFileName, int nMaxLen);

    bool IsPhysicsExist();
    bool IsSphereOverlap(const Sphere& sSphere);
    void Invalidate(bool bPhysics = false, float tolerance = 0.05f);

    IStatObj* UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0, int nVtx, int* pVtxMap = 0, float rscale = 1.f);
    bool HasSkinInfo(float skinRadius = -1.0f) { return m_hasSkinInfo && m_pSkinInfo && (skinRadius < 0.0f || m_pSkinInfo[m_nLoadedVertexCount].w[0] == skinRadius); }
    void PrepareSkinData(const Matrix34& mtxSkelToMesh, IGeometry* pPhysSkel, float r = 0.0f);
    IStatObj* SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34& mtxSkelToMesh);

    //////////////////////////////////////////////////////////////////////////
    // Sub objects.
    //////////////////////////////////////////////////////////////////////////
    virtual int GetSubObjectCount() const { return m_subObjects.size(); }
    virtual void SetSubObjectCount(int nCount);
    virtual IStatObj::SSubObject* FindSubObject(const char* sNodeName);
    virtual IStatObj::SSubObject* FindSubObject_StrStr(const char* sNodeName);
    virtual IStatObj::SSubObject* FindSubObject_CGA(const char* sNodeName);
    virtual IStatObj::SSubObject* GetSubObject(int nIndex)
    {
        if (nIndex >= 0 && nIndex < (int)m_subObjects.size())
        {
            return &m_subObjects[nIndex];
        }
        else
        {
            return 0;
        }
    }
    virtual bool RemoveSubObject(int nIndex);
    virtual IStatObj* GetParentObject() const { return m_pParentObject; }
    virtual IStatObj* GetCloneSourceObject() const { return m_pClonedSourceObject; }
    virtual bool IsSubObject() const { return m_bSubObject; };
    virtual bool CopySubObject(int nToIndex, IStatObj* pFromObj, int nFromIndex);
    virtual int PhysicalizeSubobjects(IPhysicalEntity* pent, const Matrix34* pMtx, float mass, float density = 0.0f, int id0 = 0,
        strided_pointer<int> pJointsIdMap = 0, const char* szPropsOverride = 0);
    virtual IStatObj::SSubObject& AddSubObject(IStatObj* pStatObj);
    virtual int Physicalize(IPhysicalEntity* pent, pe_geomparams* pgp, int id = -1, const char* szPropsOverride = 0);
    //////////////////////////////////////////////////////////////////////////

    virtual bool SaveToCGF(const char* sFilename, IChunkFile** pOutChunkFile = NULL, bool bHavePhiscalProxy = false);

    //virtual IStatObj* Clone(bool bCloneChildren=true, bool nDynamic=false);
    virtual IStatObj* Clone(bool bCloneGeometry, bool bCloneChildren, bool bMeshesOnly);

    virtual int SetDeformationMorphTarget(IStatObj* pDeformed);
    virtual int SubobjHasDeformMorph(int iSubObj);
    virtual IStatObj* DeformMorph(const Vec3& pt, float r, float strength, IRenderMesh* pWeights = 0);

    virtual IStatObj* HideFoliage();

    virtual int Serialize(TSerialize ser);

    // Get object properties as loaded from CGF.
    virtual const char* GetProperties() { return m_szProperties.c_str(); };
    virtual void SetProperties(const char* props) { m_szProperties = props; ParseProperties(); }

    virtual bool GetPhysicalProperties(float& mass, float& density);

    virtual IStatObj* GetLastBooleanOp(float& scale) { scale = m_lastBooleanOpScale; return m_pLastBooleanOp; }

    // Intersect ray with static object.
    // Ray must be in object local space.
    virtual bool RayIntersection(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0);
    virtual bool LineSegIntersection(const Lineseg& lineSeg, Vec3& hitPos, int& surfaceTypeId);

    virtual void DebugDraw(const SGeometryDebugDrawInfo& info, float fExtrdueScale = 0.01f);
    virtual void GetStatistics(SStatistics& stats);
    //////////////////////////////////////////////////////////////////////////

    virtual uint64 GetInitialHideMask() { return m_nInitialSubObjHideMask; }
    virtual uint64 UpdateInitialHideMask(uint64 maskAND = 0ul - 1ul, uint64 maskOR = 0) { return m_nInitialSubObjHideMask &= maskAND |= maskOR; }

    virtual void SetStreamingDependencyFilePath(const char* szFileName)
    {
        const bool streamingDependencyLoop = CheckForStreamingDependencyLoop(szFileName);
        if (streamingDependencyLoop)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "StatObj '%s' cannot set '%s' as a streaming dependency as it would result in a looping dependency.", GetFilePath(), szFileName);
            return;
        }

        m_szStreamingDependencyFilePath = szFileName;
    }

#ifdef SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS
    float GetOcclusionAmount();
    void CheckUpdateObjectHeightmap();
    float GetObjectHeight(float x, float y);
#endif

    virtual int GetMaxUsableLod() const override;
    virtual int GetMinUsableLod() const override;
    virtual SMeshBoneMapping_uint8* GetBoneMapping() const override { return m_pBoneMapping; }
    virtual int GetSpineCount() const override { return m_nSpines; }
    virtual SSpine* GetSpines() const override { return m_pSpines; }

    void RenderStreamingDebugInfo(CRenderObject* pRenderObject);
    void RenderCoverInfo(CRenderObject* pRenderObject);

    virtual int CountChildReferences() const override;
    void ReleaseStreamableContent();
    int GetStreamableContentMemoryUsage(bool bJustForDebug = false);
    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo);
    virtual float GetLodDistance() const override { return m_fLodDistance; }
    virtual bool UpdateStreamableComponents(float fImportance, const Matrix34A& objMatrix, bool bFullUpdate, int nNewLod) override;
    void GetStreamableName(string& sName)
    {
        sName = m_szFileName;
        if (m_szGeomName.length())
        {
            sName += " - ";
            sName += m_szGeomName;
        }
    };
    void GetStreamFilePath(stack_string& strOut);
    void FillRenderObject(const SRendParams& rParams, IRenderNode* pRenderNode, _smart_ptr<IMaterial> pMaterial,
        SInstancingInfo* pInstInfo, CRenderObject*& pObj, const SRenderingPassInfo& passInfo);
    virtual uint32 GetLastDrawMainFrameId() { return m_nLastDrawMainFrameId; }

    // Allow pooled allocs
    static void* operator new (size_t size);
    static void operator delete (void* pToFree);

    // Used in ObjMan.
    void TryMergeSubObjects(bool bFromStreaming) override;
    bool IsMeshStrippedCGF() const { return m_bMeshStrippedCGF; }
    string& GetFileName() override { return m_szFileName; }
    const string& GetFileName() const override { return m_szFileName; }

    const string& GetCGFNodeName() const override { return m_cgfNodeName; }

    int GetUserCount() const override { return m_nUsers; }
    bool CheckGarbage() const override { return m_bCheckGarbage; }
    void SetCheckGarbage(bool val) override { m_bCheckGarbage = val; }
    IStatObj* GetLodLevel0() override { return m_pLod0; }
    void SetLodLevel0(IStatObj* lod) override { m_pLod0 = lod; }

    AZStd::vector<SMeshColor>& GetClothData() override { return m_clothData; }

protected:
    // Called by async stream callback.
    bool LoadStreamRenderMeshes(const char* filename, const void* pData, const int nDataSize, bool bLod);
    // Called by sync stream complete callback.
    void CommitStreamRenderMeshes();


    void MergeSubObjectsRenderMeshes(bool bFromStreaming, CStatObj* pLod0, int nLod);
    void UnMergeSubObjectsRenderMeshes();
    bool CanMergeSubObjects();
    bool IsMatIDReferencedByObj(uint16 matID);

    //  bool LoadCGF_Info( const char *filename );
    CStatObj* MakeStatObjFromCgfNode(CContentCGF* pCGF, CNodeCGF* pNode, bool bLod, int nLoadingFlags, AABB& commonBBox);
    void ParseProperties();

    void CalcRadiuses();
    void GetStatisticsNonRecursive(SStatistics& stats);


    // Creates static object contents from mesh.
    // Return true if successful.
    _smart_ptr<IRenderMesh> MakeRenderMesh(CMesh* pMesh, bool bDoRenderMesh);
    void MakeRenderMesh();


    const char* stristr(const char* szString, const char* szSubstring)
    {
        int nSuperstringLength = (int)strlen(szString);
        int nSubstringLength = (int)strlen(szSubstring);

        for (int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
        {
            if (_strnicmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
            {
                return szString + nSubstringPos;
            }
        }
        return NULL;
    }

    bool CheckForStreamingDependencyLoop(const char* szFilenameDependancy) const;

    void FillClothData(CMesh& mesh);

} _ALIGN(8);


//////////////////////////////////////////////////////////////////////////
// Wrapper around CStatObj that allow rendering of static object with specified parameters.
//////////////////////////////////////////////////////////////////////////
class CStatObjWrapper
    : public CStatObj
{
    virtual void Render(const SRendParams& rParams, const SRenderingPassInfo& passInfo);
private:
    // Reference Static Object this wrapper was created for.
    CStatObj* m_pReference;
};

//////////////////////////////////////////////////////////////////////////
inline void InitializeSubObject(IStatObj::SSubObject& so)
{
    so.localTM.SetIdentity();
    so.name = "";
    so.properties = "";
    so.nType = STATIC_SUB_OBJECT_MESH;
    so.pWeights = 0;
    so.nParent = -1;
    so.tm.SetIdentity();
    so.bIdentityMatrix = true;
    so.bHidden = false;
    so.helperSize = Vec3(0, 0, 0);
    so.pStatObj = 0;
    so.bShadowProxy = 0;
}

#endif // CRYINCLUDE_CRY3DENGINE_STATOBJ_H
