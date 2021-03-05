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

#include "ResourceCompilerPC_precompiled.h"
#include "StaticObjectCompiler.h"
#include "../../CryEngine/Cry3DEngine/MeshCompiler/MeshCompiler.h"
#include "CGF/CGFNodeMerger.h"
#include "StringHelpers.h"
#include "Util.h"

#include "PropertyHelpers.h"
#include "AzFramework/StringFunc/StringFunc.h"
#include "CGFContent.h"

static const char* const s_consolesLod0MarkerStr = "consoles_lod0";
static const uint8 s_maxSkinningWeight = 0xFF;

static bool NodeHasProperty(const CNodeCGF* pNode, const char* property)
{
    return strstr(StringHelpers::MakeLowerCase(pNode->properties).c_str(), property) != 0;
}

static bool HasNodeWithConsolesLod0(const CContentCGF* pCGF)
{
    const string lodNamePrefix(CGF_NODE_NAME_LOD_PREFIX);
    const int nodeCount = pCGF->GetNodeCount();
    for (int i = 0; i < nodeCount; ++i)
    {
        const CNodeCGF* const pNode = pCGF->GetNode(i);
        if (!pNode)
        {
            continue;
        }

        if (!StringHelpers::StartsWithIgnoreCase(string(pNode->name), lodNamePrefix))
        {
            continue;
        }

        if (NodeHasProperty(pNode, s_consolesLod0MarkerStr))
        {
            return true;
        }
    }
    return false;
}

static CNodeCGF* FindNodeByName(CContentCGF* pCGF, const char* name)
{
    if (name == 0)
    {
        assert(name);
        return 0;
    }
    const int nodeCount = pCGF->GetNodeCount();
    for (int i = 0; i < nodeCount; ++i)
    {
        CNodeCGF* const pNode = pCGF->GetNode(i);
        if (pNode && strcmp(pNode->name, name) == 0)
        {
            return pNode;
        }
    }
    return 0;
}

static void ReportDuplicatedMeshNodeNames(const CContentCGF* pCGF)
{
    assert(pCGF);

    const string ignoreNamePrefix("$");

    std::set<const char*, stl::less_stricmp<const char*> > names;

    const int nodeCount = pCGF->GetNodeCount();

    for (int i = 0; i < nodeCount; ++i)
    {
        const CNodeCGF* const pNode = pCGF->GetNode(i);
        if (pNode == 0)
        {
            assert(pNode);
            continue;
        }

        const char* const pName = &pNode->name[0];

        if (pName[0] == 0)
        {
            RCLogWarning("Node with empty name found in %s", pCGF->GetFilename());
            continue;
        }

        if (StringHelpers::StartsWithIgnoreCase(string(pName), ignoreNamePrefix))
        {
            continue;
        }

        if (pNode->pMesh == 0)
        {
            continue;
        }

        if (names.find(pName) == names.end())
        {
            names.insert(pName);
        }
        else
        {
            RCLogWarning(
                "Duplicated mesh node name %s found in %s. Please make sure that all mesh nodes have unique names.",
                pName, pCGF->GetFilename());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CStaticObjectCompiler::CStaticObjectCompiler(bool bConsole, int logVerbosityLevel)
    : m_bConsole(bConsole)
    , m_logVerbosityLevel(logVerbosityLevel)
    , m_bOptimizePVRStripify(false)
{
    m_bSplitLODs = false;
    m_bOwnLod0 = false;

    for (int i = 0; i < MAX_LOD_COUNT; ++i)
    {
        m_pLODs[i] = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
CStaticObjectCompiler::~CStaticObjectCompiler()
{
    for (int i = 0; i < MAX_LOD_COUNT; ++i)
    {
        if (i != 0 || m_bOwnLod0)
        {
            delete m_pLODs[i];
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::SetSplitLods(bool bSplit)
{
    m_bSplitLODs = bSplit;
}

//////////////////////////////////////////////////////////////////////////
// Confetti: Nicholas Baldwin
void CStaticObjectCompiler::SetOptimizeStripify(bool bStripify)
{
    m_bOptimizePVRStripify = bStripify;
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* CStaticObjectCompiler::MakeCompiledCGF(CContentCGF* pCGF, bool const forceRecompile)
{
    if (pCGF->GetExportInfo()->bCompiledCGF)
    {
        const bool ok = ProcessCompiledCGF(pCGF);
        if (!ok)
        {
            return 0;
        }

        if (forceRecompile)
        {
            // most likely combined with "OptimizedPrimitiveType=1"
            // otherwise CompileMeshes() will just bail out since the CGF was already compiled.
            if (!CompileMeshes(pCGF))
            {
                return 0;
            }
        }

        return pCGF;
    }

    if (m_logVerbosityLevel > 2)
    {
        RCLog("Compiling CGF");
    }

    m_bOwnLod0 = true;
    MakeLOD(0, pCGF);

    CContentCGF* const pCompiledCGF = m_pLODs[0];

    // Setup mesh subsets for the original CGF.
    for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
    {
        CNodeCGF* const pNode = pCGF->GetNode(i);
        if (pNode->pMesh)
        {
            string errorMessage;
            if (!CGFNodeMerger::SetupMeshSubsets(pCGF, *pNode->pMesh, pNode->pMaterial, errorMessage))
            {
                RCLogError("%s: %s", __FUNCTION__, errorMessage.c_str());
                return 0;
            }
        }
    }

    if (pCGF->GetExportInfo()->bMergeAllNodes)
    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Merging nodes");
        }
        if (!MakeMergedCGF(pCompiledCGF, pCGF))
        {
            return 0;
        }
    }
    else
    {
        for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
        {
            CNodeCGF* const pNodeCGF = pCGF->GetNode(i);
            pCompiledCGF->AddNode(pNodeCGF);
        }
    }

    // Compile meshes in all nodes.
    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Compiling meshes");
        }
        if (!CompileMeshes(pCompiledCGF))
        {
            return 0;
        }
    }

    // Try to find shared meshes.
    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Searching for shared meshes");
        }
        AnalyzeSharedMeshes(pCompiledCGF);
    }

    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Physicalizing");
        }
        const bool ok = Physicalize(pCompiledCGF, pCGF);
        if (!ok)
        {
            return 0;
        }

        if (m_logVerbosityLevel > 2)
        {
            RCLog("Compiling deformable physics data");
        }
        CompileDeformablePhysData(pCompiledCGF);
    }

    if (!ValidateBoundingBoxes(pCompiledCGF))
    {
        return 0;
    }

    // Try to split LODs
    if (m_bSplitLODs)
    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Splitting to LODs");
        }
        const bool ok = SplitLODs(pCompiledCGF);
        if (!ok)
        {
            return 0;
        }
    }

    {
        if (m_logVerbosityLevel > 2)
        {
            RCLog("Validating breakable joints");
        }
        ValidateBreakableJoints(pCGF);
    }

    return pCompiledCGF;
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::ValidateBreakableJoints(const CContentCGF* pCGF)
{
    // Warn in case we have too many sub-meshes.

    const int subMeshCount = GetSubMeshCount(m_pLODs[0]);
    const int jointCount = GetJointCount(pCGF);

    enum
    {
        kBreakableSubMeshLimit = 64
    };

    if (jointCount > 0 && subMeshCount > kBreakableSubMeshLimit)
    {
        RCLogError("Breakable CGF contains %d sub-meshes (%d is the maximum): %s",
            subMeshCount, (int)kBreakableSubMeshLimit, pCGF->GetFilename());
    }
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::CompileDeformablePhysData(CContentCGF* pCompiledCGF)
{
    if (!pCompiledCGF->GetExportInfo()->bSkinnedCGF)
    {
        for (int i = 0; i < pCompiledCGF->GetNodeCount(); ++i)
        {
            if (pCompiledCGF->GetNode(i)->type == CNodeCGF::NODE_MESH)
            {
                break;
            }
        }
    }

    const string skeletonPrefix = "skeleton_";

    for (int i = 0; i < pCompiledCGF->GetNodeCount(); ++i)
    {
        CNodeCGF* pSkeletonNode = pCompiledCGF->GetNode(i);
        if (StringHelpers::StartsWithIgnoreCase(pSkeletonNode->name, skeletonPrefix))
        {
            const char* meshName = pSkeletonNode->name + skeletonPrefix.size();
            CNodeCGF* pMeshNode = FindNodeByName(pCompiledCGF, meshName);
            bool bKeepSkeleton = true; // always keep for PC

            if (!pMeshNode)
            {
                RCLogError("Unable to find mesh node \"%s\" for \"%s\"", meshName, pSkeletonNode->name);
                continue;
            }

            if (pMeshNode->pMesh == 0)
            {
                RCLogError("Node %s: Has corresponding skeleton, but no mesh. Disabling deformation.", pMeshNode->name);
                bKeepSkeleton = false;
            }

            if (m_bConsole)
            {
                if (NodeHasProperty(pSkeletonNode, "consoles_deformable")) // enabled for consoles only by UDP
                {
                    if (HasNodeWithConsolesLod0(pCompiledCGF))
                    {
                        RCLogWarning("Node %s: %s and consoles_deformable may not be used together. Disabling object deformation.",
                            pMeshNode->name, s_consolesLod0MarkerStr);
                        bKeepSkeleton = false;
                    }
                }
                else
                {
                    bKeepSkeleton = false;
                }
            }

            if (bKeepSkeleton)
            {
                float r = 0.0f;
                if (const char* ptr = strstr(pSkeletonNode->properties, "skin_dist"))
                {
                    for (; *ptr && (*ptr<'0' || * ptr>'9'); ptr++)
                    {
                        ;
                    }
                    if (*ptr)
                    {
                        r = (float)atof(ptr);
                    }
                }
                PrepareSkinData(pMeshNode, pMeshNode->localTM.GetInverted() * pSkeletonNode->localTM, pSkeletonNode, r, false);
            }
            else
            {
                SAFE_DELETE_ARRAY(pMeshNode->pSkinInfo);
                pCompiledCGF->RemoveNode(pSkeletonNode);
                --i;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::CompileMeshes(CContentCGF* pCGF)
{
    // Compile Meshes in all nodes.
    for (int i = 0; i < pCGF->GetNodeCount(); i++)
    {
        CNodeCGF* pNodeCGF = pCGF->GetNode(i);

        if (pNodeCGF->pMesh)
        {
            if (m_logVerbosityLevel > 2)
            {
                RCLog("Compiling geometry in node '%s'", pNodeCGF->name);
            }

            mesh_compiler::CMeshCompiler meshCompiler;

            const bool DegenerateFacesAreErrors = false;    // We need to get this from a CVar properly in order to make this an option, but for now,
                                                            // always treat this as a warning
            int nMeshCompileFlags =   mesh_compiler::MESH_COMPILE_TANGENTS
                                    | ((DegenerateFacesAreErrors) ? mesh_compiler::MESH_COMPILE_VALIDATE_FAIL_ON_DEGENERATE_FACES : 0)
                                    | mesh_compiler::MESH_COMPILE_VALIDATE;
            if (pCGF->GetExportInfo()->bUseCustomNormals)
            {
                nMeshCompileFlags |= mesh_compiler::MESH_COMPILE_USECUSTOMNORMALS;
            }
            if (!pNodeCGF->bPhysicsProxy)
            {
                // Confetti: Nicholas Baldwin
                nMeshCompileFlags |= (m_bOptimizePVRStripify) ? mesh_compiler::MESH_COMPILE_PVR_STRIPIFY : mesh_compiler::MESH_COMPILE_OPTIMIZE;
            }

            if (!meshCompiler.Compile(*pNodeCGF->pMesh, nMeshCompileFlags))
            {
                RCLogError("Failed to compile geometry in node '%s' in file %s - %s", pNodeCGF->name, pCGF->GetFilename(), meshCompiler.GetLastError());
                return false;
            }

            //if we dont pass in MESH_COMPILE_VALIDATE_FAIL_ON_DEGENERATE_FACES during compilation
            //it will not check for degenerate faces and fail, but we still want to warn on degenerate faces
            if (!(nMeshCompileFlags & mesh_compiler::MESH_COMPILE_VALIDATE_FAIL_ON_DEGENERATE_FACES))
            {
                if (meshCompiler.CheckForDegenerateFaces(*pNodeCGF->pMesh))
                {
                    RCLogWarning("Geometry in node '%s' in file %s contains degenerate faces. This mesh is sub optimal and should be fixed!", pNodeCGF->name, pCGF->GetFilename());
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::AnalyzeSharedMeshes(CContentCGF* pCGF)
{
    // Check if any duplicate meshes exist, and try to share them.
    mesh_compiler::CMeshCompiler meshCompiler;
    uint32 numNodes = pCGF->GetNodeCount();
    for (int i = 0; i < numNodes - 1; i++)
    {
        CNodeCGF* pNode1 = pCGF->GetNode(i);
        if (!pNode1->pMesh || pNode1->pSharedMesh)
        {
            continue;
        }

        if (!(pNode1->pMesh->GetVertexCount() && pNode1->pMesh->GetFaceCount()))
        {
            continue;
        }

        if (pNode1->bPhysicsProxy)
        {
            continue;
        }

        for (int j = i + 1; j < numNodes; j++)
        {
            CNodeCGF* pNode2 = pCGF->GetNode(j);
            if (pNode1 == pNode2 || !pNode2->pMesh)
            {
                continue;
            }

            if (pNode2->bPhysicsProxy)
            {
                continue;
            }

            if (pNode1->properties == pNode2->properties)
            {
                if (pNode1->pMesh != pNode2->pMesh && meshCompiler.CompareMeshes(*pNode1->pMesh, *pNode2->pMesh))
                {
                    // Meshes are same, share them.
                    delete pNode2->pMesh;
                    pNode2->pMesh = pNode1->pMesh;
                    pNode2->pSharedMesh = pNode1;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::ValidateBoundingBoxes(CContentCGF* pCGF)
{
    static const float maxValidObjectRadius = 10000000000.0f;
    bool ok = true;
    for (int i = 0; i < pCGF->GetNodeCount(); ++i)
    {
        const CNodeCGF* const pNode = pCGF->GetNode(i);
        if (pNode->type == CNodeCGF::NODE_MESH && pNode->pMesh && pNode->pMesh->GetVertexCount() == 0 && pNode->pMesh->GetIndexCount() == 0)
        {
            const float radius = pNode->pMesh->m_bbox.GetRadius();
            if (radius <= 0 || radius > maxValidObjectRadius || !_finite(radius))
            {
                RCLogWarning(
                    "Node '%s' in file %s has an invalid bounding box, the engine will fail to load this object. Check that the node has valid geometry and is not empty.",
                    pNode->name, pCGF->GetFilename());
                ok = false;
            }
        }
    }
    return ok;
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::Physicalize(CContentCGF* pCompiledCGF, CContentCGF* pSrcCGF)
{    
    AZ_UNUSED(pCompiledCGF);
    AZ_UNUSED(pSrcCGF);
    return true;
}

//////////////////////////////////////////////////////////////////////////

inline int check_mask(unsigned int* pMask, int i)
{
    return pMask[i >> 5] >> (i & 31) & 1;
}
inline void set_mask(unsigned int* pMask, int i)
{
    pMask[i >> 5] |= 1u << (i & 31);
}
inline void clear_mask(unsigned int* pMask, int i)
{
    pMask[i >> 5] &= ~(1u << (i & 31));
}


int UpdatePtTriDist(const Vec3* vtx, const Vec3& n, const Vec3& pt, float& minDist, float& minDenom)
{
    float rvtx[3] = { (vtx[0] - pt).len2(), (vtx[1] - pt).len2(), (vtx[2] - pt).len2() }, elen2[2], dist, denom;
    int i = idxmin3(rvtx), bInside[2];
    Vec3 edge[2], dp = pt - vtx[i];

    edge[0] = vtx[incm3(i)] - vtx[i];
    elen2[0] = edge[0].len2();
    edge[1] = vtx[decm3(i)] - vtx[i];
    elen2[1] = edge[1].len2();
    bInside[0] = isneg((dp ^ edge[0]) * n);
    bInside[1] = isneg((edge[1] ^ dp) * n);
    rvtx[i] = rvtx[i] * elen2[bInside[0]] - sqr(Util::getMax(0.0f, dp * edge[bInside[0]])) * (bInside[0] | bInside[1]);
    denom = elen2[bInside[0]];

    if (bInside[0] & bInside[1])
    {
        if (edge[0] * edge[1] < 0)
        {
            edge[0] = vtx[decm3(i)] - vtx[incm3(i)];
            dp = pt - vtx[incm3(i)];
            if ((dp ^ edge[0]) * n > 0)
            {
                dist = rvtx[incm3(i)] * edge[0].len2() - sqr(dp * edge[0]);
                denom = edge[0].len2();
                goto found;
            }
        }
        dist = sqr((pt - vtx[0]) * n), denom = n.len2();
    }
    else
    {
        dist = rvtx[i];
    }

found:
    if (dist * minDenom < minDist * denom)
    {
        minDist = dist;
        minDenom = denom;
        return 1;
    }
    return 0;
}



//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::PrepareSkinData([[maybe_unused]] CNodeCGF* pNode, [[maybe_unused]] const Matrix34& mtxSkelToMesh, [[maybe_unused]] CNodeCGF* pNodeSkel, [[maybe_unused]] float r, [[maybe_unused]] bool bSwapEndian)
{
    assert(pNode->pMesh->m_pPositionsF16 == 0);

}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::ProcessCompiledCGF(CContentCGF* pCGF)
{
    assert(pCGF->GetExportInfo()->bCompiledCGF);


    // CGF is already compiled, so we just need to perform some validation and re-compiling steps.

    ReportDuplicatedMeshNodeNames(pCGF);

    m_pLODs[0] = pCGF;
    m_bOwnLod0 = false;


    CompileDeformablePhysData(pCGF);

    // Try to split LODs
    if (m_bSplitLODs)
    {
        const bool ok = SplitLODs(pCGF);
        if (!ok)
        {
            return false;
        }
    }

    ValidateBreakableJoints(pCGF);

    return true;
}

//////////////////////////////////////////////////////////////////////////
namespace LodHelpers
{
    static bool NodeHasChildren(
        const CContentCGF* const pCGF,
        const CNodeCGF* const pNode)
    {
        for (int i = 0; i < pCGF->GetNodeCount(); ++i)
        {
            const CNodeCGF* const p = pCGF->GetNode(i);
            if (p->pParent == pNode)
            {
                return true;
            }
        }
        return false;
    }

    static int GetLodIndex(const char* const pName, const string& lodNamePrefix)
    {
        if (!StringHelpers::StartsWithIgnoreCase(string(pName), lodNamePrefix))
        {
            return -1;
        }

        int value = 0;
        const char* p = pName + lodNamePrefix.length();
        while ((p[0] >= '0') && (p[0] <= '9'))
        {
            value = value * 10 + (p[0] - '0');
            ++p;
        }

        return value;
    }

    static bool ValidateLodNodes(CContentCGF* const pCGF, const string& lodNamePrefix)
    {
        static const char* const howToFix = " Please modify and re-export source asset file.";

        for (int i = 0; i < pCGF->GetNodeCount(); ++i)
        {
            const CNodeCGF* const pNode = pCGF->GetNode(i);

            const string nodeName = pNode->name;
            if (!StringHelpers::StartsWithIgnoreCase(nodeName, lodNamePrefix))
            {
                continue;
            }

            if (nodeName.length() == lodNamePrefix.length())
            {
                RCLogError(
                    "LOD node '%s' has no index. Valid name format is '%sNxxx', where N is LOD index 1-%i and xxx is any text. File: %s.%s",
                    nodeName.c_str(), lodNamePrefix.c_str(), (int)CStaticObjectCompiler::MAX_LOD_COUNT - 1, pCGF->GetFilename(), howToFix);
                return false;
            }

            const int lodIndex = GetLodIndex(nodeName.c_str(), lodNamePrefix);
            if ((lodIndex <= 0) || (lodIndex >= CStaticObjectCompiler::MAX_LOD_COUNT))
            {
                RCLogError(
                    "LOD node '%s' has bad or missing LOD index. Valid LOD name format is '%sNxxx', where N is LOD index 1-%i and xxx is any text. File: %s.%s",
                    nodeName.c_str(), lodNamePrefix.c_str(), (int)CStaticObjectCompiler::MAX_LOD_COUNT - 1, pCGF->GetFilename(), howToFix);
                return false;
            }

            if (pNode->pParent == NULL)
            {
                RCLogError(
                    "LOD node '%s' has no parent node. File: %s.%s",
                    nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }

            if ((pNode->pParent->type != CNodeCGF::NODE_MESH) && (pNode->pParent->type != CNodeCGF::NODE_HELPER))
            {
                RCLogError(
                    "LOD0 node '%s' (parent node of LOD node '%s') is neither MESH nor HELPER. File: %s.%s",
                    pNode->pParent->name, nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }

            if (pNode->pParent->pMesh == 0)
            {
                RCLogError(
                    "LOD0 node '%s' (parent node of LOD node '%s') has no mesh data. File: %s.%s",
                    pNode->pParent->name, nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }

            if ((pNode->type != CNodeCGF::NODE_MESH) && (pNode->type != CNodeCGF::NODE_HELPER))
            {
                RCLogError(
                    "LOD node '%s' is neither MESH nor HELPER. File %s.%s",
                    nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }

            if (pNode->pMesh == 0)
            {
                RCLogError(
                    "LOD node '%s' has no mesh data. File: %s.%s",
                    nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }

            if (NodeHasChildren(pCGF, pNode))
            {
                RCLogError(
                    "LOD node '%s' has children. File: %s.%s",
                    nodeName.c_str(), pCGF->GetFilename(), howToFix);
                return false;
            }
        }

        return true;
    }

    static void FindLodNodes(
        std::vector<CNodeCGF*>& resultLodNodes,
        CContentCGF* const pCGF,
        bool bReturnSingleNode,
        const CNodeCGF* const pParent,
        const string& lodNamePrefix)
    {
        for (int i = 0; i < pCGF->GetNodeCount(); ++i)
        {
            CNodeCGF* const pNode = pCGF->GetNode(i);

            if (pParent && (pNode->pParent != pParent))
            {
                continue;
            }

            if (StringHelpers::StartsWithIgnoreCase(string(pNode->name), lodNamePrefix))
            {
                resultLodNodes.push_back(pNode);
                if (bReturnSingleNode)
                {
                    // Caller asked for *single* arbitrary LOD node
                    break;
                }
            }
        }
    }

    static bool ValidateMeshSharing(const CContentCGF* pCGF)
    {
        assert(pCGF);

        typedef std::set<CMesh*> MeshSet;
        MeshSet meshes;

        for (int i = 0; i < pCGF->GetNodeCount(); ++i)
        {
            const CNodeCGF* const pNode = pCGF->GetNode(i);
            assert(pNode);

            if (pNode->pMesh == 0)
            {
                if (pNode->pSharedMesh)
                {
                    RCLogError(
                        "Data integrity check failed on %s: node refers a shared node, but pointer to shared mesh is NULL. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
            }
            else if (pNode->pSharedMesh == 0)
            {
                const MeshSet::const_iterator it = meshes.find(pNode->pMesh);
                if (it == meshes.end())
                {
                    meshes.insert(pNode->pMesh);
                }
                else
                {
                    RCLogError(
                        "Data integrity check failed on %s: a mesh referenced from few nodes without using sharing. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
            }
            else
            {
                if (pNode == pNode->pSharedMesh)
                {
                    RCLogError(
                        "Data integrity check failed on %s: a node refers itself. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
                if (pNode->pSharedMesh->pSharedMesh)
                {
                    RCLogError(
                        "Data integrity check failed on %s: a chain of shared nodes found. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
                if (!pNode->pSharedMesh->pMesh)
                {
                    RCLogError(
                        "Data integrity check failed on %s: mesh in shared node is NULL. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
                if (pNode->pSharedMesh->pMesh != pNode->pMesh)
                {
                    RCLogError(
                        "Data integrity check failed on %s: pointer to shared mesh does not point to mesh in shared node. Contact an RC programmer.",
                        pCGF->GetFilename());
                    return false;
                }
            }
        }

        return true;
    }

    static bool CopyMeshData(CContentCGF* pCGF, CNodeCGF* pDstNode, CNodeCGF* pSrcNode)
    {
        assert(pDstNode != pSrcNode);

        if (!ValidateMeshSharing(pCGF))
        {
            return false;
        }

        if (!pSrcNode->pMesh)
        {
            RCLogError(
                "Unexpected empty LOD mesh in %s. Contact an RC programmer.",
                pCGF->GetFilename());
            return false;
        }

        if (!pDstNode->pMesh)
        {
            RCLogError(
                "Unexpected empty LOD 0 mesh in %s. Contact an RC programmer.",
                pCGF->GetFilename());
            return false;
        }

        if (pSrcNode->pSharedMesh == pDstNode)
        {
            return true;
        }

        // Make destination node "meshless"
        if (pDstNode->pSharedMesh == 0)
        {
            // Find new owner for destination node's mesh.
            CNodeCGF* newOwnerOfDstMesh = 0;
            for (int i = 0; i < pCGF->GetNodeCount(); ++i)
            {
                CNodeCGF* const pNode = pCGF->GetNode(i);
                if (pNode->pSharedMesh == pDstNode)
                {
                    if (newOwnerOfDstMesh == 0)
                    {
                        newOwnerOfDstMesh = pNode;
                        pNode->pSharedMesh = 0;
                    }
                    else
                    {
                        pNode->pSharedMesh = newOwnerOfDstMesh;
                    }
                }
            }
            if (newOwnerOfDstMesh == 0)
            {
                delete pDstNode->pMesh;
            }
        }
        pDstNode->pMesh = 0;
        pDstNode->pSharedMesh = 0;

        // Everyone who referred source node, now should refer destination node
        for (int i = 0; i < pCGF->GetNodeCount(); ++i)
        {
            CNodeCGF* const pNode = pCGF->GetNode(i);
            if (pNode->pSharedMesh == pSrcNode)
            {
                pNode->pSharedMesh = pDstNode;
            }
        }

        // Transfer mesh data from source to destination node
        pDstNode->pMesh = pSrcNode->pMesh;
        pDstNode->pSharedMesh = pSrcNode->pSharedMesh ? pSrcNode->pSharedMesh : 0;
        pSrcNode->pSharedMesh = pDstNode;

        if (!ValidateMeshSharing(pCGF))
        {
            return false;
        }

        return true;
    }

    static bool DeleteNode(CContentCGF* pCGF, CNodeCGF* pDeleteNode)
    {
        assert(pDeleteNode);

        if (!ValidateMeshSharing(pCGF))
        {
            return false;
        }

        pCGF->RemoveNode(pDeleteNode);

        if (!ValidateMeshSharing(pCGF))
        {
            return false;
        }

        return true;
    }
} // namespace LodHelpers
  //////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::SplitLODs(CContentCGF* pCGF)
{
    const string lodNamePrefix(CGF_NODE_NAME_LOD_PREFIX);

    if (!LodHelpers::ValidateMeshSharing(pCGF) || !LodHelpers::ValidateLodNodes(pCGF, lodNamePrefix))
    {
        return false;
    }

    // Check that meshes are not damaged
    for (int i = 0; i < pCGF->GetNodeCount(); ++i)
    {
        const CNodeCGF* const pNode = pCGF->GetNode(i);
        if (!pNode)
        {
            RCLogError(
                "Unexpected NULL node in %s. Contact an RC programmer.",
                pCGF->GetFilename());
            return false;
        }
        const CMesh* const pMesh = pNode->pMesh;
        if (!pMesh)
        {
            continue;
        }
        const char* pError = "";
        if (!pMesh->Validate(&pError))
        {
            RCLogError(
                "Mesh in node '%s' is damaged: %s. File %s.",
                pNode->name, pError, pCGF->GetFilename());
            return false;
        }
    }

    std::vector<CNodeCGF*> lodNodes;

    LodHelpers::FindLodNodes(lodNodes, pCGF, true, NULL, lodNamePrefix);
    if (lodNodes.empty())
    {
        // We don't have any LOD nodes. Done.
        return true;
    }

    //
    // Collect all nodes which can potentially have LODs.
    //

    struct LodableNodeInfo
    {
        CNodeCGF* pNode;
        size_t maxLodFound;
        size_t lod0Index;
        LodableNodeInfo(CNodeCGF* a_pNode, size_t a_maxLodFound, size_t a_lod0Index)
            : pNode(a_pNode)
            , maxLodFound(a_maxLodFound)
            , lod0Index(a_lod0Index)
        {
            assert(a_pNode);
        }
    };
    std::vector<LodableNodeInfo> lodableNodes;

    for (int i = 0; i < pCGF->GetNodeCount(); ++i)
    {
        CNodeCGF* const pNode = pCGF->GetNode(i);

        // Skip nodes which cannot have LODs
        {
            if (pNode->pMesh == 0)
            {
                continue;
            }

            if ((pNode->type != CNodeCGF::NODE_MESH) && (pNode->type != CNodeCGF::NODE_HELPER))
            {
                continue;
            }

            const string nodeName = pNode->name;
            if (StringHelpers::StartsWithIgnoreCase(nodeName, lodNamePrefix))
            {
                continue;
            }
        }

        //
        // Get and analyze children LOD nodes, if any
        //

        LodableNodeInfo lodsInfo(pNode, 0, 0);

        lodNodes.clear();
        LodHelpers::FindLodNodes(lodNodes, pCGF, false, pNode, lodNamePrefix);
        if (lodNodes.empty())
        {
            lodableNodes.push_back(lodsInfo);
            continue;
        }

        CNodeCGF* arrLodNodes[MAX_LOD_COUNT] = { 0 };

        for (size_t ii = 0; ii < lodNodes.size(); ++ii)
        {
            CNodeCGF* const pLodNode = lodNodes[ii];
            assert(pLodNode);
            assert(pNode == pLodNode->pParent);
            const int lodIndex = LodHelpers::GetLodIndex(pLodNode->name, lodNamePrefix);
            assert((lodIndex > 0) && (lodIndex < CStaticObjectCompiler::MAX_LOD_COUNT));
            if (arrLodNodes[lodIndex])
            {
                RCLogError(
                    "More than one node of LOD %i ('%s', '%s') attached to same parent node '%s' in file %s. Please modify and re-export source asset file.",
                    lodIndex, arrLodNodes[lodIndex]->name, pLodNode->name, pNode->name, pCGF->GetFilename());
                return false;
            }
            arrLodNodes[lodIndex] = pLodNode;

            if (lodIndex > lodsInfo.maxLodFound)
            {
                lodsInfo.maxLodFound = lodIndex;
            }
        }
        assert(arrLodNodes[0] == 0);

        //
        // Check LOD sequence for validity
        //
        {
            arrLodNodes[0] = pNode;

            // Check that we don't have gaps in the LOD sequence
            {
                int gapStart = -1;
                for (int lodIndex = 1; lodIndex < MAX_LOD_COUNT; ++lodIndex)
                {
                    if ((gapStart < 0) && !arrLodNodes[lodIndex] && arrLodNodes[lodIndex - 1])
                    {
                        gapStart = lodIndex;
                    }

                    if ((gapStart >= 0) && arrLodNodes[lodIndex])
                    {
                        RCLogError(
                            "Missing LOD node%s between '%s' and '%s' in file %s. Please modify and re-export source asset file.",
                            ((lodIndex - gapStart > 1) ? "s" : ""), arrLodNodes[gapStart - 1]->name, arrLodNodes[lodIndex]->name, pCGF->GetFilename());
                        return false;
                    }
                }
            }

            // Check that geometry simplification of the LODs is good
            for (int lodIndex = 1; lodIndex < MAX_LOD_COUNT; ++lodIndex)
            {
                const CNodeCGF* const pLod0 = arrLodNodes[lodIndex - 1];
                const CNodeCGF* const pLod1 = arrLodNodes[lodIndex];

                if (pLod1 == 0)
                {
                    break;
                }

                const int subsetCount0 = pLod0->pMesh->GetSubSetCount();
                const int subsetCount1 = pLod1->pMesh->GetSubSetCount();

                if (subsetCount1 < subsetCount0)
                {
                    // Number of draw calls decreased. The LOD is good.
                    continue;
                }

                if (subsetCount1 > subsetCount0)
                {
                    RCLogWarning(
                        "LOD node '%s' has more submaterials used than node '%s' (%d vs %d) in file %s. Please modify and re-export source asset file.",
                        pLod1->name, pLod0->name, subsetCount1, subsetCount0, pCGF->GetFilename());
                    continue;
                }

                // Number of draw calls is same. Let's check that number of faces is small enough comparing to previous LOD.

                const int faceCount0 = pLod0->pMesh->GetIndexCount() / 3;
                const int faceCount1 = pLod1->pMesh->GetIndexCount() / 3;

                static const float faceCountRatio = 1.5f;
                const int maxFaceCount1 = int(faceCount0 / faceCountRatio);

                if (faceCount1 > maxFaceCount1)
                {
                    RCLogWarning(
                        "LOD node '%s' should have less than %2.0f%% of the faces of it's parent. It has %d faces, it's parent has %d. It should have less than %d.",
                        pLod1->name, 100.0f / faceCountRatio, faceCount1, faceCount0, maxFaceCount1);
                    continue;
                }
            }

            arrLodNodes[0] = 0;
        }

        //
        // For consoles user can mark a LOD as being LOD0. Let's handle it.
        //
        if (m_bConsole)
        {
            int newLod0 = -1;
            for (int lodIndex = 0; lodIndex < MAX_LOD_COUNT; ++lodIndex)
            {
                CNodeCGF* const pLodNode = arrLodNodes[lodIndex];
                if (pLodNode && NodeHasProperty(pLodNode, s_consolesLod0MarkerStr))
                {
                    newLod0 = lodIndex;
                    break;
                }
            }

            if (newLod0 >= 0)
            {
                // Breakable objects expect rendering and physics geometry matching each other,
                // so we cannot change geometry (LOD0's by consoles_lod0's)

                const string filename = StringHelpers::MakeLowerCase(PathHelpers::GetFilename(pCGF->GetFilename()));
                if (strstr(filename.c_str(), "break"))
                {
                    RCLogWarning(
                        "Ignoring property '%s' in node '%s' because the mesh is Breakable. File %s.",
                        s_consolesLod0MarkerStr, arrLodNodes[newLod0]->name, pCGF->GetFilename());
                    newLod0 = -1;
                }
            }

            if (newLod0 >= 0)
            {
                RCLog(
                    "Found property '%s' in node '%s' of file %s. This node becomes LOD 0.",
                    s_consolesLod0MarkerStr, arrLodNodes[newLod0]->name, pCGF->GetFilename());
                lodsInfo.lod0Index = newLod0;
            }
        }

        lodableNodes.push_back(lodsInfo);
    }

    assert(!lodableNodes.empty());

    //
    // Process all nodes which can potentially have LODs.
    //

    size_t maxFinalLod = 0;
    for (size_t lodableIndex = 0; lodableIndex < lodableNodes.size(); ++lodableIndex)
    {
        const LodableNodeInfo& info = lodableNodes[lodableIndex];
        const size_t finalLod = info.maxLodFound - info.lod0Index;
        if (finalLod > maxFinalLod)
        {
            maxFinalLod = finalLod;
        }
    }

    int64 finalLodVertexCount[MAX_LOD_COUNT] = { 0 };
    int finalLodMaxAutocopyReceiver = 0;
    bool bUsedAutocopyLimit = false;

    // Two passes: first one computes resulting mesh sizes; second one
    // performs modifications to nodes and forms final lod lists.
    // Between passes we will made decision about how many lod levels
    // should be auto-copied.
    for (int pass = 0; pass < 2; ++pass)
    {
        // Find maximal LOD which should receive auto-copied LODs
        if (pass == 1)
        {
            for (int finalLodIndex = 1; finalLodIndex <= maxFinalLod; ++finalLodIndex)
            {
                const double vertexCount0 = (double)finalLodVertexCount[finalLodIndex - 1];
                const double vertexCount1 = (double)finalLodVertexCount[finalLodIndex];

                if (vertexCount1 > vertexCount0 * 0.6)
                {
                    // size of this LOD is more than 60% of previous LOD,
                    // so it's better to disable auto-copying to this LOD
                    break;
                }

                finalLodMaxAutocopyReceiver = finalLodIndex;
            }
        }

        for (size_t lodableIndex = 0; lodableIndex < lodableNodes.size(); ++lodableIndex)
        {
            const LodableNodeInfo& info = lodableNodes[lodableIndex];

            CNodeCGF* const pNode = info.pNode;
            assert(info.lod0Index + maxFinalLod >= info.maxLodFound);

            lodNodes.clear();
            LodHelpers::FindLodNodes(lodNodes, pCGF, false, pNode, lodNamePrefix);

            CNodeCGF* arrLodNodes[MAX_LOD_COUNT] = { 0 };

            for (size_t i = 0; i < lodNodes.size(); ++i)
            {
                CNodeCGF* const pLodNode = lodNodes[i];
                const int lodIndex = LodHelpers::GetLodIndex(pLodNode->name, lodNamePrefix);
                assert((lodIndex > 0) && (lodIndex < CStaticObjectCompiler::MAX_LOD_COUNT));
                arrLodNodes[lodIndex] = pLodNode;
            }
            arrLodNodes[0] = pNode;

            // Handle shifting LODs according to consoles_lod0
            if ((pass == 1) && (info.lod0Index > 0))
            {
                if (!LodHelpers::CopyMeshData(pCGF, pNode, arrLodNodes[info.lod0Index]))
                {
                    return false;
                }

                // Delete unused LOD nodes
                for (int lodIndex = 1; lodIndex <= info.lod0Index; ++lodIndex)
                {
                    if (arrLodNodes[lodIndex])
                    {
                        if (!LodHelpers::DeleteNode(pCGF, arrLodNodes[lodIndex]))
                        {
                            return false;
                        }
                    }
                }

                arrLodNodes[info.lod0Index] = pNode;
            }

            // Add LOD nodes to appropriate CGF objects
            static const bool bShowAutcopyStatistics = false;
            CNodeCGF* pLastLodNode = arrLodNodes[info.lod0Index];
            for (int finalLodIndex = 0; finalLodIndex <= maxFinalLod; ++finalLodIndex)
            {
                const size_t lodIndex = info.lod0Index + finalLodIndex;

                const bool bDuplicate = ((lodIndex >= MAX_LOD_COUNT) || (arrLodNodes[lodIndex] == 0));
                CNodeCGF* const pLodNode = bDuplicate ? pLastLodNode : arrLodNodes[lodIndex];

                if (bDuplicate && (pNode->name[0] == '$'))
                {
                    // No need to autocopy special nodes, engine doesn't handle them
                    // correctly when they are stored in LODs
                    continue;
                }

                if ((pass == 0) || (finalLodIndex == 0))
                {
                    if (pass == 0)
                    {
                        finalLodVertexCount[finalLodIndex] += pLodNode->pMesh->GetVertexCount();
                    }
                    continue;
                }

                if (bDuplicate && (pLodNode->pMesh->GetVertexCount() <= 0))
                {
                    // No need to autocopy meshes without geometry, streaming in the
                    // engine doesn't use such nodes.
                    continue;
                }

                if (bDuplicate && (finalLodIndex > finalLodMaxAutocopyReceiver))
                {
                    bUsedAutocopyLimit = true;
                    continue;
                }

                if constexpr (bShowAutcopyStatistics && bDuplicate)
                {
                    const int subsetCount = pLodNode->pMesh->GetSubSetCount();
                    const int faceCount = pLodNode->pMesh->GetIndexCount() / 3;
                    const int vertexCount = pLodNode->pMesh->GetVertexCount();
                    RCLogWarning(
                        "@,%u,%u,%u,%u,%u,%s,%s",
                        (uint)finalLodIndex - 1, (uint)maxFinalLod, (uint)subsetCount, (uint)faceCount, (uint)vertexCount, pLodNode->name, pCGF->GetFilename());
                }

                if (pLodNode != pNode)
                {
                    cry_strcpy(pLodNode->name, pNode->name);
                    pLodNode->pParent = 0;
                }

                CContentCGF* const pLodCGF = (m_pLODs[finalLodIndex] == 0)
                    ? MakeLOD(finalLodIndex, pCGF)
                    : m_pLODs[finalLodIndex];

                // Note: we should set lod nodes' pParent to 0 right before saving lod nodes to
                // a file. Otherwise parent nodes (LOD0 and its parents) will be exported also,
                // which is wrong for lod files.
                // Note that we cannot set pParent to 0 *here*, because in case of LODs auto-copying
                // the lod node can actually be the LOD0 node (pLodNome == pNode), so setting its
                // pParent to 0 will destroy LOD0's parenting info, which is bad, because we really
                // need to have proper hierarchy for LOD0 CGF.
                pLodCGF->AddNode(pLodNode);

                pLastLodNode = pLodNode;
            }

            // Delete LOD nodes from LOD0 CGF
            if (pass == 1)
            {
                for (int finalLodIndex = 1; finalLodIndex < MAX_LOD_COUNT; ++finalLodIndex)
                {
                    const size_t lodIndex = info.lod0Index + finalLodIndex;
                    if ((lodIndex < MAX_LOD_COUNT) && arrLodNodes[lodIndex])
                    {
                        if (!LodHelpers::DeleteNode(pCGF, arrLodNodes[lodIndex]))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    if (bUsedAutocopyLimit)
    {
        RCLogWarning(
            "Autocopying LODs was limited to LOD %d because vertex count difference between LODs is less than 40%% (%lli vs %lli). File %s.",
            finalLodMaxAutocopyReceiver, finalLodVertexCount[finalLodMaxAutocopyReceiver], finalLodVertexCount[finalLodMaxAutocopyReceiver + 1], pCGF->GetFilename());
    }

    if (!LodHelpers::ValidateMeshSharing(pCGF) || !LodHelpers::ValidateLodNodes(pCGF, lodNamePrefix))
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* CStaticObjectCompiler::MakeLOD(int lodIndex, const CContentCGF* pCGF)
{
    assert((lodIndex >= 0) && (lodIndex < MAX_LOD_COUNT));

    if (m_pLODs[lodIndex])
    {
        return 0;
    }

    const string filename = pCGF->GetFilename();
    m_pLODs[lodIndex] = new CContentCGF(filename.c_str());

    CContentCGF* pCompiledCGF = m_pLODs[lodIndex];
    *pCompiledCGF->GetExportInfo() = *pCGF->GetExportInfo(); // Copy export info.
    *pCompiledCGF->GetPhysicalizeInfo() = *pCGF->GetPhysicalizeInfo();
    pCompiledCGF->GetExportInfo()->bCompiledCGF = true;
    pCompiledCGF->GetUsedMaterialIDs() = pCGF->GetUsedMaterialIDs();
    *pCompiledCGF->GetSkinningInfo() = *pCGF->GetSkinningInfo();

    if (lodIndex > 0 && m_pLODs[0])
    {
        m_pLODs[0]->GetExportInfo()->bHaveAutoLods = true;
    }

    return m_pLODs[lodIndex];
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::MakeMergedCGF(CContentCGF* pCompiledCGF, CContentCGF* pCGF)
{
    std::vector<CNodeCGF*> merge_nodes;
    for (int i = 0; i < pCGF->GetNodeCount(); i++)
    {
        CNodeCGF* pNode = pCGF->GetNode(i);
        if (pNode->pMesh && !pNode->bPhysicsProxy && pNode->type == CNodeCGF::NODE_MESH)
        {
            merge_nodes.push_back(pNode);
        }
    }

    if (merge_nodes.empty())
    {
        RCLogError("Error merging nodes, No mergeable geometry in CGF %s", pCGF->GetFilename());
        return false;
    }

    CMesh* pMergedMesh = new CMesh;
    string errorMessage;
    if (!CGFNodeMerger::MergeNodes(pCGF, merge_nodes, errorMessage, pMergedMesh))
    {
        RCLogError("Error merging nodes: %s, in CGF %s", errorMessage.c_str(), pCGF->GetFilename());
        delete pMergedMesh;
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // Add single node for merged mesh.
    CNodeCGF* pNode = new CNodeCGF;
    pNode->type = CNodeCGF::NODE_MESH;
    cry_strcpy(pNode->name, "Merged");
    pNode->bIdentityMatrix = true;
    pNode->pMesh = pMergedMesh;
    pNode->pMaterial = pCGF->GetCommonMaterial();

    CNodeCGF* pMergedNode = pNode;

    // Add node to CGF contents.
    pCompiledCGF->AddNode(pNode);
    //////////////////////////////////////////////////////////////////////////

    const string lodNamePrefix(CGF_NODE_NAME_LOD_PREFIX);

    for (int nLod = 1; nLod < MAX_LOD_COUNT; nLod++)
    {
        // Try to merge LOD nodes.
        merge_nodes.clear();
        for (int i = 0; i < pCGF->GetNodeCount(); i++)
        {
            CNodeCGF* pNode2 = pCGF->GetNode(i);
            if (pNode2->pMesh && !pNode2->bPhysicsProxy && pNode2->type == CNodeCGF::NODE_HELPER)
            {
                const string nodeName(pNode2->name);
                if (StringHelpers::StartsWithIgnoreCase(nodeName, lodNamePrefix))
                {
                    if (nodeName.length() <= lodNamePrefix.length())
                    {
                        RCLogError("Error merging LOD %d nodes: LOD node name '%s' doesn't contain LOD index in CGF %s", nLod, nodeName.c_str(), pCGF->GetFilename());
                        return false;
                    }
                    const int lodIndex = atoi(nodeName.c_str() + lodNamePrefix.length());
                    if (lodIndex == nLod)
                    {
                        // This is a LOD helper.
                        merge_nodes.push_back(pNode2);
                    }
                }
            }
        }
        if (!merge_nodes.empty())
        {
            CMesh* pMergedLodMesh = new CMesh;
            string errorMessage2;
            if (!CGFNodeMerger::MergeNodes(pCGF, merge_nodes, errorMessage2, pMergedLodMesh))
            {
                RCLogError("Error merging LOD %d nodes: %s, in CGF %s", nLod, errorMessage2.c_str(), pCGF->GetFilename());
                delete pMergedLodMesh;
                return false;
            }

            //////////////////////////////////////////////////////////////////////////
            // Add LOD node
            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // Add single node for merged mesh.
            CNodeCGF* pNode2 = new CNodeCGF;
            pNode2->type = CNodeCGF::NODE_HELPER;
            pNode2->helperType = HP_GEOMETRY;
            azsnprintf(pNode2->name, sizeof(pNode2->name), "%s%d_%s", lodNamePrefix.c_str(), nLod, pMergedNode->name);
            pNode2->bIdentityMatrix = true;
            pNode2->pMesh = pMergedLodMesh;
            pNode2->pParent = pMergedNode;
            pNode2->pMaterial = pCGF->GetCommonMaterial();

            // Add node to CGF contents.
            pCompiledCGF->AddNode(pNode2);
            //////////////////////////////////////////////////////////////////////////
        }
    }

    // Add rest of the helper nodes.
    int numNodes = pCGF->GetNodeCount();
    for (int i = 0; i < numNodes; i++)
    {
        CNodeCGF* pNode2 = pCGF->GetNode(i);

        if (pNode2->type != CNodeCGF::NODE_MESH)
        {
            // Do not add LOD nodes.
            if (!StringHelpers::StartsWithIgnoreCase(string(pNode2->name), lodNamePrefix))
            {
                if (pNode2->pParent && pNode2->pParent->type == CNodeCGF::NODE_MESH)
                {
                    pNode2->pParent = pMergedNode;
                }
                pCompiledCGF->AddNode(pNode2);
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
int CStaticObjectCompiler::GetSubMeshCount(const CContentCGF* pCGFLod0)
{
    int subMeshCount = 0;
    if (pCGFLod0)
    {
        const int nodeCount = pCGFLod0->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            const CNodeCGF* const pNode = pCGFLod0->GetNode(i);
            if (pNode && pNode->type == CNodeCGF::NODE_MESH && pNode->pMesh)
            {
                ++subMeshCount;
            }
        }
    }
    return subMeshCount;
}

//////////////////////////////////////////////////////////////////////////
int CStaticObjectCompiler::GetJointCount(const CContentCGF* pCGF)
{
    int jointCount = 0;
    if (pCGF)
    {
        const int nodeCount = pCGF->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            const CNodeCGF* const pNode = pCGF->GetNode(i);
            if (pNode && pNode->type == CNodeCGF::NODE_HELPER && StringHelpers::StartsWith(pNode->name, "$joint"))
            {
                ++jointCount;
            }
        }
    }
    return jointCount;
}
