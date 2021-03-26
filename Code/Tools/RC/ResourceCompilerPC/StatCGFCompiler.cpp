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
#include "ConvertContext.h"
#include "IConfig.h"
#include "StatCGFCompiler.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>

#include "../CryEngine/Cry3DEngine/CGF/CGFLoader.h"
#include "CGF/CGFSaver.h"
#include "CryVersion.h"
#include "StaticObjectCompiler.h"
#include "FileUtil.h"
#include "MathHelpers.h"
#include "StringHelpers.h"
#include "UpToDateFileHelpers.h"

AZ::ComponentTypeList CGFToolApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();

    auto removed = AZStd::remove_if(components.begin(), components.end(),
        [](const AZ::Uuid& id) -> bool
    {
        return id == azrtti_typeid<AzFramework::TargetManagementComponent>()
            || id == azrtti_typeid<AzToolsFramework::PerforceComponent>()
            || id == azrtti_typeid<AZ::UserSettingsComponent>()
            || id == azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>();
    });
    components.erase(removed, components.end());

    return components;
}

void CGFToolApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
{
    AzToolsFramework::ToolsApplication::SetSettingsRegistrySpecializations(specializations);
    specializations.Append("statcgfcompiler");
}

/////////////////////////////////////////////////////////////////////////
CStatCGFCompiler::CStatCGFCompiler()
{
    //MessageBox(NULL, "Pres OK baton ->", "Ok?", MB_OK);

    m_refCount = 1;
}

//////////////////////////////////////////////////////////////////////////
CStatCGFCompiler::~CStatCGFCompiler()
{
}

//////////////////////////////////////////////////////////////////////////
string CStatCGFCompiler::GetOutputFileNameOnly() const
{
    string sourceFileFinal = m_CC.m_config->GetAsString("overwritefilename", m_CC.m_sourceFileNameOnly.c_str(), m_CC.m_sourceFileNameOnly.c_str());

    if (StringHelpers::EqualsIgnoreCase(PathHelpers::FindExtension(sourceFileFinal), "i_cgf"))
    {
        sourceFileFinal = PathHelpers::ReplaceExtension(sourceFileFinal, "cgf");
    }

    const string ext = PathHelpers::FindExtension(sourceFileFinal);
    if (StringHelpers::EqualsIgnoreCase(ext, "cgf") || StringHelpers::EqualsIgnoreCase(ext, "cga"))
    {
        if (m_CC.m_config->GetAsBool("StripNonMesh", false, true))
        {
            sourceFileFinal += "m";
        }
    }

    return sourceFileFinal;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CStatCGFCompiler::GetDependencyAbsolutePath(const AZStd::string& fileName) const
{
    AZStd::string cgfSourcePath = PathHelpers::GetDirectory(m_CC.GetSourcePath().c_str()).c_str();
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, cgfSourcePath);
                        
    // register the absolute path of the file as the dependency here, as the asset catalog will take care
    //  of resolving it to its proper scan folder and path relative to that scan folder.
    AZStd::string absoluteFilePath = fileName.c_str();
    if (!cgfSourcePath.empty())
    {
        AzFramework::StringFunc::AssetDatabasePath::Join(cgfSourcePath.c_str(), fileName.c_str(), absoluteFilePath);
        AzFramework::StringFunc::AssetDatabasePath::Normalize(absoluteFilePath);
    }
    return absoluteFilePath;
}

//////////////////////////////////////////////////////////////////////////
string CStatCGFCompiler::GetOutputPath() const
{
    return PathHelpers::Join(m_CC.GetOutputFolder(), GetOutputFileNameOnly());
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::Release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
ICompiler* CStatCGFCompiler::CreateCompiler()
{
    // Only ever return one compiler, since we don't support multithreading. Since
    // the compiler is just this object, we can tell whether we have already returned
    // a compiler by checking the ref count.
    if (m_refCount >= 2)
    {
        return 0;
    }

    // Until we support multithreading for this convertor, the compiler and the
    // convertor may as well just be the same object.
    ++m_refCount;
    return this;
}

//////////////////////////////////////////////////////////////////////////
static string getTextMatrix(const Matrix34& M)
{
    string text = "";

    char buff[200];

    azsnprintf(buff, sizeof(buff), "axisX(%g %g %g)", M.m00, M.m10, M.m20);
    text += buff;
    azsnprintf(buff, sizeof(buff), " axisY(%g %g %g)", M.m01, M.m11, M.m21);
    text += buff;
    azsnprintf(buff, sizeof(buff), " axisZ(%g %g %g)", M.m02, M.m12, M.m22);
    text += buff;
    azsnprintf(buff, sizeof(buff), " trans(%g %g %g)", M.m03, M.m13, M.m23);
    text += buff;

    return text;
}

static const char* getTextNodeType(CNodeCGF::ENodeType const type)
{
    const char* text;

    switch (type)
    {
    case CNodeCGF::NODE_HELPER:
        text = "NODE_HELPER";
        break;
    case CNodeCGF::NODE_LIGHT:
        text = "NODE_LIGHT";
        break;
    case CNodeCGF::NODE_MESH:
        text = "NODE_MESH";
        break;
    default:
        text = "***UNKNOWN***";
        break;
    }

    return text;
}

static const char* getTextHelperType(HelperTypes const type)
{
    const char* text;

    switch (type)
    {
    case HP_POINT:
        text = "HP_POINT";
        break;
    case HP_DUMMY:
        text = "HP_DUMMY";
        break;
    case HP_XREF:
        text = "HP_XREF";
        break;
    case HP_CAMERA:
        text = "HP_CAMERA";
        break;
    case HP_GEOMETRY:
        text = "HP_GEOMETRY";
        break;
    default:
        text = "*** UNKNOWN ***";
        break;
    }

    return text;
}

static string getTextPhysicalizeType(int const type)
{
    string text;

    switch (type)
    {
    case PHYS_GEOM_TYPE_NONE:
        text += "PHYS_GEOM_TYPE_NONE";
        break;
    case PHYS_GEOM_TYPE_DEFAULT:
        text += "PHYS_GEOM_TYPE_DEFAULT";
        break;
    case PHYS_GEOM_TYPE_NO_COLLIDE:
        text += "PHYS_GEOM_TYPE_NO_COLLIDE";
        break;
    case PHYS_GEOM_TYPE_OBSTRUCT:
        text += "PHYS_GEOM_TYPE_OBSTRUCT";
        break;
    case PHYS_GEOM_TYPE_DEFAULT_PROXY:
        text += "PHYS_GEOM_TYPE_DEFAULT_PROXY";
        break;
    default:
    {
        text += "*** UNKNOWN ***";
        char buff[50];
        azsnprintf(buff, sizeof(buff), " (0x%x)", type);
        text += buff;
    }
    break;
    }

    return text;
}

static string getTextPhysicalizeFlags(int const flags)
{
    string text;

    switch (flags)
    {
    case 0:
        text += "none";
        break;
    case CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded:
        text += "MeshNotNeeded";
        break;
    default:
    {
        text += "*** UNKNOWN ***";
        char buff[50];
        azsnprintf(buff, sizeof(buff), " (0x%x)", flags);
        text += buff;
    }
    break;
    }

    return text;
}

bool CStatCGFCompiler::DebugDumpCGF(const char* sourceFileName, const char* outputFilePath)
{
    RCLog("Dumping geometry file %s...", sourceFileName);

    class Listener
        : public ILoaderCGFListener
    {
    public:
        Listener()
        {
        }
        virtual void Warning(const char* format)
        {
            RCLogWarning("%s", format);
        }
        virtual void Error(const char* format)
        {
            RCLogError("%s", format);
        }
    };

    Listener listener;
    CChunkFile chunkFile;
    CLoaderCGF cgfLoader;
    CContentCGF* pCGF = cgfLoader.LoadCGF(sourceFileName, chunkFile, &listener);

    if (!pCGF)
    {
        RCLogError("Dump: Failed to load geometry file %s - %s", sourceFileName, cgfLoader.GetLastError());
        return false;
    }

    if (pCGF->GetConsoleFormat())
    {
        RCLogError("Dump: Cannot dump geometry file %s because it's in console format.", sourceFileName);
        delete pCGF;
        return false;
    }

    string dumpFilename = outputFilePath;
    dumpFilename += ".dump";    

    char resolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(dumpFilename.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

    FILE* f = nullptr; 
    azfopen(&f, resolvedPath, "wt");

    if (f == 0)
    {
        RCLogError("Dump: Cannot create dump file %s.", resolvedPath);
        delete pCGF;
        return false;
    }


    fprintf(f, "<<< Dump of '%s' >>>\n", sourceFileName);
    fprintf(f, "\n");

    fprintf(f, "---------------------------------------------\n");
    fprintf(f, "material#: %d\n", pCGF->GetMaterialCount());
    fprintf(f, "\n");

    std::vector<CMesh*> meshes;

    for (int materialIdx = 0; materialIdx < pCGF->GetMaterialCount(); ++materialIdx)
    {
        fprintf(f, "material[%d]:\n", materialIdx);
        const CMaterialCGF* pMat = pCGF->GetMaterial(materialIdx);

        if (pMat == 0)
        {
            fprintf(f, "\t" "***NULL MATERIAL***");
            continue;
        }

        fprintf(f, "\t" "name: '%s'\n", pMat->name);
        fprintf(f, "\t" "nPhysicalizeType: %s\n", getTextPhysicalizeType(pMat->nPhysicalizeType).c_str());

        fprintf(f, "\t" "subMaterials[]: size=%d, capacity=%d\n", pMat->subMaterials.size(), pMat->subMaterials.capacity());
        for (int i = 0; i < pMat->subMaterials.size(); ++i)
        {
            CMaterialCGF* const pSubMat = pMat->subMaterials[i];

            fprintf(f, "\t\t" "subMaterials[%d]: ", i);
            if (pSubMat == 0)
            {
                fprintf(f, "NULL");
            }
            else
            {
                // Trying to find pSubMat in our materials
                bool found = false;
                for (int j = 0; j < pCGF->GetMaterialCount(); ++j)
                {
                    if (pSubMat == pCGF->GetMaterial(j))
                    {
                        found = true;
                        fprintf(f, "material[%d] '%s'", j, pSubMat->name);
                        break;
                    }
                }
                if (!found)
                {
                    fprintf(f, "*** not in material[] *** '%s'", pSubMat->name);
                }
            }
            fprintf(f, "\n");
        }

        fprintf(f, "\t" "internals:\n");
        fprintf(f, "\t\t" "nChunkId: %d\n", pMat->nChunkId);

        fprintf(f, "\n");
    }
    fprintf(f, "---------------------------------------------\n");

    {
        CMaterialCGF* pMat = pCGF->GetCommonMaterial();

        fprintf(f, "\t" "pCommonMaterial: ");
        if (pMat == 0)
        {
            fprintf(f, "NULL");
        }
        else
        {
            // Trying to find commonmaterial in our materials
            bool found = false;
            for (int j = 0; j < pCGF->GetMaterialCount(); ++j)
            {
                if (pMat == pCGF->GetMaterial(j))
                {
                    found = true;
                    fprintf(f, "material[%d] '%s'", j, pMat->name);
                    break;
                }
            }
            if (!found)
            {
                fprintf(f, "*** not in material[] *** '%s'", pMat->name);
            }
        }
        fprintf(f, "\n");
    }

    {
        const CPhysicalizeInfoCGF* pPhys = pCGF->GetPhysicalizeInfo();
        fprintf(f, "\t" "PhysicalizeInfo: (not printed yet)\n");
    }

    {
        CExportInfoCGF* pExport = pCGF->GetExportInfo();
        fprintf(f, "\t" "ExportInfo: (not printed yet)\n");
    }

    {
        CSkinningInfo* pSkin = pCGF->GetSkinningInfo();
        fprintf(f, "\t" "SkinningInfo: (not printed yet)\n");
    }

    {
        SFoliageInfoCGF* pSkin = pCGF->GetFoliageInfo();
        fprintf(f, "\t" "FoliageInfo: (not printed yet)\n");
    }

    fprintf(f, "---------------------------------------------\n");

    fprintf(f, "node#: %d\n", pCGF->GetNodeCount());
    fprintf(f, "\n");

    for (int nodeIdx = 0; nodeIdx < pCGF->GetNodeCount(); ++nodeIdx)
    {
        fprintf(f, "node[%d]:\n", nodeIdx);
        const CNodeCGF* pNode = pCGF->GetNode(nodeIdx);

        if (pNode == 0)
        {
            fprintf(f, "\t" "***NULL NODE***");
            continue;
        }

        fprintf(f, "\t" "type: %s\n", getTextNodeType(pNode->type));
        fprintf(f, "\t" "name: '%s'\n", pNode->name);
        fprintf(f, "\t" "properties: '%s'\n", pNode->properties.c_str());
        fprintf(f, "\t" "localTM: %s\n", getTextMatrix(pNode->localTM).c_str());
        fprintf(f, "\t" "worldTM: %s\n", getTextMatrix(pNode->worldTM).c_str());

        fprintf(f, "\t" "pParent: ");
        if (pNode->pParent == 0)
        {
            fprintf(f, "NULL");
        }
        else
        {
            // Trying to find pParent node in our nodes
            bool found = false;
            for (int i = 0; i < pCGF->GetNodeCount(); ++i)
            {
                if (pNode->pParent == pCGF->GetNode(i))
                {
                    found = true;
                    fprintf(f, "node[%d] '%s'", i, pNode->pParent->name);
                    break;
                }
            }
            if (!found)
            {
                fprintf(f, "*** not in node[] *** '%s'", pNode->pParent->name);
            }
        }
        fprintf(f, "\n");

        fprintf(f, "\t" "pSharedMesh: ");
        if (pNode->pSharedMesh == 0)
        {
            fprintf(f, "NULL");
        }
        else
        {
            // Trying to find pSharedMesh node in our nodes
            bool found = false;
            for (int i = 0; i < pCGF->GetNodeCount(); ++i)
            {
                if (pNode->pSharedMesh == pCGF->GetNode(i))
                {
                    found = true;
                    fprintf(f, "node[%d] '%s'", i, pNode->pSharedMesh->name);
                    break;
                }
            }
            if (!found)
            {
                fprintf(f, "*** not in node[] *** '%s'", pNode->pSharedMesh->name);
            }
        }
        fprintf(f, "\n");

        //if (pNode->type == CNodeCGF::NODE_MESH)
        {
            fprintf(f, "\t" "pMesh: ");
            CMesh* const pMesh = pNode->pMesh;
            if (pMesh == 0)
            {
                fprintf(f, "NULL");
            }
            else
            {
                unsigned int i;
                for (i = 0; i < meshes.size(); ++i)
                {
                    if (meshes[i] == pMesh)
                    {
                        break;
                    }
                }
                if (i >= meshes.size())
                {
                    meshes.push_back(pMesh);
                    i = meshes.size() - 1;
                }
                fprintf(f, "\t" "mesh[%d]", i);
            }
            fprintf(f, "\n");
        }

        if (pNode->type == CNodeCGF::NODE_HELPER)
        {
            fprintf(f, "\t" "helperType: %s\n", getTextHelperType(pNode->helperType));
            fprintf(f, "\t" "helperSize: %g %g %g\n", pNode->helperSize.x, pNode->helperSize.y, pNode->helperSize.z);
        }

        fprintf(f, "\t" "pMaterial: ");
        if (pNode->pMaterial == 0)
        {
            fprintf(f, "NULL");
        }
        else
        {
            // Trying to find pMaterial in our materials
            bool found = false;
            for (int i = 0; i < pCGF->GetMaterialCount(); ++i)
            {
                if (pNode->pMaterial == pCGF->GetMaterial(i))
                {
                    found = true;
                    fprintf(f, "material[%d] '%s'", i, pNode->pMaterial->name);
                    break;
                }
            }
            if (!found)
            {
                fprintf(f, "*** not in material[] *** '%s'", pNode->pMaterial->name);
            }
        }
        fprintf(f, "\n");

        fprintf(f, "\t" "nPhysicalizeFlags: %s\n", getTextPhysicalizeFlags(pNode->nPhysicalizeFlags).c_str());

        {
            bool bIsEmpty = true;

            for (int i = 0; i < 4; ++i)
            {
                const bool bSlotIsEmpty = (pNode->physicalGeomData[i].size() == 0) && (pNode->physicalGeomData[i].capacity() == 0);
                if (!bSlotIsEmpty)
                {
                    bIsEmpty = false;
                    break;
                }
            }

            if (bIsEmpty)
            {
                fprintf(f, "\t" "physicalGeomData[]: empty\n");
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                {
                    const bool bSlotIsEmpty = (pNode->physicalGeomData[i].size() == 0) && (pNode->physicalGeomData[i].capacity() == 0);
                    if (!bSlotIsEmpty)
                    {
                        fprintf(f, "\t" "physicalGeomData[%d]: size=%zu, capacity=%zu\n", i, pNode->physicalGeomData[i].size(), pNode->physicalGeomData[i].capacity());
                    }
                }
            }
        }

        fprintf(f, "\t" "internals:\n");

        fprintf(f, "\t\t" "nChunkId:       %d\n", pNode->nChunkId);
        fprintf(f, "\t\t" "nParentChunkId: %d\n", pNode->nParentChunkId);
        fprintf(f, "\t\t" "nObjectChunkId: %d\n", pNode->nObjectChunkId);
        fprintf(f, "\t\t" "pos_controller_id:    %d\n", pNode->pos_cont_id);
        fprintf(f, "\t\t" "rot_controller_id:    %d\n", pNode->rot_cont_id);
        fprintf(f, "\t\t" "scl_controller_id:    %d\n", pNode->scl_cont_id);

        fprintf(f, "\t" "bIdentityMatrix: %s\n", (pNode->bIdentityMatrix ? "true" : "false"));
        fprintf(f, "\t" "bPhysicsProxy:   %s\n", (pNode->bPhysicsProxy   ? "true" : "false"));
        fprintf(f, "\n");
    }
    fprintf(f, "---------------------------------------------\n");

    bool hasError = false;

    fprintf(f, "mesh#: %zu\n", meshes.size());
    fprintf(f, "\n");

    for (int meshIdx = 0; meshIdx < meshes.size(); ++meshIdx)
    {
        fprintf(f, "mesh[%d]:\n", meshIdx);

        CMesh* const pMesh = meshes[meshIdx];

        const char* pErrorDescription = 0;
        bool const bMeshValid = pMesh->Validate(&pErrorDescription);
        if (!bMeshValid)
        {
            hasError = true;
            fprintf(f, "\t\t" "** ERROR **: mesh is invalid (%s)\n", pErrorDescription);
        }

        fprintf(f, "\t" "numFaces:    %d\n", pMesh->GetFaceCount());
        fprintf(f, "\t" "numVertices: %d\n", pMesh->GetVertexCount());
        fprintf(f, "\t" "nCoorCount:  %d\n", pMesh->GetTexCoordCount());
        fprintf(f, "\t" "nIndexCount: %d\n", pMesh->GetIndexCount());

        fprintf(f,
            "\t" "bbox: min(%g %g %g) max(%g %g %g)\n",
            pMesh->m_bbox.min.x, pMesh->m_bbox.min.y, pMesh->m_bbox.min.z,
            pMesh->m_bbox.max.x, pMesh->m_bbox.max.y, pMesh->m_bbox.max.z);

        fprintf(f, "\t" "texMappingDensity: %g\n", pMesh->m_texMappingDensity);

        fprintf(f, "\t" "subsets[]: size=%d, capacity=%d\n", pMesh->m_subsets.size(), pMesh->m_subsets.capacity());
        for (int i = 0; i < pMesh->m_subsets.size(); ++i)
        {
            SMeshSubset& subset = pMesh->m_subsets[i];

            fprintf(f, "\t" "subset[%d]:\n", i);

            fprintf(f, "\t\t" "center: %g %g %g\n", subset.vCenter.x, subset.vCenter.y, subset.vCenter.z);
            fprintf(f, "\t\t" "radius: %g\n", subset.fRadius);

            fprintf(f, "\t\t" "nFirstIndexId: %d\n", subset.nFirstIndexId);
            fprintf(f, "\t\t" "nNumIndices: %d\n", subset.nNumIndices);

            fprintf(f, "\t\t" "nFirstVertId: %d\n", subset.nFirstVertId);
            fprintf(f, "\t\t" "nNumVerts: %d\n", subset.nNumVerts);

            fprintf(f, "\t\t" "nMatID: %d\n", subset.nMatID);
            fprintf(f, "\t\t" "nMatFlags: 0x%08x\n", subset.nMatFlags);
            fprintf(f, "\t\t" "nPhysicalizeType: %s\n", getTextPhysicalizeType(subset.nPhysicalizeType).c_str());

            if (!bMeshValid)
            {
                // Check indices
                if (subset.nNumIndices > 0)
                {
                    const vtx_idx* const pIndices = pMesh->m_pIndices;
                    if (pIndices == 0)
                    {
                        fprintf(f, "\t\t" "pIndices: NULL   ** ERROR **\n");
                        break;
                    }
                    if (subset.nNumIndices % 3)
                    {
                        fprintf(f, "\t\t" "nNumIndices is not multiplication of 3   ** ERROR **\n");
                        break;
                    }
                    if (subset.nFirstIndexId < 0)
                    {
                        fprintf(f, "\t\t" "nFirstIndexId < 0  ** ERROR **\n");
                        break;
                    }
                    if (subset.nFirstIndexId + subset.nNumIndices > pMesh->GetIndexCount())
                    {
                        fprintf(f, "\t\t" "nIndices + numIndices > mesh's nIndexCount   ** ERROR **\n");
                        break;
                    }
                    if (subset.nNumVerts <= 0)
                    {
                        fprintf(f, "\t\t" "nNumVerts <= 0  ** ERROR **\n");
                        break;
                    }
                    if (subset.nFirstVertId < 0)
                    {
                        fprintf(f, "\t\t" "nFirstVertId < 0  ** ERROR **\n");
                        break;
                    }
                    if (subset.nFirstVertId + subset.nNumVerts > pMesh->GetVertexCount())
                    {
                        fprintf(f, "\t\t" "nFirstVertId + nNumVerts > mesh's numVertices   ** ERROR **\n");
                        break;
                    }
                    for (int ii = 0; ii < subset.nNumIndices; ++ii)
                    {
                        const int vertexIndex = pIndices[subset.nFirstIndexId + ii];
                        const int nLastVertId = subset.nFirstVertId + subset.nNumVerts - 1;
                        if ((vertexIndex < subset.nFirstVertId) || (vertexIndex > nLastVertId))
                        {
                            fprintf(f, "\t\t" "%ith index refers to vertex %i (outside of [%i;%i])   ** ERROR **\n", ii, vertexIndex, subset.nFirstIndexId, nLastVertId);
                            break;
                        }
                    }
                }
            }
        }

        fprintf(f, "\n");
    }

    if (hasError)
    {
        fprintf(f, "** ATTENTION! At least one mesh has fatal errors in geometry. Search for '** ERROR **' above\n");
    }

    fprintf(f, "<<< End of dump of '%s' >>>\n", sourceFileName);

    fclose(f);
    f = 0;

    delete pCGF;
    pCGF = 0;

    RCLog("Finished dumping geometry file %s.", sourceFileName);

    return true;
}


bool debugValidateCGF(CContentCGF* pCGF, const char* a_filename)
{
    RCLog("Validating geometry file %s...", a_filename);

    if (!pCGF)
    {
        RCLogError("Validate: Failed to load geometry file %s", a_filename);
        return false;
    }

    if (pCGF->GetConsoleFormat())
    {
        RCLogError("Validate: Cannot validate geometry file %s because it's in console format.", a_filename);
        return false;
    }

    const char* pErrorDescription = 0;
    if (!pCGF->ValidateMeshes(&pErrorDescription))
    {
        RCLogError("Validate: Geometry file %s is damaged (%s).", a_filename, pErrorDescription);
        return false;
    }

    RCLog("Validate: Geometry file %s is ok.", a_filename);
    return true;
}

#if defined(AZ_PLATFORM_WINDOWS)
#include <conio.h>
#endif
//////////////////////////////////////////////////////////////////////////
bool CStatCGFCompiler::Process()
{
    // Register the AssetBuilderSDK structures needed later on.
    CGFToolApplication application;
    AZ::ComponentApplication::Descriptor descriptor;
    application.Start(descriptor);

    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::ProcessJobResponse response;

    // We want to query the watch folder here for cases where we have to build a material relative path dependency.
    // This is necessary for cases where the source assets lie outside of the game project directory
    AZStd::string sourceWatchFolder = m_CC.m_config->GetAsString("watchfolder", "", "").c_str();
    if (sourceWatchFolder.empty())
    {
        // Fallbacks just in case watchfolder is not set for some reason
        sourceWatchFolder = m_CC.m_config->GetAsString("sourceroot", "", "").c_str();
        if (sourceWatchFolder.empty())
        {
            sourceWatchFolder = m_CC.m_config->GetAsString("gameroot", "", "").c_str();
        }
    }

    bool compileSuccess = CompileCGF(response, sourceWatchFolder);

    return WriteResponse(m_CC.GetOutputFolder(), response, compileSuccess);
}

//////////////////////////////////////////////////////////////////////////

bool CStatCGFCompiler::CompileCGF(AssetBuilderSDK::ProcessJobResponse& response, AZStd::string assetRoot, AZStd::string gameFolder)
{
#if defined(AZ_PLATFORM_WINDOWS)
    // _EM_INVALID is used to avoid Floating Point Exception inside CryPhysics
    MathHelpers::AutoFloatingPointExceptions autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW | _EM_INVALID));
#endif

    // If no game project was specified, then query it from the Settings Registry
    if (gameFolder.empty())
    {
        gameFolder = AZ::Utils::GetProjectName();
    }

    // If no asset root was specified, then query it from the application
    if (assetRoot.empty())
    {
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(assetRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
        }
    }
    
    const string sourceFile = m_CC.GetSourcePath();
    const string outputFile = GetOutputPath();

#if !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE) // Exception handling not enabled on linux/mac builds
    try
#endif
    {
        const bool bStripMeshData = m_CC.m_config->HasKey("StripMesh");
        const bool bStripNonMeshData = m_CC.m_config->GetAsBool("StripNonMesh", false, true);
        const bool bCompactVertexStreams = m_CC.m_config->GetAsBool("CompactVertexStreams", false, true);
        // Confetti: Nicholas Baldwin
        const bool bOptimizePVRStripify = m_CC.m_config->GetAsInt("OptimizedPrimitiveType", 0, 0) == 1;
        const bool bComputeSubsetTexelDensity = m_CC.m_config->GetAsBool("ComputeSubsetTexelDensity", false, true);
        const bool bSplitLods = m_CC.m_config->GetAsBool("SplitLODs", false, true);

        if (m_CC.m_config->GetAsBool("debugdump", false, true))
        {
            // Write a debug .dump file into the cache
            DebugDumpCGF(sourceFile.c_str(), GetOutputPath());
        }

        class Listener
            : public ILoaderCGFListener
        {
        public:
            Listener()
                : m_bLoadingErrors(false)
                , m_bLoadingWarnings(false)
            {
            }
            virtual void Warning(const char* format)
            {
                RCLogWarning("%s", format);
                m_bLoadingWarnings = true;
            }
            virtual void Error(const char* format)
            {
                RCLogError("%s", format);
                m_bLoadingErrors = true;
            }

        public:
            bool m_bLoadingErrors;
            bool m_bLoadingWarnings;
        };

        if (m_CC.m_pRC->GetVerbosityLevel() > 2)
        {
            RCLog("Loading CGF file %s", sourceFile.c_str());
        }

        Listener listener;
        CChunkFile chunkFile;
        CLoaderCGF cgfLoader;
        CContentCGF* const pCGF = cgfLoader.LoadCGF(sourceFile, chunkFile, &listener);

        if (m_CC.m_pRC->GetVerbosityLevel() > 2)
        {
            RCLog("Loaded CGF file %s", sourceFile.c_str());
        }

        if (!pCGF || listener.m_bLoadingErrors)
        {
            RCLogError("%s: Failed to load geometry file %s: %s", __FUNCTION__, sourceFile.c_str(), cgfLoader.GetLastError());
            delete pCGF;
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        // Validate mesh, but abort compilation if validation fails
        if (m_CC.m_config->GetAsBool("debugvalidate", false, true))
        {
            bool isValid = debugValidateCGF(pCGF, sourceFile.c_str());
            if (!isValid)
            {
                delete pCGF;
                return false;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        const bool bConsole = !m_CC.m_pRC->GetPlatformInfo(m_CC.m_platform)->HasName("pc");
        const bool bNeedEndianSwap = (m_CC.m_pRC->GetPlatformInfo(m_CC.m_platform)->bBigEndian != SYSTEM_IS_BIG_ENDIAN);
        const bool bUseQuaternions = m_CC.m_config->GetAsBool("qtangents", false, true);

        bool bStorePositionsAsF16;
        {
            const char* const optionName = "vertexPositionFormat";
            const string s = m_CC.m_config->GetAsString(optionName, "f32", "f32");

            if (StringHelpers::EqualsIgnoreCase(s, "f32"))
            {
                bStorePositionsAsF16 = false;
            }
            else if (StringHelpers::EqualsIgnoreCase(s, "f16"))
            {
                bStorePositionsAsF16 = true;
            }
            else if (StringHelpers::EqualsIgnoreCase(s, "exporter"))
            {
                bStorePositionsAsF16 = !pCGF->GetExportInfo()->bWantF32Vertices;
            }
            else
            {
                RCLogError("Unknown value of '%s': '%s'. Valid values are: 'f32', 'f16', 'exporter'.", optionName, s.c_str());
                delete pCGF;
                return false;
            }
        }

        bool bStoreIndicesAsU16;
        {
            const char* const optionName = "vertexIndexFormat";
            const string s = m_CC.m_config->GetAsString(optionName, "u32", "u32");

            if (StringHelpers::EqualsIgnoreCase(s, "u32"))
            {
                bStoreIndicesAsU16 = false;
            }
            else if (StringHelpers::EqualsIgnoreCase(s, "u16"))
            {
                bStoreIndicesAsU16 = true;
            }
            else
            {
                RCLogError("Unknown value of '%s': '%s'. Valid values are: 'u32', 'u16'.", optionName, s.c_str());
                delete pCGF;
                return false;
            }
        }

        // Delete Node and Mesh chunks from CGF chunk file.
        {
            if (m_CC.m_pRC->GetVerbosityLevel() > 2)
            {
                RCLog("Deleting old chunks");
            }
            DeleteOldChunks(pCGF, chunkFile);
        }


        CStaticObjectCompiler statCgfCompiler(bConsole, m_CC.m_pRC->GetVerbosityLevel());
        statCgfCompiler.SetSplitLods(bSplitLods);
        // Confetti: Nicholas Baldwin
        statCgfCompiler.SetOptimizeStripify(bOptimizePVRStripify);

        if (m_CC.m_pRC->GetVerbosityLevel() > 2)
        {
            RCLog("Making compilied CGF");
        }

        CContentCGF* const pCompiledCGF = statCgfCompiler.MakeCompiledCGF(pCGF, m_CC.m_bForceRecompiling);
        if (!pCompiledCGF)
        {
            RCLogError(
                "Failed to process geometry file %s. Try to re-export the file. If it not helps - contact an RC programmer.",
                sourceFile.c_str());
            delete pCGF;
            return false;
        }

        // Check that we didn't have internal failures in data processing
        {
            if (m_CC.m_pRC->GetVerbosityLevel() > 2)
            {
                RCLog("Validating meshes");
            }

            const char* pErrorDescription = 0;
            if (!pCompiledCGF->ValidateMeshes(&pErrorDescription))
            {
                RCLogError(
                    "Failed to process geometry in file %s (%s). Try to re-export the file. If it not helps - contact an RC programmer.",
                    sourceFile.c_str(),
                    pErrorDescription);
                delete pCGF;
                return false;
            }
        }

        {
            const SFileVersion& fv = m_CC.m_pRC->GetFileVersion();
            pCompiledCGF->GetExportInfo()->rc_version[0] = fv.v[0];
            pCompiledCGF->GetExportInfo()->rc_version[1] = fv.v[1];
            pCompiledCGF->GetExportInfo()->rc_version[2] = fv.v[2];
            pCompiledCGF->GetExportInfo()->rc_version[3] = fv.v[3];

            StringHelpers::SafeCopyPadZeros(
                pCompiledCGF->GetExportInfo()->rc_version_string,
                sizeof(pCompiledCGF->GetExportInfo()->rc_version_string),
                StringHelpers::Format(" RCVer:%d.%d ", fv.v[2], fv.v[1]).c_str());
        }

        // Write modified content to the output chunk file
        {
            CSaverCGF cgfSaver(chunkFile);
            cgfSaver.SetMeshDataSaving(!bStripMeshData);
            cgfSaver.SetNonMeshDataSaving(!bStripNonMeshData);
            cgfSaver.SetSavePhysicsMeshes(!bStripNonMeshData);
            cgfSaver.SetVertexStreamCompacting(bCompactVertexStreams);
            cgfSaver.SetSubsetTexelDensityComputing(bComputeSubsetTexelDensity);

            if (bStripNonMeshData)
            {
                // Start from a blank slate for stripped cgfs
                chunkFile.Clear();
            }

            cgfSaver.SaveContent(pCompiledCGF, bNeedEndianSwap, bStorePositionsAsF16, bUseQuaternions, bStoreIndicesAsU16);

#if defined(AZ_PLATFORM_WINDOWS)
            SetFileAttributes(outputFile, FILE_ATTRIBUTE_ARCHIVE);
#endif
            
            if (!chunkFile.Write(outputFile))
            {
                RCLogError(
                    "Failed to process geometry file %s: %s. Try to re-export the file. If it not helps - contact an RC programmer.",
                    sourceFile.c_str(), chunkFile.GetLastError());
                delete pCGF;
                return false;
            }

            m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
        }

        // Add base CGF product
        static AZ::Data::AssetType meshAssetType("{C2869E3B-DDA0-4E01-8FE3-6770D788866B}"); // from MeshAsset.h
        {
            // Add our base job product.  
            // This accepts either the full path or the path relative to the Asset Processor temp directory, which is just the filename in this case
            AssetBuilderSDK::JobProduct baseJobProduct(m_CC.m_sourceFileNameOnly.c_str(), meshAssetType, 0); // sub-id 0

            // Add material product dependencies
            {
                CMaterialCGF* commonMaterial = pCGF->GetCommonMaterial();
                if (commonMaterial != nullptr)
                {
                    // Append .mtl to the material name
                    AZStd::string materialName = AZStd::string::format("%s.mtl", commonMaterial->name);

                    // Check if our material name is just a name, or if it contains a path
                    if (strstr(materialName.c_str(), "/") || strstr(materialName.c_str(), "\\"))
                    {
                        // The material path is already relative to devroot, for example "samplesproject/materials/foo.mtl"
                        // We need to convert this to the cache path AP generates.  In this case, cut off the game project name.
                        AZStd::string materialRelativePath = materialName;
                        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, gameFolder);
                        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathRelative, materialRelativePath, gameFolder.c_str());
                        baseJobProduct.m_pathDependencies.emplace(materialRelativePath, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
                    }
                    else
                    {
                        // In this case, we just have the name of the material.  The material is assumed to be in the same directory as the CGF.
                        // Now we need to generate that directory...
                        baseJobProduct.m_pathDependencies.emplace(GetDependencyAbsolutePath(materialName), AssetBuilderSDK::ProductPathDependencyType::SourceFile);
                    }
                }

                baseJobProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            }

            response.m_outputProducts.push_back(baseJobProduct);
        }

        bool bHaveSplitLods = false;
        if (bSplitLods)
        {
            std::vector<string> unusedFiles;

            // Save split LODs
            for (int lodIndex = 1; lodIndex < CStaticObjectCompiler::MAX_LOD_COUNT; ++lodIndex)
            {
                string lodFileName = PathHelpers::RemoveExtension(GetOutputFileNameOnly());
                lodFileName += "_lod";
                lodFileName += '0' + lodIndex;
                lodFileName += '.';
                lodFileName += PathHelpers::FindExtension(outputFile);
                string lodFullPath = PathHelpers::Join(m_CC.GetOutputFolder(), lodFileName);

                CContentCGF* const pLodCgf = statCgfCompiler.m_pLODs[lodIndex];
                if (!pLodCgf)
                {
                    if (FileUtil::FileExists(lodFullPath.c_str()))
                    {
                        unusedFiles.push_back(lodFullPath);
                    }
                    continue;
                }

                // Check that we didn't have internal failures in data processing
                {
                    const char* pErrorDescription = 0;
                    if (!pLodCgf->ValidateMeshes(&pErrorDescription))
                    {
                        RCLogError(
                            "Failed to process geometry of LOD %i in file %s (%s). Try to re-export the file. If it not helps - contact an RC programmer.",
                            lodIndex,
                            sourceFile.c_str(),
                            pErrorDescription);
                        delete pCGF;
                        return false;
                    }
                }

                // Set pParent to 0 for every node (for details read big comment block
                // above in the end of CStaticObjectCompiler::SplitLODs(), above AddNode()
                // call).
                for (int i = 0; i < pCGF->GetNodeCount(); ++i)
                {
                    CNodeCGF* const pNode = pCGF->GetNode(i);
                    pNode->pParent = 0;
                }

                // Save LOD content to the LOD chunk file
                {
                    CChunkFile lodChunkFile;
                    CSaverCGF lodCgfSaver(lodChunkFile);
                    lodCgfSaver.SetMeshDataSaving(!bStripMeshData);
                    lodCgfSaver.SetNonMeshDataSaving(!bStripNonMeshData);
                    lodCgfSaver.SetSavePhysicsMeshes(!bStripNonMeshData);
                    lodCgfSaver.SetVertexStreamCompacting(bCompactVertexStreams);
                    lodCgfSaver.SetSubsetTexelDensityComputing(bComputeSubsetTexelDensity);
                    lodCgfSaver.SaveContent(pLodCgf, bNeedEndianSwap, bStorePositionsAsF16, bUseQuaternions, bStoreIndicesAsU16);
#if defined(AZ_PLATFORM_WINDOWS)
                    SetFileAttributes(lodFullPath, FILE_ATTRIBUTE_ARCHIVE);
#endif
                    lodChunkFile.Write(lodFullPath);

                    // Each LOD writes to a CGF which is an output product
                    static const AZ::Data::AssetType staticMeshLodsAssetType("{9AAE4926-CB6A-4C60-9948-A1A22F51DB23}");
                    AssetBuilderSDK::JobProduct lodJobProduct(lodFileName.c_str(), staticMeshLodsAssetType, lodIndex); // sub-id is the lod index
                    response.m_outputProducts.push_back(lodJobProduct);
                    response.m_outputProducts.at(0).m_pathDependencies.emplace(
                        GetDependencyAbsolutePath(lodFileName.c_str()),
                        AssetBuilderSDK::ProductPathDependencyType::ProductFile);
                    response.m_outputProducts.at(0).m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies

                    m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), lodFullPath);
                }

                bHaveSplitLods = true;
            }

            for (size_t i = 0; i < unusedFiles.size(); ++i)
            {
                m_CC.m_pRC->MarkOutputFileForRemoval(unusedFiles[i]);
            }
        }

        if (!UpToDateFileHelpers::SetMatchingFileTime(GetOutputPath(), m_CC.GetSourcePath()))
        {
            return false;
        }
        
        m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());

        delete pCGF;
    }
#if !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE) // Exception handling not enabled on linux/mac builds
    catch (char*)
    {
        RCLogError("Unexpected failure in processing %s to %s.", sourceFile.c_str(), outputFile.c_str());
        return false;
    }
#endif // !defined(AZ_PLATFORM_LINUX)

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CStatCGFCompiler::DeleteOldChunks(CContentCGF* pCGF, CChunkFile& chunkFile)
{
    for (int i = 0; i < pCGF->GetNodeCount(); ++i)
    {
        CNodeCGF* const pNode = pCGF->GetNode(i);
        if (pNode->nChunkId)
        {
            chunkFile.DeleteChunkById(pNode->nChunkId); // Delete chunk of node.
            // Check if light node, for light nodes we not change light object.
            if (pNode->nObjectChunkId && pNode->type != CNodeCGF::NODE_LIGHT)
            {
                chunkFile.DeleteChunkById(pNode->nObjectChunkId); // Delete chunk of mesh.
            }
            pNode->nObjectChunkId = 0;
        }
    }

    chunkFile.DeleteChunksByType(ChunkType_ExportFlags);
    chunkFile.DeleteChunksByType(ChunkType_MtlName);
    chunkFile.DeleteChunksByType(ChunkType_Mesh);
    chunkFile.DeleteChunksByType(ChunkType_MeshSubsets);
    chunkFile.DeleteChunksByType(ChunkType_DataStream);
    chunkFile.DeleteChunksByType(ChunkType_MeshPhysicsData);
    chunkFile.DeleteChunksByType(ChunkType_BreakablePhysics);
    chunkFile.DeleteChunksByType(ChunkType_FoliageInfo);
    chunkFile.DeleteChunksByType(ChunkType_FaceMap);
    chunkFile.DeleteChunksByType(ChunkType_VertAnim);
    chunkFile.DeleteChunksByType(ChunkType_SceneProps);
}

//////////////////////////////////////////////////////////////////////////
bool CStatCGFCompiler::IsLodFile(const string& filename) const
{
    string file = filename;
    file.MakeLower();
    const char* const s = strstr(file.c_str(), "_lod");
    return (s && s[4] >= '0' && s[4] <= '9' && s[5] == '.');
}

//////////////////////////////////////////////////////////////////////////
bool CStatCGFCompiler::WriteResponse(const char* cacheFolder, AssetBuilderSDK::ProcessJobResponse& response, bool success) const
{
    AZStd::string responseFilePath;
    AzFramework::StringFunc::Path::ConstructFull(cacheFolder, AssetBuilderSDK::s_processJobResponseFileName, responseFilePath);

    response.m_requiresSubIdGeneration = false;
    response.m_resultCode = success ? AssetBuilderSDK::ProcessJobResult_Success : AssetBuilderSDK::ProcessJobResult_Failed;

    bool result = AZ::Utils::SaveObjectToFile(responseFilePath, AZ::DataStream::StreamType::ST_XML, &response);

    if (!result)
    {
        AZ_Error("CStatCGFCompiler", false, "Unable to save ProcessJobResponse file to %s.\n", responseFilePath.c_str());
    }

    return result && success;
}
