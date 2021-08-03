/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_IENTITYRENDERSTATE_H
#define CRYINCLUDE_CRYCOMMON_IENTITYRENDERSTATE_H
#pragma once

#include <IRenderer.h>
#include <limits>
#include <AzCore/Component/EntityId.h>

namespace AZ 
{
    class Vector2;
}

struct IMaterial;
struct IVisArea;
struct SRenderingPassInfo;
struct SRendItemSorter;
struct SFrameLodInfo;
struct pe_params_area;
struct pe_articgeomparams;

// @NOTE: When removing an item from this enum, replace it with a dummy - ID's from this enum are stored in data and should not change.
enum EERType
{
    eERType_NotRenderNode,
    eERType_Dummy_10,
    eERType_Dummy8, 
    eERType_Light,
    eERType_Cloud,
    eERType_TerrainSystem, // used to be eERType_Dummy_1 which used to be eERType_VoxelObject, preserve order for compatibility
    eERType_FogVolume,
    eERType_Decal,
    eERType_Dummy_6, // used to be eERType_ParticleEmitter, preserve order for compatibility
    eERType_WaterVolume,
    eERType_Dummy_5, // used to be eERType_WaterWave, preserve order for compatibility
    eERType_Dummy_7, // used to be eERType_Road, preserve order for compatibility
    eERType_DistanceCloud,
    eERType_VolumeObject,
    eERType_Dummy_0, // used to be eERType_AutoCubeMap, preserve order for compatibility
    eERType_Rope,
    eERType_PrismObject,
    eERType_Dummy_2, // used to be eERType_IsoMesh, preserve order for compatibility
    eERType_Dummy_4,
    eERType_RenderComponent,
    eERType_GameEffect,
    eERType_BreakableGlass,
    eERType_Dummy_3, // used to be eERType_LightShape, preserve order for compatibility
    eERType_Dummy_9,
    eERType_GeomCache,
    eERType_StaticMeshRenderComponent,
    eERType_DynamicMeshRenderComponent,
    eERType_SkinnedMeshRenderComponent,
    eERType_TypesNum, // MUST BE AT END TOTAL NUMBER OF ERTYPES
};

enum ERNListType
{
    eRNListType_Unknown,
    eRNListType_DecalsAndRoads,
    eRNListType_ListsNum,
    eRNListType_First = eRNListType_Unknown, // This should be the last member
    // And it counts on eRNListType_Unknown
    // being the first enum element.
};

enum EOcclusionObjectType
{
    eoot_OCCLUDER,
    eoot_OCEAN,
    eoot_OCCELL,
    eoot_OCCELL_OCCLUDER,
    eoot_OBJECT,
    eoot_OBJECT_TO_LIGHT,
    eoot_TERRAIN_NODE,
    eoot_PORTAL,
};

// RenderNode flags

#define ERF_GOOD_OCCLUDER               BIT(0)
#define ERF_PROCEDURAL                  BIT(1)
#define ERF_CLONE_SOURCE                BIT(2) // set if this object was cloned from another one
#define ERF_CASTSHADOWMAPS              BIT(3) // if you ever set this flag, be sure also to set ERF_HAS_CASTSHADOWMAPS
#define ERF_RENDER_ALWAYS               BIT(4)
#define ERF_DYNAMIC_DISTANCESHADOWS     BIT(5)
#define ERF_HIDABLE                     BIT(6)
#define ERF_HIDABLE_SECONDARY           BIT(7)
#define ERF_HIDDEN                      BIT(8)
#define ERF_SELECTED                    BIT(9)
#define ERF_PROCEDURAL_ENTITY           BIT(10) // this is an object generated at runtime which has a limited lifetime (matches procedural entity)
#define ERF_OUTDOORONLY                 BIT(11)
#define ERF_NODYNWATER                  BIT(12)
#define ERF_EXCLUDE_FROM_TRIANGULATION  BIT(13)
#define ERF_REGISTER_BY_BBOX            BIT(14)
#define ERF_STATIC_INSTANCING           BIT(15)
#define ERF_VOXELIZE_STATIC             BIT(16)
#define ERF_NO_PHYSICS                  BIT(17)
#define ERF_NO_DECALNODE_DECALS         BIT(18)
#define ERF_REGISTER_BY_POSITION        BIT(19)
#define ERF_COMPONENT_ENTITY            BIT(20)
#define ERF_RECVWIND                    BIT(21)
#define ERF_COLLISION_PROXY             BIT(22) // Collision proxy is a special object that is only visible in editor
// and used for physical collisions with player and vehicles.
#define ERF_LOD_BBOX_BASED              BIT(23) // Lod changes based on bounding boxes.
#define ERF_SPEC_BIT0                   BIT(24) // Bit0 of min config specification.
#define ERF_SPEC_BIT1                   BIT(25) // Bit1 of min config specification.
#define ERF_SPEC_BIT2                   BIT(26) // Bit2 of min config specification.
#define ERF_SPEC_BITS_MASK              (ERF_SPEC_BIT0 | ERF_SPEC_BIT1 | ERF_SPEC_BIT2) // Bit mask of the min spec bits.
#define ERF_SPEC_BITS_SHIFT             (24)    // Bit offset of the ERF_SPEC_BIT0.
#define ERF_RAYCAST_PROXY               BIT(27) // raycast proxy is only used for raycasting
#define ERF_HUD                         BIT(28) // Hud object that can avoid some visibility tests
#define ERF_RAIN_OCCLUDER               BIT(29) // Is used for rain occlusion map
#define ERF_HAS_CASTSHADOWMAPS          BIT(30) // at one point had ERF_CASTSHADOWMAPS set
#define ERF_ACTIVE_LAYER                BIT(31) // the node is on a currently active layer

struct IShadowCaster
{
    // <interfuscator:shuffle>
    virtual ~IShadowCaster(){}
    virtual bool HasOcclusionmap([[maybe_unused]] int nLod, [[maybe_unused]] IRenderNode* pLightOwner) { return false; }
    virtual CLodValue ComputeLod(int wantedLod, [[maybe_unused]] const SRenderingPassInfo& passInfo) { return CLodValue(wantedLod); }
    virtual void Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo) = 0;
    virtual const AABB GetBBoxVirtual() = 0;
    virtual void FillBBox(AABB& aabb) = 0;
    virtual EERType GetRenderNodeType() = 0;
    virtual bool IsRenderNode() { return true; }
    // </interfuscator:shuffle>
    uint8 m_cStaticShadowLod;
};

// Optional filter function for octree queries to perform custom filtering of the results.
// return true to keep the render node, false to filter it out.
using ObjectTreeQueryFilterCallback = AZStd::function<bool(IRenderNode*, EERType)>;

struct IOctreeNode
{
public:
    virtual ~IOctreeNode() {};

    virtual void GetObjectsByType(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox, ObjectTreeQueryFilterCallback filterCallback = nullptr) = 0;

    struct CVisArea* m_pVisArea;
};

struct SLodDistDissolveTransitionState
{
    float fStartDist;
    int8 nOldLod;
    int8 nNewLod;
    bool bFarside;
};

struct SLightInfo
{
    bool operator == (const SLightInfo& other) const
    { return other.vPos.IsEquivalent(vPos, 0.1f) && fabs(other.fRadius - fRadius) < 0.1f; }
    Vec3 vPos;
    float fRadius;
    bool bAffecting;
};

struct IRenderNode
    : public IShadowCaster
{
    enum EInternalFlags
    {
        DECAL_OWNER     = BIT(0),           // Owns some decals.
        REQUIRES_NEAREST_CUBEMAP = BIT(1), // Pick nearest cube map
        UPDATE_DECALS   = BIT(2),           // The node changed geometry - decals must be updated.
        REQUIRES_FORWARD_RENDERING = BIT(3), // Special shadow processing needed.
        WAS_INVISIBLE   = BIT(4),           // Was invisible last frame.
        WAS_IN_VISAREA  = BIT(5),           // Was inside vis-ares last frame.
        WAS_FARAWAY         = BIT(6),         // Was considered 'far away' for the purposes of physics deactivation.
        HAS_OCCLUSION_PROXY = BIT(7)    // This node has occlusion proxy.
    };

    IRenderNode()
    {
        m_dwRndFlags = 0;
        m_fViewDistanceMultiplier = static_cast<float>(IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX); // By default object is not limited by distance. 
        m_ucLodRatio = 100;
        m_pOcNode = 0;
        m_fWSMaxViewDist = 0;
        m_nInternalFlags = 0;
        m_nMaterialLayers = 0;
        m_pRNTmpData = NULL;
        m_pPrev = m_pNext = NULL;
        m_nSID = 0;
        m_cShadowLodBias = 0;
        m_cStaticShadowLod = 0;
    }

    virtual bool CanExecuteRenderAsJob() { return false; }

    // <interfuscator:shuffle>
    // Debug info about object.
    virtual const char* GetName() const = 0;
    virtual const char* GetEntityClassName() const = 0;
    virtual string GetDebugString([[maybe_unused]] char type = 0) const { return ""; }
    virtual float GetImportance() const   { return 1.f; }

    // Description:
    //    Releases IRenderNode.
    virtual void ReleaseNode([[maybe_unused]] bool bImmediate = false) { delete this; }


    virtual IRenderNode* Clone() const { return NULL; }

    // Description:
    //    Sets render node transformation matrix.
    virtual void SetMatrix([[maybe_unused]] const Matrix34& mat) {}

    // Description:
    //    Gets local bounds of the render node.
    virtual void GetLocalBounds(AABB& bbox) { AABB WSBBox(GetBBox()); bbox = AABB(WSBBox.min - GetPos(true), WSBBox.max - GetPos(true)); }

    virtual Vec3 GetPos(bool bWorldOnly = true) const = 0;
    virtual const AABB GetBBox() const = 0;
    virtual void FillBBox(AABB& aabb) { aabb = GetBBox(); }
    virtual void SetBBox(const AABB& WSBBox) = 0;

    virtual void SetScale([[maybe_unused]] const Vec3& scale) {}
    
    //Get the scales assuming the scale is uniform or per column as needed.
    virtual float GetUniformScale() { return 1.0f; }
    virtual float GetColumnScale([[maybe_unused]] int column) { return 1.0f; }

    // Summary:
    //    Changes the world coordinates position of this node by delta
    //    Don't forget to call this base function when overriding it.
    virtual void OffsetPosition(const Vec3& delta) = 0;

    // Return true when the node is initialized and ready to render
    virtual bool IsReady() const { return true; }

    // Summary:
    //   Renders node geometry
    virtual void Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo) = 0;

    // Hides/disables node in renderer.
    virtual void Hide(bool bHide) { SetRndFlags(ERF_HIDDEN, bHide); }
    bool IsHidden() const { return (GetRndFlags() & ERF_HIDDEN) != 0; }

    // Gives access to object components.
    virtual struct IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);
    virtual _smart_ptr<IMaterial> GetEntitySlotMaterial([[maybe_unused]] unsigned int nPartId, [[maybe_unused]] bool bReturnOnlyVisible = false, [[maybe_unused]] bool* pbDrawNear = NULL) { return NULL; }
    virtual void SetEntityStatObj([[maybe_unused]] unsigned int nSlot, [[maybe_unused]] IStatObj* pStatObj, [[maybe_unused]] const Matrix34A* pMatrix = NULL) {};
    virtual int GetSlotCount() const { return 1; }

    // Summary:
    //   Returns IRenderMesh of the object.
    virtual struct IRenderMesh* GetRenderMesh([[maybe_unused]] int nLod) { return 0; };

    // Description:
    //   Allows to adjust default lod distance settings,
    //   if fLodRatio is 100 - default lod distance is used.
    virtual void SetLodRatio(int nLodRatio) { m_ucLodRatio = static_cast<unsigned char>(min(255, max(0, nLodRatio))); }

    // Summary:
    //   Gets material layers mask.
    virtual uint8 GetMaterialLayers() const { return m_nMaterialLayers; }


    // Summary
    //   Physicalizes if it isn't already.
    virtual void CheckPhysicalized() {};

    // Summary:
    //   Physicalizes node.
    virtual void Physicalize([[maybe_unused]] bool bInstant = false) {}

    virtual ~IRenderNode() { assert(!m_pRNTmpData); };

    // Summary:
    //   Sets override material for this instance.
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat) = 0;
    // Summary:
    //   Queries override material of this instance.
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL) = 0;
    virtual _smart_ptr<IMaterial> GetMaterialOverride() = 0;
    virtual void GetMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials)
    {
        _smart_ptr<IMaterial> currentMaterial = GetMaterialOverride();
        if (!currentMaterial)
        {
            currentMaterial = GetMaterial();
        }
        if (currentMaterial)
        {
            materials.push_back(currentMaterial);
        }
    }

    // Used by the editor during export
    virtual void SetCollisionClassIndex([[maybe_unused]] int tableIndex) {}

    virtual int GetEditorObjectId() { return 0; }
    virtual void SetEditorObjectId([[maybe_unused]] int nEditorObjectId) {}
    virtual void SetStatObjGroupIndex([[maybe_unused]] int nVegetationGroupIndex) { }
    virtual int GetStatObjGroupId() const { return -1; }
    virtual void SetLayerId([[maybe_unused]] uint16 nLayerId) { }
    virtual uint16 GetLayerId() { return 0; }
    virtual float GetMaxViewDist() = 0;

    virtual EERType GetRenderNodeType() = 0;
    virtual bool IsAllocatedOutsideOf3DEngineDLL() 
    { 
        return GetRenderNodeType() == eERType_RenderComponent || 
            GetRenderNodeType() == eERType_StaticMeshRenderComponent || 
            GetRenderNodeType() == eERType_DynamicMeshRenderComponent || 
            GetRenderNodeType() == eERType_SkinnedMeshRenderComponent;
    }
    virtual void Dephysicalize([[maybe_unused]] bool bKeepIfReferenced = false) {}
    virtual void Dematerialize() {}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual void Precache() {};

    virtual const AABB GetBBoxVirtual() { return GetBBox(); }

    //  virtual float GetLodForDistance(float fDistance) { return 0; }

    virtual void OnRenderNodeBecomeVisible([[maybe_unused]] const SRenderingPassInfo& passInfo) {}
    virtual void OnPhysAreaChange() {}

    virtual bool IsMovableByGame() const { return false; }

    virtual uint8 GetSortPriority() { return 0; }

    // Types of voxelization for objects and lights
    enum EVoxelGIMode : int
    {
        VM_None = 0, // No voxelization
        VM_Static, // Incremental or asynchronous lazy voxelization
        VM_Dynamic, // Real-time every-frame voxelization on GPU
    };

    virtual EVoxelGIMode GetVoxelGIMode() { return VM_None; }
    virtual void SetDesiredVoxelGIMode([[maybe_unused]] EVoxelGIMode voxelMode) {}

    virtual void SetMinSpec(int nMinSpec) { m_dwRndFlags &= ~ERF_SPEC_BITS_MASK; m_dwRndFlags |= (nMinSpec << ERF_SPEC_BITS_SHIFT) & ERF_SPEC_BITS_MASK; };

    // Description:
    //  Allows to adjust default max view distance settings,
    //  if fMaxViewDistRatio is 1.0f - default max view distance is used.
    virtual void SetViewDistanceMultiplier(float fViewDistanceMultiplier);
    // </interfuscator:shuffle>

    void CopyIRenderNodeData(IRenderNode* pDest) const
    {
        pDest->m_fWSMaxViewDist         = m_fWSMaxViewDist;
        pDest->m_dwRndFlags                 = m_dwRndFlags;
        //pDest->m_pOcNode                      = m_pOcNode;        // Removed to stop the registering from earlying out.
        pDest->m_fViewDistanceMultiplier = m_fViewDistanceMultiplier;
        pDest->m_ucLodRatio                 = m_ucLodRatio;
        pDest->m_cShadowLodBias         = m_cShadowLodBias;
        pDest->m_cStaticShadowLod       = m_cStaticShadowLod;
        pDest->m_nInternalFlags         = m_nInternalFlags;
        pDest->m_nMaterialLayers        = m_nMaterialLayers;
        //pDestBrush->m_pRNTmpData              //If this is copied from the source render node, there are two
        //  pointers to the same data, and if either is deleted, there will
        //  be a crash when the dangling pointer is used on the other
    }

    // Rendering flags.
    ILINE void SetRndFlags(unsigned int dwFlags) { m_dwRndFlags = dwFlags; }
    ILINE void SetRndFlags(unsigned int dwFlags, bool bEnable)
    {
        if (bEnable)
        {
            SetRndFlags(m_dwRndFlags | dwFlags);
        }
        else
        {
            SetRndFlags(m_dwRndFlags & (~dwFlags));
        }
    }
    ILINE unsigned int GetRndFlags() const { return m_dwRndFlags; }

    // Object draw frames (set if was drawn).
    ILINE void SetDrawFrame(int nFrameID, int nRecursionLevel)
    {
        assert(m_pRNTmpData);
        int* pDrawFrames = (int*)m_pRNTmpData;
        pDrawFrames[nRecursionLevel] = nFrameID;
    }

    ILINE int GetDrawFrame(int nRecursionLevel = 0) const
    {
        IF (!m_pRNTmpData, 0)
        {
            return 0;
        }

        int* pDrawFrames = (int*)m_pRNTmpData;
        return pDrawFrames[nRecursionLevel];
    }

    // Returns:
    //   Current VisArea or null if in outdoors or entity was not registered in 3dengine.
    ILINE IVisArea* GetEntityVisArea() const { return m_pOcNode ? (IVisArea*)(m_pOcNode->m_pVisArea) : NULL; }

    // Summary:
    //   Makes object visible at any distance.
    ILINE void SetViewDistUnlimited() { SetViewDistanceMultiplier(100.0f); }

    // Summary:
    //   Retrieves the view distance settings.
    ILINE float GetViewDistanceMultiplier() const
    {
        return m_fViewDistanceMultiplier;
    }

    // Summary:
    //   Returns lod distance ratio.
    ILINE int GetLodRatio() const { return m_ucLodRatio; }

    // Summary:
    //   Returns lod distance ratio
    ILINE float GetLodRatioNormalized() const { return 0.01f * m_ucLodRatio; }

    virtual bool GetLodDistances([[maybe_unused]] const SFrameLodInfo& frameLodInfo, [[maybe_unused]] float* distances) const { return false; }

    // Returns distance for first lod change, not factoring in distance multiplier or lod ratio
    virtual float GetFirstLodDistance() const { return FLT_MAX; }

    // Description:
    //  Bias value to add to the regular lod
    virtual void SetShadowLodBias(int8 nShadowLodBias) { m_cShadowLodBias = nShadowLodBias; }

    // Summary:
    //   Returns lod distance ratio.
    ILINE int GetShadowLodBias() const { return m_cShadowLodBias; }

    // Summary:
    //   Sets material layers mask.
    ILINE void SetMaterialLayers(uint8 nMtlLayers) { m_nMaterialLayers = nMtlLayers; }

    ILINE int  GetMinSpec() const { return (m_dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT; };

    static const ERNListType GetRenderNodeListId(const EERType eRType)
    {
        switch (eRType)
        {
        case eERType_Decal:
            return eRNListType_DecalsAndRoads;
        default:
            return eRNListType_Unknown;
        }
    }

    virtual AZ::EntityId GetEntityId() { return AZ::EntityId(); }

    //////////////////////////////////////////////////////////////////////////
    // Variables
    //////////////////////////////////////////////////////////////////////////

public:

    // Every sector has linked list of IRenderNode objects.
    IRenderNode* m_pNext, * m_pPrev;

    // Current objects tree cell.
    IOctreeNode* m_pOcNode;

    // Pointer to temporary data allocated only for currently visible objects.
    struct CRNTmpData* m_pRNTmpData;

    // Max view distance.
    float m_fWSMaxViewDist;

    // Render flags.
    int m_dwRndFlags;

    // Shadow LOD bias
    // Set to SHADOW_LODBIAS_DISABLE to disable any shadow lod overrides for this rendernode
    static const int8 SHADOW_LODBIAS_DISABLE = -128;
    int8 m_cShadowLodBias;

    // Segment Id
    int m_nSID;

    // Max view distance settings.
    static const int VIEW_DISTANCE_MULTIPLIER_MAX = 100;
    float m_fViewDistanceMultiplier;

    // LOD settings.
    unsigned char m_ucLodRatio;

    // Flags for render node internal usage, one or more bits from EInternalFlags.
    unsigned char m_nInternalFlags;

    // Material layers bitmask -> which material layers are active.
    unsigned char m_nMaterialLayers;
};

inline void IRenderNode::SetViewDistanceMultiplier(float fViewDistanceMultiplier)
{
    fViewDistanceMultiplier = CLAMP(fViewDistanceMultiplier, 0.0f, 100.0f);
    if (fabs(m_fViewDistanceMultiplier - fViewDistanceMultiplier) > FLT_EPSILON)
    {
        m_fViewDistanceMultiplier = fViewDistanceMultiplier;
    }
}


///////////////////////////////////////////////////////////////////////////////
inline IStatObj* IRenderNode::GetEntityStatObj([[maybe_unused]] unsigned int nPartId, [[maybe_unused]] unsigned int nSubPartId, [[maybe_unused]] Matrix34A* pMatrix, [[maybe_unused]] bool bReturnOnlyVisible)
{
    return 0;
}


//We must use interfaces instead of unsafe type casts and unnecessary includes

struct ILightSource
    : public IRenderNode
{
    // <interfuscator:shuffle>
    virtual void SetLightProperties(const CDLight& light) = 0;
    virtual CDLight& GetLightProperties() = 0;
    virtual const Matrix34& GetMatrix() = 0;
    virtual struct ShadowMapFrustum* GetShadowFrustum(int nId = 0) = 0;
    virtual bool IsLightAreasVisible() = 0;
    virtual void SetCastingException(IRenderNode* pNotCaster) = 0;
    virtual void SetName(const char* name) = 0;
    // </interfuscator:shuffle>
};

struct SCloudMovementProperties
{
    bool m_autoMove;
    Vec3 m_speed;
    Vec3 m_spaceLoopBox;
    float m_fadeDistance;
};

// Summary:
//   ICloudRenderNode is an interface to the Cloud Render Node object.
struct ICloudRenderNode
    : public IRenderNode
{
    // <interfuscator:shuffle>
    // Description:
    //    Loads a cloud from a cloud description XML file.
    virtual bool LoadCloud(const char* sCloudFilename) = 0;
    virtual bool LoadCloudFromXml(XmlNodeRef cloudNode) = 0;
    virtual void SetMovementProperties(const SCloudMovementProperties& properties) = 0;
    // </interfuscator:shuffle>
};

// Summary:
//   IVoxelObject is an interface to the Voxel Object Render Node object.
struct IVoxelObject
    : public IRenderNode
{
    // <interfuscator:shuffle>
    virtual void SetCompiledData(void* pData, int nSize, uint8 ucChildId, EEndian eEndian) = 0;
    virtual void SetObjectName(const char* pName) = 0;
    virtual void SetMatrix(const Matrix34& mat) = 0;
    virtual bool ResetTransformation() = 0;
    virtual void InterpolateVoxelData() = 0;
    virtual void SetFlags(int nFlags) = 0;
    virtual void Regenerate() = 0;
    virtual void CopyHM() = 0;
    virtual bool IsEmpty() = 0;
    // </interfuscator:shuffle>
};

// Summary:
//   IFogVolumeRenderNode is an interface to the Fog Volume Render Node object.
struct SFogVolumeProperties
{
    // Common parameters.
    // Center position & rotation values are taken from the entity matrix.

    int           m_volumeType;
    Vec3      m_size;
    ColorF        m_color;
    bool      m_useGlobalFogColor;
    bool        m_ignoresVisAreas;
    bool        m_affectsThisAreaOnly;
    float     m_globalDensity;
    float     m_densityOffset;
    float     m_softEdges;
    float     m_fHDRDynamic;            // 0 to get the same results in LDR, <0 to get darker, >0 to get brighter.
    float     m_nearCutoff;

    float m_heightFallOffDirLong;       // Height based fog specifics.
    float m_heightFallOffDirLati;       // Height based fog specifics.
    float m_heightFallOffShift;             // Height based fog specifics.
    float m_heightFallOffScale;             // Height based fog specifics.

    float m_rampStart;
    float m_rampEnd;
    float m_rampInfluence;
    float m_windInfluence;
    float m_densityNoiseScale;
    float m_densityNoiseOffset;
    float m_densityNoiseTimeFrequency;
    Vec3 m_densityNoiseFrequency;
};

struct IFogVolumeRenderNode
    : public IRenderNode
{
    // <interfuscator:shuffle>
    virtual void SetFogVolumeProperties(const SFogVolumeProperties& properties) = 0;
    virtual const Matrix34& GetMatrix() const = 0;

    virtual void FadeGlobalDensity(float fadeTime, float newGlobalDensity) = 0;
    // </interfuscator:shuffle>
};

// LY renderer system spec levels.
enum class EngineSpec : AZ::u32
{
    Low = 1,
    Medium,
    High,
    VeryHigh,
    Never = UINT_MAX,
};

struct SDecalProperties
{
    SDecalProperties()
    {
        m_projectionType = ePlanar;
        m_sortPrio = 0;
        m_deferred = false;
        m_pos = Vec3(0.0f, 0.0f, 0.0f);
        m_normal = Vec3(0.0f, 0.0f, 1.0f);
        m_explicitRightUpFront = Matrix33::CreateIdentity();
        m_radius = 1.0f;
        m_depth = 1.0f;
        m_opacity = 1.0f;
        m_angleAttenuation = 1.0f;
        m_maxViewDist = 8000.0f;
        m_minSpec = EngineSpec::Low;
    }

    enum EProjectionType : int
    {
        ePlanar,
        eProjectOnTerrain,
        eProjectOnTerrainAndStaticObjects
    };

    EProjectionType m_projectionType;
    uint8 m_sortPrio;
    uint8 m_deferred;
    Vec3 m_pos;
    Vec3 m_normal;
    Matrix33 m_explicitRightUpFront;
    float m_radius;
    float m_depth;
    const char* m_pMaterialName;
    float m_opacity;
    float m_angleAttenuation;
    float m_maxViewDist;
    EngineSpec m_minSpec;
};

// Description:
//   IDecalRenderNode is an interface to the Decal Render Node object.
struct IDecalRenderNode
    : public IRenderNode
{
    // <interfuscator:shuffle>
    virtual void SetDecalProperties(const SDecalProperties& properties) = 0;
    virtual const SDecalProperties* GetDecalProperties() const = 0;
    virtual const Matrix34& GetMatrix() = 0;
    virtual void CleanUpOldDecals() = 0;
    // </interfuscator:shuffle>
};

// Description:
//   IWaterVolumeRenderNode is an interface to the Water Volume Render Node object.
struct IWaterVolumeRenderNode
    : public IRenderNode
{
    enum EWaterVolumeType
    {
        eWVT_Unknown,
        eWVT_Ocean,
        eWVT_Area,
        eWVT_River
    };

    // <interfuscator:shuffle>
    // Description:
    // Sets if the render node is attached to a parent entity
    // This must be called right after the object construction if it is the case
    // Only supported for Areas (not rivers or ocean)
    virtual void SetAreaAttachedToEntity() = 0;

    virtual void SetFogDensity(float fogDensity) = 0;
    virtual float GetFogDensity() const = 0;
    virtual void SetFogColor(const Vec3& fogColor) = 0;
    virtual void SetFogColorAffectedBySun(bool enable) = 0;
    virtual void SetFogShadowing(float fogShadowing) = 0;

    virtual void SetCapFogAtVolumeDepth(bool capFog) = 0;
    virtual void SetVolumeDepth(float volumeDepth) = 0;
    virtual void SetStreamSpeed(float streamSpeed) = 0;

    virtual void SetCaustics(bool caustics) = 0;
    virtual void SetCausticIntensity(float causticIntensity) = 0;
    virtual void SetCausticTiling(float causticTiling) = 0;
    virtual void SetCausticHeight(float causticHeight) = 0;
    virtual void SetAuxPhysParams(pe_params_area*) = 0;

    virtual void CreateOcean(uint64 volumeID, /* TBD */ bool keepSerializationParams = false) = 0;
    virtual void CreateArea(uint64 volumeID, const Vec3* pVertices, unsigned int numVertices, const Vec2& surfUVScale, const Plane_tpl<f32>& fogPlane, bool keepSerializationParams = false, int nSID = -1) = 0;
    virtual void CreateRiver(uint64 volumeID, const Vec3* pVertices, unsigned int numVertices, float uTexCoordBegin, float uTexCoordEnd, const Vec2& surfUVScale, const Plane_tpl<f32>& fogPlane, bool keepSerializationParams = false, int nSID = -1) = 0;
    virtual void CreateRiver(uint64 volumeID, const AZStd::vector<AZ::Vector3>& verticies, const AZ::Transform& transform, float uTexCoordBegin, float uTexCoordEnd, const AZ::Vector2& surfUVScale, const AZ::Plane& fogPlane, bool keepSerializationParams = false, int nSID = -1) = 0;

    virtual void SetAreaPhysicsArea(const Vec3* pVertices, unsigned int numVertices, bool keepSerializationParams = false) = 0;
    virtual void SetRiverPhysicsArea(const Vec3* pVertices, unsigned int numVertices, bool keepSerializationParams = false) = 0;
    virtual void SetRiverPhysicsArea(const AZStd::vector<AZ::Vector3>& verticies, const AZ::Transform& transform, bool keepSerializationParams = false) = 0;

    // </interfuscator:shuffle>

    // This flag is used to account for legacy entities which used to serialize the node without parent objects.
    // Now there are runtime components which spawn the rendering node, however we need to support legacy code as well. 
    // Remove this flag when legacy entities are removed entirely
    bool m_hasToBeSerialised = true;
};

// Description:
//   IDistanceCloudRenderNode is an interface to the Distance Cloud Render Node object.
struct SDistanceCloudProperties
{
    Vec3 m_pos;
    float m_sizeX;
    float m_sizeY;
    float m_rotationZ;
    const char* m_pMaterialName;
};

struct IDistanceCloudRenderNode
    : public IRenderNode
{
    virtual void SetProperties(const SDistanceCloudProperties& properties) = 0;
};

struct SVolumeObjectProperties
{
};

struct SVolumeObjectMovementProperties
{
    bool m_autoMove;
    Vec3 m_speed;
    Vec3 m_spaceLoopBox;
    float m_fadeDistance;
};

// Description:
//   IVolumeObjectRenderNode is an interface to the Volume Object Render Node object.
struct IVolumeObjectRenderNode
    : public IRenderNode
{
    // <interfuscator:shuffle>
    virtual void LoadVolumeData(const char* filePath) = 0;
    virtual void SetProperties(const SVolumeObjectProperties& properties) = 0;
    virtual void SetMovementProperties(const SVolumeObjectMovementProperties& properties) = 0;
    // </interfuscator:shuffle>
};

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
struct IPrismRenderNode
    : public IRenderNode
{
};
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

#endif // CRYINCLUDE_CRYCOMMON_IENTITYRENDERSTATE_H
