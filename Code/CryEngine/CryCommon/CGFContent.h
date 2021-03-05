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

// Description : Describe contents on CGF file.


#ifndef CRYINCLUDE_CRYCOMMON_CGFCONTENT_H
#define CRYINCLUDE_CRYCOMMON_CGFCONTENT_H
#pragma once

#include <IIndexedMesh.h> // <> required for Interfuscator
#include <IChunkFile.h> // <> required for Interfuscator
#include <CryHeaders.h>
#include <Cry_Color.h>
#include <CryArray.h>
#include <StringUtils.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/std/containers/unordered_map.h> //Required for LOD support for touch bending vegetation
#include <AzCore/std/string/string.h> //Required for LOD support for touch bending vegetation

const int CGF_NODE_NAME_LENGTH = 64;
//END: Add LOD support for touch bending vegetation

struct CMaterialCGF;
struct IConvertContext;

#define CGF_NODE_NAME_LOD_PREFIX "$lod"

//////////////////////////////////////////////////////////////////////////
// This structure represents CGF node.
//////////////////////////////////////////////////////////////////////////
struct CNodeCGF
    : public _cfg_reference_target<CNodeCGF>
{
    enum ENodeType
    {
        NODE_MESH,
        NODE_LIGHT,
        NODE_HELPER,
    };
    enum EPhysicalizeFlags
    {
        ePhysicsalizeFlag_MeshNotNeeded = BIT(2), // When set physics data doesn't need additional Mesh indices or vertices.
        ePhysicsalizeFlag_NoBreaking = BIT(3), // node is unsuitable for procedural 3d breaking
    };

    ENodeType type;
    //START: Add LOD support for touch bending vegetation
    char      name[CGF_NODE_NAME_LENGTH];
    //END: Add LOD support for touch bending vegetation
    string    properties;
    Matrix34  localTM;      // Local space transformation matrix.
    Matrix34  worldTM;      // World space transformation matrix.
    CNodeCGF* pParent;      // Pointer to parent node.
    CNodeCGF* pSharedMesh;  // Not NULL if this node is sharing mesh and physics from referenced Node.
    CMesh*    pMesh;        // Pointer to mesh loaded for this node. (Only when type == NODE_MESH)

    HelperTypes helperType;  // Only relevant if type==NODE_HELPER
    Vec3        helperSize;  // Only relevant if type==NODE_HELPER

    CMaterialCGF* pMaterial; // Material node.

    // Physical data of the node with mesh.
    int nPhysicalizeFlags;              // Saved into the nFlags2 chunk member.
    AZStd::vector<char> physicalGeomData[4];
    int nPhysTriCount;                  // Not saved! only used for statistics in RC

    //////////////////////////////////////////////////////////////////////////
    // Used internally.
    int nChunkId;       // Chunk id as loaded from CGF.
    int nParentChunkId; // Chunk id of parent Node.
    int nObjectChunkId; // Chunk id of the corresponding mesh.
    int pos_cont_id;    // position controller chunk id
    int rot_cont_id;    // rotation controller chunk id
    int scl_cont_id;    // scale controller chunk id
    //////////////////////////////////////////////////////////////////////////

    // True if worldTM is identity.
    bool bIdentityMatrix;
    // True when this node is invisible physics proxy.
    bool bPhysicsProxy;

    // These values are not saved, but are only used for loading empty mesh chunks.
    struct MeshInfo
    {
        int nVerts;
        int nIndices;
        int nSubsets;
        Vec3 bboxMin;
        Vec3 bboxMax;
        float fGeometricMean;
    };
    MeshInfo meshInfo;

    CrySkinVtx* pSkinInfo; // for skinning with skeleton meshes (deformable objects)

    //////////////////////////////////////////////////////////////////////////
    // Constructor.
    //////////////////////////////////////////////////////////////////////////
    void Init()
    {
        type = NODE_MESH;
        localTM.SetIdentity();
        worldTM.SetIdentity();
        pParent = 0;
        pSharedMesh = 0;
        pMesh = 0;
        pMaterial = 0;
        helperType = HP_POINT;
        helperSize.Set(0, 0, 0);
        nPhysicalizeFlags = 0;
        nChunkId = 0;
        nParentChunkId = 0;
        nObjectChunkId = 0;
        pos_cont_id = rot_cont_id = scl_cont_id = 0;
        bIdentityMatrix = true;
        bPhysicsProxy = false;
        pSkinInfo = 0;
        nPhysTriCount = 0;

        ZeroStruct(meshInfo);
    }

    CNodeCGF()
    {
        Init();
    }

    explicit CNodeCGF(_cfg_reference_target<CNodeCGF>::DeleteFncPtr pDeleteFnc)
        : _cfg_reference_target<CNodeCGF>(pDeleteFnc)
    {
        Init();
    }

    ~CNodeCGF()
    {
        if (!pSharedMesh)
        {
            delete pMesh;
        }
        if (pSkinInfo)
        {
            delete[] pSkinInfo;
        }
    }
};


//////////////////////////////////////////////////////////////////////////
// structures for skinning
//////////////////////////////////////////////////////////////////////////

struct TFace
{
    uint16 i0, i1, i2;
    TFace() {}
    TFace(uint16 v0, uint16 v1, uint16 v2) { i0 = v0; i1 = v1; i2 = v2; }
    TFace(const CryFace& face)  { i0 = aznumeric_caster(face[0]); i1 = aznumeric_caster(face[1]); i2 = aznumeric_caster(face[2]); }
    void operator = (const TFace& f) {  i0 = f.i0; i1 = f.i1;   i2 = f.i2;    }
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
    AUTO_STRUCT_INFO
};

struct PhysicalProxy
{
    uint32 ChunkID;
    DynArray<Vec3> m_arrPoints;
    DynArray<uint16> m_arrIndices;
    DynArray<char> m_arrMaterials;
};

struct MorphTargets
{
    uint32 MeshID;
    string m_strName;
    DynArray<SMeshMorphTargetVertex> m_arrIntMorph;
    DynArray<SMeshMorphTargetVertex> m_arrExtMorph;
};

typedef MorphTargets* MorphTargetsPtr;

struct IntSkinVertex
{
    Vec3 __obsolete0;           // thin/fat vertex position. must be removed in the next RC refactoring
    Vec3 pos;                   // vertex-position of model.2
    Vec3 __obsolete2;           // thin/fat vertex position. must be removed in the next RC refactoring
    uint16 boneIDs[4];
    f32 weights[4];
    ColorB color;   //index for blend-array
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
    AUTO_STRUCT_INFO
};



//////////////////////////////////////////////////////////////////////////
// TCB Controller implementation.
//////////////////////////////////////////////////////////////////////////


// retrieves the position and orientation (in the logarithmic space, i.e. instead of quaternion, its logarithm is returned)
// may be optimal for motion interpolation
struct PQLog
{
    Vec3 vPos;
    Vec3 vRotLog; // logarithm of the rotation
    void blendPQ (const PQLog& pqFrom, const PQLog& pqTo, f32 fBlend);
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

struct CControllerType
{
    uint16 m_controllertype;
    uint16 m_index;
    CControllerType()
    {
        m_controllertype = 0xffff;
        m_index = 0xffff;
    }
};

struct TCBFlags
{
    uint8 f0, f1;
    TCBFlags() {    f0 = f1 = 0; }
};

struct CStoredSkinningInfo
{
    int32 m_nTicksPerFrame;
    f32 m_secsPerTick;
    int32 m_nStart;
    int32 m_nEnd;
    f32 m_Speed;
    f32 m_Distance;
    f32 m_Slope;
    int m_nAssetFlags;
    f32 m_LHeelStart, m_LHeelEnd;
    f32 m_LToe0Start, m_LToe0End;
    f32 m_RHeelStart, m_RHeelEnd;
    f32 m_RToe0Start, m_RToe0End;
    Vec3 m_MoveDirection; // raw storage

    CStoredSkinningInfo()
        :  m_Speed(-1.0f)
        , m_Distance(-1.0f)
        , m_nAssetFlags(0)
        , m_LHeelStart(-10000.0f)
        , m_LHeelEnd(-10000.0f)
        , m_LToe0Start(-10000.0f)
        , m_LToe0End(-10000.0f)
        , m_RHeelStart(-10000.0f)
        , m_RHeelEnd(-10000.0f)
        , m_RToe0Start(-10000.0f)
        , m_RToe0End(-10000.0f)
        , m_Slope(-1.0f)
    {
    }
    AUTO_STRUCT_INFO
};



// structure for recreating controllers
struct CControllerInfo
{
    uint32 m_nControllerID;
    uint32 m_nPosKeyTimeTrack;
    uint32 m_nPosTrack;
    uint32 m_nRotKeyTimeTrack;
    uint32 m_nRotTrack;

    CControllerInfo()
        : m_nControllerID(~0)
        , m_nPosKeyTimeTrack(~0)
        , m_nPosTrack(~0)
        , m_nRotKeyTimeTrack(~0)
        , m_nRotTrack(~0) {}

    AUTO_STRUCT_INFO
};

struct MeshCollisionInfo
{
    AABB m_aABB;
    OBB m_OBB;
    Vec3 m_Pos;
    DynArray<int16> m_arrIndexes;
    int32 m_iBoneId;

    MeshCollisionInfo()
    {
        // This didn't help much.
        // The BBs are reset to opposite infinites,
        // but never clamped/grown by any member points.
        m_aABB.min.zero();
        m_aABB.max.zero();
        m_OBB.m33.SetIdentity();
        m_OBB.h.zero();
        m_OBB.c.zero();
        m_Pos.zero();
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_arrIndexes);
    }
};



struct SJointsAimIK_Rot
{
    const char* m_strJointName;
    int16 m_nJointIdx;
    int16 m_nPosIndex;
    uint8 m_nPreEvaluate;
    uint8 m_nAdditive;
    int16 m_nRotJointParentIdx;
    SJointsAimIK_Rot()
    {
        m_strJointName       =  0;
        m_nJointIdx          = -1;
        m_nPosIndex          = -1;
        m_nPreEvaluate       =  0;
        m_nAdditive          =  0;
        m_nRotJointParentIdx = -1;
    };
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

struct SJointsAimIK_Pos
{
    const char* m_strJointName;
    int16 m_nJointIdx;
    uint8 m_nAdditive;
    uint8 m_nEmpty;
    SJointsAimIK_Pos()
    {
        m_strJointName =  0;
        m_nJointIdx    = -1;
        m_nAdditive    =  0;
        m_nEmpty       =  0;
    };
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};


struct DirectionalBlends
{
    string m_AnimToken;
    uint32 m_AnimTokenCRC32;
    const char* m_strParaJointName;
    int16 m_nParaJointIdx;
    int16 m_nRotParaJointIdx;
    const char* m_strStartJointName;
    int16 m_nStartJointIdx;
    int16 m_nRotStartJointIdx;
    const char* m_strReferenceJointName;
    int32 m_nReferenceJointIdx;
    DirectionalBlends()
    {
        m_AnimTokenCRC32        =  0;
        m_strParaJointName      =  0;
        m_nParaJointIdx         = -1;
        m_nRotParaJointIdx      = -1;
        m_strStartJointName     =  0;
        m_nStartJointIdx        = -1;
        m_nRotStartJointIdx     = -1;
        m_strReferenceJointName =  0;
        m_nReferenceJointIdx    =  1;  //by default we use the Pelvis
    };
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
};


struct CSkinningInfo
    : public _reference_target_t
{
    DynArray<CryBoneDescData> m_arrBonesDesc;       //animation-bones

    DynArray<SJointsAimIK_Rot>  m_LookIK_Rot;                       //rotational joints used for Look-IK
    DynArray<SJointsAimIK_Pos>  m_LookIK_Pos;                       //positional joints used for Look-IK
    DynArray<DirectionalBlends> m_LookDirBlends;          //positional joints used for Look-IK

    DynArray<SJointsAimIK_Rot>  m_AimIK_Rot;                        //rotational joints used for Aim-IK
    DynArray<SJointsAimIK_Pos>  m_AimIK_Pos;                        //positional joints used for Aim-IK
    DynArray<DirectionalBlends> m_AimDirBlends;         //positional joints used for Aim-IK


    DynArray<PhysicalProxy> m_arrPhyBoneMeshes; //collision proxi
    DynArray<MorphTargetsPtr> m_arrMorphTargets;
    DynArray<TFace> m_arrIntFaces;
    DynArray<IntSkinVertex> m_arrIntVertices;
    DynArray<uint16> m_arrExt2IntMap;
    DynArray<BONE_ENTITY> m_arrBoneEntities;            //physical-bones
    DynArray<MeshCollisionInfo> m_arrCollisions;

    uint32 m_numChunks{ 0 };
    bool m_bRotatedMorphTargets;
    bool m_bProperBBoxes;

    CSkinningInfo()
        : m_bRotatedMorphTargets(false)
        , m_bProperBBoxes(false) {}

    ~CSkinningInfo()
    {
        for (DynArray<MorphTargetsPtr>::iterator it = m_arrMorphTargets.begin(), end = m_arrMorphTargets.end(); it != end; ++it)
        {
            delete *it;
        }
    }

    int32 GetJointIDByName(const char* strJointName) const
    {
        uint32 numJoints = m_arrBonesDesc.size();
        for (uint32 i = 0; i < numJoints; i++)
        {
            if (_stricmp(m_arrBonesDesc[i].m_arrBoneName, strJointName) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    // Return name of bone from bone table, return zero id nId is out of range
    const char* GetJointNameByID(int32 nJointID) const
    {
        int32 numJoints = m_arrBonesDesc.size();
        if (nJointID >= 0 && nJointID < numJoints)
        {
            return m_arrBonesDesc[nJointID].m_arrBoneName;
        }
        return ""; // invalid bone id
    }
};

//////////////////////////////////////////////////////////////////////////
// This structure represents Material inside CGF.
//////////////////////////////////////////////////////////////////////////
struct CMaterialCGF
    : public _cfg_reference_target<CMaterialCGF>
{
    char name[128]; // Material name;
    int nFlags;  // Material flags.
    int nPhysicalizeType;
    bool bOldMaterial;
    float shOpacity;

    // Array of sub materials.
    DynArray<CMaterialCGF*> subMaterials;

    //////////////////////////////////////////////////////////////////////////
    // Used internally.
    int nChunkId;
    //////////////////////////////////////////////////////////////////////////

    void Init()
    {
        nFlags = 0;
        nChunkId = 0;
        bOldMaterial = false;
        nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
        shOpacity = 1.f;
    }

    CMaterialCGF() { Init(); }

    explicit CMaterialCGF(_cfg_reference_target<CMaterialCGF>::DeleteFncPtr pDeleteFnc)
        : _cfg_reference_target<CMaterialCGF>(pDeleteFnc)
    { Init(); }
};

//////////////////////////////////////////////////////////////////////////
// Info about physicalization of the CGF.
//////////////////////////////////////////////////////////////////////////
struct CPhysicalizeInfoCGF
{
    bool bWeldVertices;
    float fWeldTolerance; // Min Distance between vertices when they collapse to single vertex if bWeldVertices enabled.

    // breakable physics
    int nGranularity;
    int nMode;

    Vec3* pRetVtx;
    int nRetVtx;
    int* pRetTets;
    int nRetTets;

    CPhysicalizeInfoCGF()
        : bWeldVertices(true)
        , fWeldTolerance(0.01f)
        , nMode(-1)
        , nGranularity(-1)
        , pRetVtx(0)
        , nRetVtx(0)
        , pRetTets(0)
        , nRetTets(0){}

    ~CPhysicalizeInfoCGF()
    {
        if (pRetVtx)
        {
            delete []pRetVtx;
            pRetVtx = 0;
        }
        if (pRetTets)
        {
            delete []pRetTets;
            pRetTets = 0;
        }
    }
};


//////////////////////////////////////////////////////////////////////////
// Serialized skinnable foliage data
//////////////////////////////////////////////////////////////////////////

#define NODE_PROPERTY_STIFFNESS "stiffness"
#define NODE_PROPERTY_DAMPING   "damping"
#define NODE_PROPERTY_THICKNESS "thickness"

struct SSpineRC
{
    SSpineRC()
        : pVtx(nullptr)
        , pSegDim(nullptr)
        , nVtx(0)
        , len(0)
        , pBoneIDs(nullptr)
        , parentBoneID(-1)
        , pStiffness(nullptr)
        , pDamping(nullptr)
        , pThickness(nullptr) {}

    ~SSpineRC()
    {
        if (pVtx)
        {
            delete[] pVtx;
        }
        if (pSegDim)
        {
            delete[] pSegDim;
        }
        if (pBoneIDs)
        {
            delete[] pBoneIDs;
        }
        if (pStiffness)
        {
            delete[] pStiffness;
        }
        if (pDamping)
        {
            delete[] pDamping;
        }
        if (pThickness)
        {
            delete[] pThickness;
        }
    }

    /// Add Skinned Geometry (.CGF) export type (for touch bending vegetation)
    static float GetDefaultStiffness() { return 0.5f; }
    static float GetDefaultDamping() { return 0.5f; }
    static float GetDefaultThickness() { return 0.03f; }

    Vec3* pVtx;
    Vec4* pSegDim;
    int nVtx;
    float len;
    Vec3 navg;

    int parentBoneID;
    int* pBoneIDs;

    //Per Bone parameters.
    float* pStiffness;
    float* pDamping;
    float* pThickness;

    int iAttachSpine;
    int iAttachSeg;
};

struct SFoliageInfoCGF
{
    SFoliageInfoCGF() { nSpines = 0; pSpines = 0; pBoneMapping = 0; }
    ~SFoliageInfoCGF()
    {
        if (pSpines)
        {
            for (int i = 1; i < nSpines; i++) // spines 1..n-1 use the same buffer, so make sure they don't delete it
            {
                pSpines[i].pVtx = nullptr;
                pSpines[i].pSegDim = nullptr;
                pSpines[i].pBoneIDs = nullptr;
                pSpines[i].pStiffness = nullptr;
                pSpines[i].pDamping = nullptr;
                pSpines[i].pThickness = nullptr;
            }
            delete[] pSpines;
        }

        SAFE_DELETE_ARRAY(pBoneMapping);

        AZStd::unordered_map<AZStd::string, SMeshBoneMappingInfo_uint8*>::iterator iter = boneMappings.begin();
        while (iter != boneMappings.end())
        {
            if (iter->second != nullptr)
            {
                SAFE_DELETE_ARRAY(iter->second->pBoneMapping);
            }
            iter++;
        }

    }

    SSpineRC* pSpines;
    int nSpines;

    ///Bone mappings for each LOD level
    AZStd::unordered_map<AZStd::string, SMeshBoneMappingInfo_uint8*> boneMappings;

    ///Bone mapping for legacy format
    struct SMeshBoneMapping_uint8* pBoneMapping;
    int nSkinnedVtx;

    DynArray<uint16> chunkBoneIds;
};


//////////////////////////////////////////////////////////////////////////
struct CExportInfoCGF
{
    bool bMergeAllNodes;
    bool bUseCustomNormals;
    bool bCompiledCGF;
    bool bHavePhysicsProxy;
    bool bHaveAutoLods;
    bool bNoMesh;
    bool bWantF32Vertices;
    bool b8WeightsPerVertex;

    /// Prevent reprocessing skinning data for skinned CGF
    bool bSkinnedCGF;

    bool bFromColladaXSI;
    bool bFromColladaMAX;
    bool bFromColladaMAYA;

    unsigned int rc_version[4]; // Resource compiler version.
    char rc_version_string[16]; // Version as a string.

    unsigned int authorToolVersion;
};

//////////////////////////////////////////////////////////////////////////
// This class contain all info loaded from the CGF file.
//////////////////////////////////////////////////////////////////////////
class CContentCGF
{
public:
    //////////////////////////////////////////////////////////////////////////
    CContentCGF(const char* filename)
    {
        azstrcpy(m_filename, AZ_ARRAY_SIZE(m_filename), filename);
        memset(&m_exportInfo, 0, sizeof(m_exportInfo));
        m_exportInfo.bMergeAllNodes = true;
        m_exportInfo.bUseCustomNormals = false;
        m_exportInfo.bWantF32Vertices = false;
        m_exportInfo.b8WeightsPerVertex = false;
        m_exportInfo.bSkinnedCGF = false;
        m_pCommonMaterial = 0;
        m_bConsoleFormat = false;
        m_pOwnChunkFile = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    virtual ~CContentCGF()
    {
        // Free nodes.
        m_nodes.clear();
        if (m_pOwnChunkFile)
        {
            m_pOwnChunkFile->Release();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    const char* GetFilename() const
    {
        return m_filename;
    }

    void SetFilename(const char* filename)
    {
        azstrcpy(m_filename, AZ_ARRAY_SIZE(m_filename), filename);
    }

    //////////////////////////////////////////////////////////////////////////
    // Access to CGF nodes.
    void AddNode(CNodeCGF* pNode)
    {
        m_nodes.push_back(pNode);
    }

    int GetNodeCount() const
    {
        return m_nodes.size();
    }

    CNodeCGF* GetNode(int i)
    {
        return m_nodes[i];
    }

    const CNodeCGF* GetNode(int i) const
    {
        return m_nodes[i];
    }

    void ClearNodes()
    {
        m_nodes.clear();
    }

    void RemoveNode(CNodeCGF* pNode)
    {
        assert(pNode);
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i] == pNode)
            {
                pNode->pParent = 0;
                m_nodes.erase(i);
                break;
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Access to CGF materials.
    void AddMaterial(CMaterialCGF* pNode)
    {
        m_materials.push_back(pNode);
    }

    int GetMaterialCount() const
    {
        return m_materials.size();
    }

    CMaterialCGF* GetMaterial(int i)
    {
        return m_materials[i];
    }

    void ClearMaterials()
    {
        m_materials.clear();
    }

    CMaterialCGF* GetCommonMaterial() const
    {
        return m_pCommonMaterial;
    }

    void SetCommonMaterial(CMaterialCGF* pMtl)
    {
        m_pCommonMaterial = pMtl;
    }

    DynArray<int>& GetUsedMaterialIDs()
    {
        return m_usedMaterialIds;
    }

    const DynArray<int>& GetUsedMaterialIDs() const
    {
        return m_usedMaterialIds;
    }

    //////////////////////////////////////////////////////////////////////////

    CPhysicalizeInfoCGF* GetPhysicalizeInfo()
    {
        return &m_physicsInfo;
    }

    const CPhysicalizeInfoCGF* GetPhysicalizeInfo() const
    {
        return &m_physicsInfo;
    }

    CExportInfoCGF* GetExportInfo()
    {
        return &m_exportInfo;
    }

    const CExportInfoCGF* GetExportInfo() const
    {
        return &m_exportInfo;
    }

    CSkinningInfo* GetSkinningInfo()
    {
        return &m_SkinningInfo;
    }

    const CSkinningInfo* GetSkinningInfo() const
    {
        return &m_SkinningInfo;
    }

    SFoliageInfoCGF* GetFoliageInfo()
    {
        return &m_foliageInfo;
    }

    bool GetConsoleFormat()
    {
        return m_bConsoleFormat;
    }

    bool ValidateMeshes(const char** const ppErrorDescription) const
    {
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            const CNodeCGF* const pNode = m_nodes[i];
            if (pNode && pNode->pMesh && (!pNode->pMesh->Validate(ppErrorDescription)))
            {
                return false;
            }
        }
        return true;
    }

    // Set chunk file that this CGF owns.
    void SetChunkFile(IChunkFile* pChunkFile)
    {
        m_pOwnChunkFile = pChunkFile;
    }

public:
    bool m_bConsoleFormat;

private:
    char m_filename[260];
    CSkinningInfo m_SkinningInfo;
    DynArray<_smart_ptr<CNodeCGF> > m_nodes;
    DynArray<_smart_ptr<CMaterialCGF> > m_materials;
    DynArray<int> m_usedMaterialIds;
    _smart_ptr<CMaterialCGF> m_pCommonMaterial;

    CPhysicalizeInfoCGF m_physicsInfo;
    CExportInfoCGF m_exportInfo;
    SFoliageInfoCGF m_foliageInfo;

    IChunkFile* m_pOwnChunkFile;
};

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IAnimationGroup;
        }
    }
}

// Asset Writer interface for writing CContentCGF content to asset file
struct IAssetWriter
{
    virtual ~IAssetWriter()
    {
    }

    virtual bool WriteCGF(CContentCGF* content) = 0;
    virtual bool WriteCHR(CContentCGF* content, IConvertContext* convertContext) = 0;
    virtual bool WriteSKIN(CContentCGF* content, IConvertContext* convertContext, bool exportMorphTargets) = 0;
};


#endif // CRYINCLUDE_CRYCOMMON_CGFCONTENT_H
