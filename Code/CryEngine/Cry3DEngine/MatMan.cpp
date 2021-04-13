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

// Description : Material Manager Implementation


#include "Cry3DEngine_precompiled.h"
#include "MatMan.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include "IRenderer.h"
#include "SurfaceTypeManager.h"
#include "CGFContent.h"
#include <CryPath.h>
#include <IResourceManager.h>
#include "MaterialUtils.h" // from crycommon
#include <Pak/CryPakUtils.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#define MATERIAL_EXT ".mtl"
#define MATERIAL_NODRAW "nodraw"

#define MATERIAL_DECALS_FOLDER "Materials/Decals"
#define MATERIAL_DECALS_SEARCH_WILDCARD "*.mtl"
#define MTL_LEVEL_CACHE_PAK "mtl.pak"


//////////////////////////////////////////////////////////////////////////
struct MaterialHelpers CMatMan::s_materialHelpers;

int CMatMan::e_sketch_mode = 0;
int CMatMan::e_pre_sketch_spec = 0;
int CMatMan::e_texeldensity = 0;

//------------------------------------------------------------------------------
// Default textures declarations
//------------------------------------------------------------------------------
#if !defined(_RELEASE)
    // Texture names to be used for error / process loading indications
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMe.tif";
    static const char* szTextureCompiling = "EngineAssets/TextureMsg/TextureCompiling.tif";
    static const char* szShaderCompiling = "EngineAssets/TextureMsg/ShaderCompiling.tif";
    static const char* szGeomNotBreakable = "EngineAssets/TextureMsg/GeomNotBreakable.tif";
#else
    // Some of the textures here will direct to the regular DefaultSolids_diff to prevent eye catching
    // bug textures in release mode
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMeRelease.tif";
    static const char* szTextureCompiling = "EngineAssets/TextureMsg/DefaultSolids_diff.tif";
    static const char* szShaderCompiling = "EngineAssets/TextureMsg/DefaultSolids_diff.tif";
    static const char* szGeomNotBreakable = "EngineAssets/TextureMsg/ReplaceMeRelease.tif";
#endif

static const char* szDefaultSolid = "EngineAssets/TextureMsg/DefaultSolids_diff.tif";
static const char* szNormalDefault = "EngineAssets/Textures/white_ddn.dds";
//------------------------------------------------------------------------------


static void OnSketchModeChange(ICVar* pVar)
{
    int mode = pVar->GetIVal();
    ((CMatMan*)gEnv->p3DEngine->GetMaterialManager())->SetSketchMode(mode);
}

static void OnDebugTexelDensityChange(ICVar* pVar)
{
    int mode = pVar->GetIVal();
    ((CMatMan*)gEnv->p3DEngine->GetMaterialManager())->SetTexelDensityDebug(mode);
}

//////////////////////////////////////////////////////////////////////////
CMatMan::CMatMan()
{
    m_bInitialized = false;
    m_bLoadSurfaceTypesInInit = true;
    m_pListener = NULL;
    m_pDefaultMtl = NULL;
    m_pDefaultTerrainLayersMtl = NULL;
    m_pDefaultLayersMtl = NULL;
    m_pDefaultHelperMtl = NULL;
    m_pNoDrawMtl = NULL;

    m_pSurfaceTypeManager = new CSurfaceTypeManager(GetSystem());

    REGISTER_CVAR_CB(e_sketch_mode, 0, VF_CHEAT, "Enables Sketch mode drawing", OnSketchModeChange);
    REGISTER_CVAR_CB(e_texeldensity, 0, VF_CHEAT,
        "Enables texel density debug\n"
        " 1: Objects texel density\n"
        " 2: Objects texel density with colored mipmaps\n"
        " 3: Terrain texel density\n"
        " 4: Terrain texel density with colored mipmaps\n",
        OnDebugTexelDensityChange);

    m_pXmlParser = GetISystem()->GetXmlUtils()->CreateXmlParser();

    // Connect for LegacyAssetEventBus::Handler
    BusConnect(AZ_CRC("mtl", 0xb01910e0));
}

//////////////////////////////////////////////////////////////////////////
CMatMan::~CMatMan()
{
    delete m_pSurfaceTypeManager;
    int nNotUsed = 0, nNotUsedParents = 0;

    m_pDefaultMtl = nullptr;
    m_pDefaultTerrainLayersMtl = nullptr;
    m_pDefaultLayersMtl = nullptr;
    m_pDefaultHelperMtl = nullptr;
    m_pNoDrawMtl = nullptr;

    if (nNotUsed)
    {
        PrintMessage("Warning: CMatMan::~CMatMan: %d(%d) of %" PRISIZE_T " materials was not used in level",
            nNotUsedParents, nNotUsed, m_mtlNameMap.size());
    }


    // Disconnect for LegacyAssetEventBus::Handler
    BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CMatMan::UnifyName(const char* sMtlName) const
{
    if (strlen(sMtlName) > AZ_MAX_PATH_LEN)
    {
        AZ_Error("Rendering", false, "Error attempting to generate material identifier from the input '%s'. The length of the string exceeds the maximum path length. If you are using script canvas or lua to find or load a material, ensure you are using a valid path to a material as input.", sMtlName);
        return AZStd::string();
    }

    char name[AZ_MAX_PATH_LEN];
    
    azstrcpy(name, AZ_MAX_PATH_LEN, sMtlName);
    MaterialUtils::UnifyMaterialName(name);
    
    return name;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::CreateMaterial(const char* sMtlName, int nMtlFlags)
{
    CMatInfo* pMat = new CMatInfo;

    //m_mtlSet.insert( pMat );
    pMat->SetName(sMtlName);
    pMat->SetFlags(nMtlFlags | pMat->GetFlags());

    if (!(nMtlFlags & MTL_FLAG_PURE_CHILD))
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
        m_mtlNameMap[ UnifyName(sMtlName) ] = pMat;
    }

    if (nMtlFlags & MTL_FLAG_NON_REMOVABLE)
    {
        // Add reference to this material to prevent its deletion.
        AZStd::lock_guard<AZStd::mutex> lock(m_nonRemovablesMutex);
        m_nonRemovables.push_back(pMat);
    }
    return pMat;
}

//////////////////////////////////////////////////////////////////////////
// A placeholder material while the original is loading.
// Add more edge case handling for various material types if required
_smart_ptr<IMaterial> CMatMan::CreateMaterialPlaceholder(const char* materialName, int nMtlFlags, const char* textureName, _smart_ptr<IMaterial> existingMtl)
{
    SInputShaderResources   sr;
    SShaderItem             si;

    sr.m_LMaterial.m_Opacity = 1;
    sr.m_LMaterial.m_Diffuse.set(1, 1, 1, 1);
    sr.m_LMaterial.m_Specular.set(0, 0, 0, 0);

    // This will create texture data insertion to the table for the diffuse slot
    sr.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name = textureName;

    if (nMtlFlags & MTL_FLAG_IS_TERRAIN)
        si = GetRenderer()->EF_LoadShaderItem("Terrain.Layer", true, 0, &sr);
    else if (nMtlFlags & MTL_FLAG_IS_SKY)
        si = GetRenderer()->EF_LoadShaderItem("SkyHDR", true, 0, &sr);
    else
        si = GetRenderer()->EF_LoadShaderItem("Illum", true, 0, &sr);

    if (si.m_pShaderResources)
        si.m_pShaderResources->SetMaterialName(materialName);

    if (existingMtl)
    {
        //For existing material we need to clear sub-materials, set flags and assign the new shader item.
        existingMtl->SetSubMtlCount(0);
        existingMtl->SetFlags(nMtlFlags);
        existingMtl->AssignShaderItem(si);

        //Note: All PURE_CHILD materials are sub-materials, but not all sub-materials are PURE_CHILD.
        //You can have one sub-material that shares some properties with another sub-material in the same parent, and overrides other properties. A sub-material that does this is not a pure child.
        //But since it is seldom used, this should cover most cases when we want to know whether the material is a root.
        if (!(nMtlFlags & MTL_FLAG_PURE_CHILD))
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
            m_mtlNameMap[UnifyName(materialName)] = existingMtl;
        }

        return existingMtl;
    }
    else
    {
        _smart_ptr<IMaterial>   pMtl = CreateMaterial(materialName);
        pMtl->AssignShaderItem(si);
        return pMtl;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::NotifyCreateMaterial(_smart_ptr<IMaterial> pMtl)
{
    if (m_pListener)
    {
        m_pListener->OnCreateMaterial(pMtl);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CMatMan::Unregister(_smart_ptr<IMaterial> pMat, bool deleteEditorMaterial)
{
    assert(pMat);
    if (m_pListener && deleteEditorMaterial)
    {
        m_pListener->OnDeleteMaterial(pMat);
    }

    if (!(pMat->GetFlags() & MTL_FLAG_PURE_CHILD))
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
        
        AZStd::string unifiedName = UnifyName(pMat->GetName());
        m_pendingMaterialLoads.erase(unifiedName);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::RenameMaterial(_smart_ptr<IMaterial> pMtl, const char* sNewName)
{
    assert(pMtl);
    
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
    const char* sName = pMtl->GetName();
    AZStd::unique_ptr<ManualResetEvent> resetEvent = nullptr;

    if (*sName != '\0')
    {
        AZStd::string unifiedName = UnifyName(pMtl->GetName());
        
        if (m_pendingMaterialLoads.find(unifiedName) != m_pendingMaterialLoads.end())
        {
            resetEvent = std::move(m_pendingMaterialLoads[unifiedName]);
            m_pendingMaterialLoads.erase(unifiedName);
        }

        m_mtlNameMap.erase(unifiedName);
    }

    pMtl->SetName(sNewName);
    AZStd::string newUnifiedName = UnifyName(sNewName);

    m_mtlNameMap[newUnifiedName] = pMtl;
    if (resetEvent != nullptr)
    {
        m_pendingMaterialLoads[newUnifiedName] = std::move(resetEvent);
    }

}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::FindMaterial(const char* sMtlName) const
{
    AZStd::string name = UnifyName(sMtlName);
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);

    MtlNameMap::const_iterator it = m_mtlNameMap.find(name);

    if (it == m_mtlNameMap.end())
    {
        return 0;
    }

    return it->second;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
_smart_ptr<IMaterial> CMatMan::LoadMaterial(const char* sMtlName, bool bMakeIfNotFound, bool bNonremovable, unsigned long nLoadingFlags)
{
    return LoadMaterialInternal(sMtlName, bMakeIfNotFound, bNonremovable, nLoadingFlags);
}



////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
_smart_ptr<IMaterial> CMatMan::LoadMaterialInternal(const char* sMtlName, bool bMakeIfNotFound, bool bNonremovable, unsigned long nLoadingFlags)
{
    if (!m_bInitialized)
    {
        InitDefaults();
    }

    if (m_pDefaultMtl && GetCVars()->e_StatObjPreload == 2)
    {
        return m_pDefaultMtl;
    }

    AZStd::string           name = UnifyName(sMtlName);
    _smart_ptr<IMaterial>   pMtl = nullptr;

    UniqueManualEvent       uniqueManualEvent = CheckMaterialCache(name, pMtl);

    if (pMtl != nullptr)
    {
        return pMtl;
    }

    // Failed to retrieve from cache and failed to get 'permission' to safely load, abort load
    if (!uniqueManualEvent.HasControl())
    {
        if (bMakeIfNotFound)
        {
            pMtl = CreateMaterialPlaceholder(name.c_str(), nLoadingFlags, szDefaultSolid);
            return pMtl;
        }
        return nullptr;
    }

    LOADING_TIME_PROFILE_SECTION; // Only profile actually loading of the material.

    CRY_DEFINE_ASSET_SCOPE("Material", sMtlName);

    _smart_ptr<IMaterial> materialRawPointer = pMtl;

    AZStd::string filename = name;
    auto extPos = filename.find('.');

    if (extPos == AZStd::string::npos)
    {
        filename += MATERIAL_EXT;
    }

    
    bool fileExists = AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str());
    if (!fileExists)
    {
        // If the material doesn't exist check if it's queued or being compiled. If so it means the file will become available shortly (as
        //      GetAssetStatus will push it to the top of the queue) and hot loading will take care of the file. If it's in a broken state,
        //      remove it as if loading failed.
        AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
        AzFramework::AssetSystemRequestBus::BroadcastResult(status, &AzFramework::AssetSystemRequestBus::Events::GetAssetStatus, filename);

        switch (status)
        {
        case AzFramework::AssetSystem::AssetStatus_Queued:
            // Fall through
        case AzFramework::AssetSystem::AssetStatus_Compiling:
        {
            AZStd::string   unifiedName = UnifyName(filename.c_str());
            pMtl = CreateMaterialPlaceholder(unifiedName.c_str(), nLoadingFlags, szDefaultSolid );
            break;
        }

        case AzFramework::AssetSystem::AssetStatus_Compiled:
            // If the materials compiled it could be that between the check if it exists and getting the status it completed compilation.
            //      In this case, check the status again and load as normal if found, otherwise consider it an error.
            if (AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str()))
            {
                fileExists = true;
                break;
            }
            // else fall through

        case AzFramework::AssetSystem::AssetStatus_Unknown:
            // Fall through
        case AzFramework::AssetSystem::AssetStatus_Missing:
            // Fall through
        case AzFramework::AssetSystem::AssetStatus_Failed:
            // Fall through
        default:
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
            uniqueManualEvent.Set();
            m_pendingMaterialLoads.erase(name);
            break;
        }
        }
    }
    
    if (fileExists)
    {
        // If the material already exists load it from the cache. If there's a build in flight the material will get reloaded
        //      when building finishes and if it's not in flight anymore the latest material will be loaded.
        XmlNodeRef mtlNode = GetSystem()->LoadXmlFromFile(filename.c_str());

        if (mtlNode)
        {
            pMtl = MakeMaterialFromXml(name, mtlNode, false, 0, nullptr, nLoadingFlags);

            if (pMtl && e_sketch_mode != 0)
            {
                ((CMatInfo*)pMtl.get())->SetSketchMode(e_sketch_mode);
            }
        }
        else
        {
            //Loading has failed so evict from pending list.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
            uniqueManualEvent.Set();
            m_pendingMaterialLoads.erase(name);
        }
    }

    if (bNonremovable && pMtl)
    {   // mark as non-removable material on specific cases (probes..)
        AZStd::lock_guard<AZStd::mutex> lock(m_nonRemovablesMutex);
        m_nonRemovables.push_back((CMatInfo*)pMtl.get());
    }

    if (!pMtl && bMakeIfNotFound)
    {
        pMtl = CreateMaterialPlaceholder(name.c_str(), nLoadingFlags, szDefaultSolid);
    }

    return pMtl;
}

//! Let the first thread load the material, block the rest until its done so they can just use the cached version
template<typename T>
UniqueManualEvent CMatMan::CheckMaterialCache(const AZStd::string& name, T& cachedMaterial)
{
    bool hasControl = false;
    ManualResetEvent* manualResetEvent;

    AZStd::unique_lock<AZStd::recursive_mutex> lock(m_materialMapMutex);

    auto iterator = m_pendingMaterialLoads.find(name);

    if (iterator != m_pendingMaterialLoads.end())
    {
        manualResetEvent = iterator->second.get();
    }
    else
    {
        // Event not found, create one
        hasControl = true;
        manualResetEvent = new ManualResetEvent();
        m_pendingMaterialLoads.emplace(name, AZStd::unique_ptr<ManualResetEvent>(manualResetEvent));
    }

    if (!hasControl)
    {
        manualResetEvent->Wait();

        MtlNameMap::const_iterator it = m_mtlNameMap.find(name);

        if (it != m_mtlNameMap.end())
        {
            cachedMaterial = it->second;
        }
        else
        {
            cachedMaterial = nullptr;
        }
    }

    return UniqueManualEvent(manualResetEvent, hasControl);
}


//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::MakeMaterialFromXml(const AZStd::string& sMtlName, XmlNodeRef node, bool bForcePureChild, uint16 sortPrio /*= 0*/, _smart_ptr<IMaterial> pExistingMtl /*= 0*/, unsigned long nLoadingFlags /*= 0*/, _smart_ptr<IMaterial> pParentMtl /*= 0*/)
{
    int mtlFlags = 0;
    CryFixedStringT<128> shaderName;
    uint64 nShaderGenMask = 0;
    SInputShaderResources sr;

    assert(node != 0);

    sr.m_SortPrio = sortPrio;

    // Loading
    node->getAttr("MtlFlags", mtlFlags);
    mtlFlags &= (MTL_FLAGS_SAVE_MASK); // Clean flags that are not supposed to be save/loaded.
    if (bForcePureChild)
    {
        mtlFlags |= MTL_FLAG_PURE_CHILD;
    }

    _smart_ptr<IMaterial> pMtl = pExistingMtl;
    if (!pMtl)
    {
        pMtl = CreateMaterial(sMtlName.c_str(), mtlFlags);
    }
    else
    {
        pMtl->SetFlags(mtlFlags | pMtl->GetFlags());
        pMtl->SetDirty(false);
    }

    uint32 dccMaterialHash = 0;
    node->getAttr("DccMaterialHash", dccMaterialHash);
    pMtl->SetDccMaterialHash(dccMaterialHash);

    if (!(mtlFlags & MTL_FLAG_MULTI_SUBMTL))
    {
        shaderName = node->getAttr("Shader");

        if (!(mtlFlags & MTL_64BIT_SHADERGENMASK))
        {
            uint32 nShaderGenMask32 = 0;
            node->getAttr("GenMask", nShaderGenMask32);
            nShaderGenMask = nShaderGenMask32;

            // Remap 32bit flags to 64 bit version
            nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen((const char*) shaderName, nShaderGenMask);
            mtlFlags |= MTL_64BIT_SHADERGENMASK;
        }
        else
        {
            node->getAttr("GenMask", nShaderGenMask);
        }

        if (node->haveAttr("StringGenMask"))
        {
            const char* pszShaderGenMask = node->getAttr("StringGenMask");
            nShaderGenMask = GetRenderer()->EF_GetShaderGlobalMaskGenFromString((const char*) shaderName, pszShaderGenMask, nShaderGenMask); // get common mask gen
        }
        else
        {
            // version doesn't have string gen mask yet ? Remap flags if needed
            nShaderGenMask = GetRenderer()->EF_GetRemapedShaderMaskGen((const char*) shaderName, nShaderGenMask, ((mtlFlags & MTL_64BIT_SHADERGENMASK) != 0));
        }
        mtlFlags |= MTL_64BIT_SHADERGENMASK;

        const char* surfaceType = node->getAttr("SurfaceType");
        pMtl->SetSurfaceType(surfaceType);

        if (azstricmp(shaderName, "nodraw") == 0)
        {
            mtlFlags |= MTL_FLAG_NODRAW;
        }

        pMtl->SetFlags(mtlFlags | pMtl->GetFlags());

        s_materialHelpers.SetLightingFromXml(sr, node);
        s_materialHelpers.SetTexturesFromXml(sr, node);
        s_materialHelpers.MigrateXmlLegacyData(sr, node);

        // Next warn about textures with drive letter in them
        for (auto &iter : sr.m_TexturesResourcesMap )
        {
            SEfResTexture*      pTexture = &(iter.second);
            const char*         name = pTexture->m_Name;
            if (name && (strchr(name, ':') != nullptr))
            {
                CryLog("Invalid texture '%s' found in material '%s'", name, sMtlName.c_str());
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if we have a link name
    //////////////////////////////////////////////////////////////////////////
    XmlNodeRef pLinkName = node->findChild("MaterialLinkName");
    if (pLinkName)
    {
        const char* szLinkName = pLinkName->getAttr("name");
        pMtl->SetMaterialLinkName(szLinkName);
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if we have vertex deform.
    //////////////////////////////////////////////////////////////////////////
    s_materialHelpers.SetVertexDeformFromXml(sr, node);

    //////////////////////////////////////////////////////////////////////////
    // Load public parameters.
    XmlNodeRef publicVarsNode = node->findChild("PublicParams");

    //////////////////////////////////////////////////////////////////////////
    // Reload shader item with new resources and shader.
    if (!(mtlFlags & MTL_FLAG_MULTI_SUBMTL))
    {
        sr.m_szMaterialName = sMtlName.c_str();
        
        LoadMaterialShader(pMtl, pParentMtl, shaderName.c_str(), nShaderGenMask, sr, publicVarsNode);
        pMtl->SetShaderName(shaderName);
    }
    else
    {
        //Release any shader item if assigned for material group
        pMtl->ReleaseCurrentShaderItem();
    }

    //////////////////////////////////////////////////////////////////////////
    // Load material layers data
    //////////////////////////////////////////////////////////////////////////

    if (pMtl && pMtl->GetShaderItem().m_pShader && pMtl->GetShaderItem().m_pShaderResources)
    {
        XmlNodeRef pMtlLayersNode = node->findChild("MaterialLayers");
        if (pMtlLayersNode)
        {
            int nLayerCount = min((int) MTL_LAYER_MAX_SLOTS, (int) pMtlLayersNode->getChildCount());
            if (nLayerCount)
            {
                uint8 nMaterialLayerFlags = 0;

                pMtl->SetLayerCount(nLayerCount);
                for (int l(0); l < nLayerCount; ++l)
                {
                    XmlNodeRef pLayerNode = pMtlLayersNode->getChild(l);
                    if (pLayerNode)
                    {
                        if (const char* pszShaderName = pLayerNode->getAttr("Name"))
                        {
                            bool bNoDraw = false;
                            pLayerNode->getAttr("NoDraw", bNoDraw);

                            uint8 nLayerFlags = 0;
                            if (bNoDraw)
                            {
                                nLayerFlags |= MTL_LAYER_USAGE_NODRAW;

                                if (!azstricmp(pszShaderName, "frozenlayerwip"))
                                {
                                    nMaterialLayerFlags |= MTL_LAYER_FROZEN;
                                }
                            }
                            else
                            {
                                nLayerFlags &= ~MTL_LAYER_USAGE_NODRAW;
                            }

                            bool bFadeOut = false;
                            pLayerNode->getAttr("FadeOut", bFadeOut);
                            if (bFadeOut)
                            {
                                nLayerFlags |= MTL_LAYER_USAGE_FADEOUT;
                            }
                            else
                            {
                                nLayerFlags &= ~MTL_LAYER_USAGE_FADEOUT;
                            }

                            XmlNodeRef pPublicsParamsNode = pLayerNode->findChild("PublicParams");
                            sr.m_szMaterialName = sMtlName.c_str();
                            LoadMaterialLayerSlot(l, pMtl, pszShaderName, sr, pPublicsParamsNode, nLayerFlags);
                        }
                    }
                }

                SShaderItem& pShaderItemBase = pMtl->GetShaderItem();
                if (pShaderItemBase.m_pShaderResources)
                {
                    pShaderItemBase.m_pShaderResources->SetMtlLayerNoDrawFlags(nMaterialLayerFlags);
                }
            }
        }
    }

    // Serialize sub materials.
    XmlNodeRef childsNode = node->findChild("SubMaterials");
    if (childsNode)
    {
        int nSubMtls = childsNode->getChildCount();
        pMtl->SetSubMtlCount(nSubMtls);
        for (int i = 0; i < nSubMtls; i++)
        {
            XmlNodeRef mtlNode = childsNode->getChild(i);
            if (mtlNode->isTag("Material"))
            {
                const char* name = mtlNode->getAttr("Name");
                _smart_ptr<IMaterial> pChildMtl = MakeMaterialFromXml(name, mtlNode, true, uint16(nSubMtls - i - 1), 0, nLoadingFlags, pMtl);
                if (pChildMtl)
                {
                    pMtl->SetSubMtl(i, pChildMtl);
                }
                else
                {
                    pMtl->SetSubMtl(i, m_pDefaultMtl);
                }
            }
            else
            {
                const char* name = mtlNode->getAttr("Name");
                if (name[0])
                {
                    _smart_ptr<IMaterial> pChildMtl = LoadMaterial(name, true, false, nLoadingFlags);
                    if (pChildMtl)
                    {
                        pMtl->SetSubMtl(i, pChildMtl);
                    }
                }
            }
        }
    }
    NotifyCreateMaterial(pMtl);
    return pMtl;
}

//////////////////////////////////////////////////////////////////////////
bool CMatMan::LoadMaterialShader(_smart_ptr<IMaterial> pMtl, _smart_ptr<IMaterial> pParentMtl, const char* sShader, uint64 nShaderGenMask, SInputShaderResources& sr, XmlNodeRef& publicsNode)
{
    // Mark material invalid by default.
    sr.m_ResFlags = pMtl->GetFlags();

    // Set public params.
    if (publicsNode)
    {
        // Copy public params from the shader.
        //sr.m_ShaderParams = shaderItem.m_pShader->GetPublicParams();
        // Parse public parameters, and assign them to source shader resources.
        ParsePublicParams(sr, publicsNode);
        //shaderItem.m_pShaderResources->SetShaderParams(&sr, shaderItem.m_pShader);
    }

    SShaderItem shaderItem = gEnv->pRenderer->EF_LoadShaderItem(sShader, false, 0, &sr, nShaderGenMask);
    if (!shaderItem.m_pShader || (shaderItem.m_pShader->GetFlags() & EF_NOTFOUND) != 0)
    {
        Warning("Failed to load shader \"%s\" in material \"%s\"", sShader, pMtl->GetName());
        if (!shaderItem.m_pShader)
        {
            return false;
        }
    }
    pMtl->AssignShaderItem(shaderItem);

    return true;
}

bool CMatMan::LoadMaterialLayerSlot(uint32 nSlot, _smart_ptr<IMaterial> pMtl, const char* szShaderName, SInputShaderResources& pBaseResources, XmlNodeRef& pPublicsNode, uint8 nLayerFlags)
{
    if (!pMtl || pMtl->GetLayer(nSlot) || !pPublicsNode)
    {
        return false;
    }

    // need to handle no draw case
    if (azstricmp(szShaderName, "nodraw") == 0)
    {
        // no shader = skip layer
        return false;
    }

    // Get base material/shaderItem info
    SInputShaderResources       pInputResources;
    SShaderItem& pShaderItemBase = pMtl->GetShaderItem();

    uint32 nMaskGenBase = (uint32)pShaderItemBase.m_pShader->GetGenerationMask();
    SShaderGen* pShaderGenBase = pShaderItemBase.m_pShader->GetGenerationParams();

    // copy diffuse and bump textures names
    pInputResources.m_szMaterialName = pBaseResources.m_szMaterialName;

    // The following copies the entire texture data for this slot as it did not exist in the map
    if (pBaseResources.GetTextureResource(EFTT_DIFFUSE))
    {
        pInputResources.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name = pBaseResources.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name;
    }

    if (pBaseResources.GetTextureResource(EFTT_NORMALS))
    {
        pInputResources.m_TexturesResourcesMap[EFTT_NORMALS].m_Name = pBaseResources.m_TexturesResourcesMap[EFTT_NORMALS].m_Name;
    }

    // Names validity - if the texture slot doesn't exist or no name replace with default textures
    if (pInputResources.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name.empty())
    {
        pInputResources.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name = szReplaceMe;
    }

    if (pInputResources.m_TexturesResourcesMap[EFTT_NORMALS].m_Name.empty())
    {
        pInputResources.m_TexturesResourcesMap[EFTT_NORMALS].m_Name = szNormalDefault;
    }
    // Load layer shader item
    IShader* pNewShader = gEnv->pRenderer->EF_LoadShader(szShaderName, 0);
    if (!pNewShader)
    {
        Warning("Failed to load material layer shader %s in Material %s", szShaderName, pMtl->GetName());
        return false;
    }

    // mask generation for base material shader
    uint32 nMaskGenLayer = 0;
    SShaderGen* pShaderGenLayer = pNewShader->GetGenerationParams();
    if (pShaderGenBase && pShaderGenLayer)
    {
        for (unsigned nLayerBit(0); nLayerBit < pShaderGenLayer->m_BitMask.size(); ++nLayerBit)
        {
            SShaderGenBit* pLayerBit = pShaderGenLayer->m_BitMask[nLayerBit];

            for (unsigned nBaseBit(0); nBaseBit < pShaderGenBase->m_BitMask.size(); ++nBaseBit)
            {
                SShaderGenBit* pBaseBit = pShaderGenBase->m_BitMask[nBaseBit];

                // Need to check if flag name is common to both shaders (since flags values can be different), if so activate it on this layer
                if (nMaskGenBase & pBaseBit->m_Mask)
                {
                    if (!pLayerBit->m_ParamName.empty() && !pBaseBit->m_ParamName.empty())
                    {
                        if (pLayerBit->m_ParamName == pBaseBit->m_ParamName)
                        {
                            nMaskGenLayer |= pLayerBit->m_Mask;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Reload with proper flags
    IShader* pShader = gEnv->pRenderer->EF_LoadShader(szShaderName, 0, nMaskGenLayer);
    if (!pShader)
    {
        Warning("Failed to load material layer shader %s in Material %s", szShaderName, pMtl->GetName());
        SAFE_RELEASE(pNewShader);
        return false;
    }
    SAFE_RELEASE(pNewShader);

    // Copy public params from the shader.
    //pInputResources.m_ShaderParams = pShaderItem.m_pShader->GetPublicParams();

    // Copy resources from base material
    SShaderItem ShaderItem(pShader, pShaderItemBase.m_pShaderResources->Clone());

    ParsePublicParams(pInputResources, pPublicsNode);

    // Parse public parameters, and assign them to source shader resources.
    ShaderItem.m_pShaderResources->SetShaderParams(&pInputResources, ShaderItem.m_pShader);

    IMaterialLayer* pCurrMtlLayer = pMtl->CreateLayer();

    pCurrMtlLayer->SetFlags(nLayerFlags);
    pCurrMtlLayer->SetShaderItem(pMtl, ShaderItem);

    // Clone returns an instance with a refcount of 1, and SetShaderItem increments it, so
    // we need to release the cloned ref.
    SAFE_RELEASE(ShaderItem.m_pShaderResources);
    SAFE_RELEASE(ShaderItem.m_pShader);

    pMtl->SetLayer(nSlot, pCurrMtlLayer);

    return true;
}

//////////////////////////////////////////////////////////////////////////

static void shGetVector4(const char* buf, float v[4])
{
    if (!buf)
    {
        return;
    }
#if !defined(NDEBUG)
    int res =
#endif
        azsscanf(buf, "%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3]);
    assert(res);
}

void CMatMan::ParsePublicParams(SInputShaderResources& sr, XmlNodeRef paramsNode)
{
    sr.m_ShaderParams.clear();

    int nA = paramsNode->getNumAttributes();
    if (!nA)
    {
        return;
    }

    for (int i = 0; i < nA; i++)
    {
        const char* key = NULL, * val = NULL;
        paramsNode->getAttributeByIndex(i, &key, &val);
        SShaderParam Param;
        assert(val && key);
        Param.m_Name = key;
        Param.m_Value.m_Color[0] = Param.m_Value.m_Color[1] = Param.m_Value.m_Color[2] = Param.m_Value.m_Color[3] = 0;
        shGetVector4(val, Param.m_Value.m_Color);
        Param.m_Type = eType_FCOLOR;
        sr.m_ShaderParams.push_back(Param);
    }
}

//////////////////////////////////////////////////////////////////////////
ISurfaceType* CMatMan::GetSurfaceTypeByName(const char* sSurfaceTypeName, const char* sWhy)
{
    return m_pSurfaceTypeManager->GetSurfaceTypeByName(sSurfaceTypeName, sWhy);
};

//////////////////////////////////////////////////////////////////////////
int CMatMan::GetSurfaceTypeIdByName(const char* sSurfaceTypeName, const char* sWhy)
{
    ISurfaceType* pSurfaceType = m_pSurfaceTypeManager->GetSurfaceTypeByName(sSurfaceTypeName, sWhy);
    if (pSurfaceType)
    {
        return pSurfaceType->GetId();
    }
    return 0;
};

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::GetDefaultLayersMaterial()
{
    if (!m_bInitialized)
    {
        InitDefaults();
    }

    return m_pDefaultLayersMtl;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::GetDefaultHelperMaterial()
{
    if (!m_bInitialized)
    {
        InitDefaults();
    }

    return m_pDefaultHelperMtl;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::GetLoadedMaterials(AZStd::vector<_smart_ptr<IMaterial>>* pData, uint32& nObjCount) const
{
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);
    nObjCount = m_mtlNameMap.size();

    if (!pData)
    {
        return;
    }

    MtlNameMap::const_iterator it, end = m_mtlNameMap.end();

    for (it = m_mtlNameMap.begin(); it != end; ++it)
    {
        _smart_ptr<IMaterial> pMat = it->second;

        pData->push_back(pMat);
    }
}


//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::CloneMaterial(_smart_ptr<IMaterial> pSrcMtl, int nSubMtl)
{
    if (pSrcMtl->GetFlags() & MTL_FLAG_MULTI_SUBMTL)
    {
        _smart_ptr<IMaterial> pMultiMat = new CMatInfo;

        //m_mtlSet.insert( pMat );
        pMultiMat->SetName(pSrcMtl->GetName());
        pMultiMat->SetFlags(pMultiMat->GetFlags() | MTL_FLAG_MULTI_SUBMTL);

        bool bCloneAllSubMtls = nSubMtl < 0;

        int nSubMtls = pSrcMtl->GetSubMtlCount();
        pMultiMat->SetSubMtlCount(nSubMtls);
        for (int i = 0; i < nSubMtls; i++)
        {
            CMatInfo* pChildSrcMtl = (CMatInfo*)pSrcMtl->GetSubMtl(i).get();
            if (!pChildSrcMtl)
            {
                continue;
            }
            if (bCloneAllSubMtls)
            {
                pMultiMat->SetSubMtl(i, pChildSrcMtl->Clone());
            }
            else
            {
                pMultiMat->SetSubMtl(i, pChildSrcMtl);
                if (i == nSubMtls)
                {
                    // Clone this slot.
                    pMultiMat->SetSubMtl(i, pChildSrcMtl->Clone());
                }
            }
        }
        return pMultiMat;
    }
    else
    {
        _smart_ptr<IMaterial> pMat = reinterpret_cast<CMatInfo*>(pSrcMtl.get())->Clone();
        return pMat;
    }
}

void CMatMan::CopyMaterial(_smart_ptr<IMaterial> pMtlSrc, _smart_ptr<IMaterial> pMtlDest, EMaterialCopyFlags flags)
{
    ((CMatInfo*)pMtlSrc.get())->Copy(pMtlDest, flags);
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::CloneMultiMaterial(_smart_ptr<IMaterial> pSrcMtl, const char* sSubMtlName)
{
    if (pSrcMtl->GetFlags() & MTL_FLAG_MULTI_SUBMTL)
    {
        _smart_ptr<IMaterial> pMultiMat = new CMatInfo;

        //m_mtlSet.insert( pMat );
        pMultiMat->SetName(pSrcMtl->GetName());
        pMultiMat->SetFlags(pMultiMat->GetFlags() | MTL_FLAG_MULTI_SUBMTL);

        bool bCloneAllSubMtls = sSubMtlName == 0;

        int nSubMtls = pSrcMtl->GetSubMtlCount();
        pMultiMat->SetSubMtlCount(nSubMtls);
        for (int i = 0; i < nSubMtls; i++)
        {
            CMatInfo* pChildSrcMtl = (CMatInfo*)pSrcMtl->GetSubMtl(i).get();
            if (!pChildSrcMtl)
            {
                continue;
            }
            if (bCloneAllSubMtls)
            {
                pMultiMat->SetSubMtl(i, pChildSrcMtl->Clone());
            }
            else
            {
                pMultiMat->SetSubMtl(i, pChildSrcMtl);
                if (azstricmp(pChildSrcMtl->GetName(), sSubMtlName) == 0)
                {
                    // Clone this slot.
                    pMultiMat->SetSubMtl(i, pChildSrcMtl->Clone());
                }
            }
        }
        return pMultiMat;
    }
    else
    {
        return ((CMatInfo*)pSrcMtl.get())->Clone();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::DoLoadSurfaceTypesInInit(bool doLoadSurfaceTypesInInit)
{
    m_bLoadSurfaceTypesInInit = doLoadSurfaceTypesInInit;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::InitDefaults()
{
    if (m_bInitialized)
    {
        return;
    }
    m_bInitialized = true;

    LOADING_TIME_PROFILE_SECTION;

    SYNCHRONOUS_LOADING_TICK();

    if (m_bLoadSurfaceTypesInInit)
    {
        m_pSurfaceTypeManager->LoadSurfaceTypes();
    }

    if (!m_pDefaultMtl)
    {
        // This line is REQUIRED by the buildbot testing framework to determine when tests have formally started. Please inform WillW or Morgan before changing this.
        CryLogAlways("Initializing default materials...");
        m_pDefaultMtl = CreateMaterialPlaceholder("Default", 0, szReplaceMe);
    }

    if (!m_pDefaultTerrainLayersMtl)
    {
        m_pDefaultTerrainLayersMtl = CreateMaterialPlaceholder("DefaultTerrainLayer", MTL_FLAG_IS_TERRAIN, szReplaceMe);
    }

    if (!m_pDefaultLayersMtl)
    {
        m_pDefaultLayersMtl = LoadMaterial("Materials/material_layers_default", false);
    }

    if (!m_pNoDrawMtl)
    {
        m_pNoDrawMtl = new CMatInfo;
        m_pNoDrawMtl->SetFlags(MTL_FLAG_NODRAW);
        m_pNoDrawMtl->SetName(MATERIAL_NODRAW);
        SShaderItem si;
        si.m_pShader = GetRenderer()->EF_LoadShader(MATERIAL_NODRAW, 0);
        m_pNoDrawMtl->AssignShaderItem(si);

        AZStd::string unifiedName = UnifyName(m_pNoDrawMtl->GetName());
        auto* resetEvent = new ManualResetEvent();
        resetEvent->Set();

        m_mtlNameMap[unifiedName] = m_pNoDrawMtl;
        m_pendingMaterialLoads.emplace(unifiedName, AZStd::unique_ptr<ManualResetEvent>(resetEvent));
    }

    if (!m_pDefaultHelperMtl)
    {
        m_pDefaultHelperMtl = new CMatInfo;
        m_pDefaultHelperMtl->SetName("DefaultHelper");
        SInputShaderResources sr;
        sr.m_LMaterial.m_Opacity = 1;
        sr.m_LMaterial.m_Diffuse.set(1, 1, 1, 1);
        // Notice that the following line creates a texture data slot, inserts it and set the texture name
        sr.m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name = szReplaceMe;    
        SShaderItem si = GetRenderer()->EF_LoadShaderItem("Helper", true, 0, &sr);
        if (si.m_pShaderResources)
        {
            si.m_pShaderResources->SetMaterialName("DefaultHelper");
        }
        m_pDefaultHelperMtl->AssignShaderItem(si);
    }

    SLICE_AND_SLEEP();
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::LoadCGFMaterial(CMaterialCGF* pMaterialCGF, const char* sCgfFilename, unsigned long nLoadingFlags)
{
    FUNCTION_PROFILER_3DENGINE;
    LOADING_TIME_PROFILE_SECTION;

    CryPathString sMtlName = pMaterialCGF->name;
    if (sMtlName.find('/') == stack_string::npos)
    {
        // If no slashes in the name assume it is in same folder as a cgf.
        sMtlName = PathUtil::AddSlash(PathUtil::GetPath(stack_string(sCgfFilename))) + sMtlName;
    }
    else
    {
        sMtlName = PathUtil::MakeGamePath(sMtlName);
    }
    return LoadMaterial(sMtlName.c_str(), true, false, nLoadingFlags);
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::SetSketchMode(int mode)
{
    if (mode != 0)
    {
        gEnv->pConsole->ExecuteString("exec sketch_on");
    }
    else
    {
        gEnv->pConsole->ExecuteString("exec sketch_off");
    }

    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);

    for (MtlNameMap::iterator it = m_mtlNameMap.begin(); it != m_mtlNameMap.end(); ++it)
    {
        CMatInfo* pMtl = (CMatInfo*)it->second.get();
        pMtl->SetSketchMode(mode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::SetTexelDensityDebug(int mode)
{
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);

    for (MtlNameMap::iterator it = m_mtlNameMap.begin(); it != m_mtlNameMap.end(); ++it)
    {
        CMatInfo* pMtl = (CMatInfo*)it->second.get();
        pMtl->SetTexelDensityDebug(mode);
    }
}


namespace
{
    static bool IsPureChild(_smart_ptr<IMaterial> pMtl)
    {
        return (pMtl->GetFlags() & MTL_FLAG_PURE_CHILD) ? true : false;
    }
    static bool IsMultiSubMaterial(_smart_ptr<IMaterial> pMtl)
    {
        return (pMtl->GetFlags() & MTL_FLAG_MULTI_SUBMTL) ? true : false;
    };
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::ReloadMaterial(_smart_ptr<IMaterial> pMtl)
{
    AZStd::string name = UnifyName(pMtl->GetName());

    AZStd::string filename = name;
    auto extPos = filename.find('.');
    if (extPos == AZStd::string::npos)
    {
        filename += MATERIAL_EXT;
    }

    XmlNodeRef mtlNode = GetSystem()->LoadXmlFromFile(filename.c_str());

    if (mtlNode)
    {
        // This should reload the Material's data in-place without modifying any
        // material management registration. Otherwise we would have to send some
        // kind of messages about the material being replaced to every object 
        // with a pointer to it.
        MakeMaterialFromXml(name, mtlNode, false, 0, pMtl);
    }
    else
    {
        AZ_Warning("Material System", false, "Failed to re-load %s", filename.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::LoadMaterialFromXml(const char* sMtlName, XmlNodeRef mtlNode)
{
    AZStd::string name = UnifyName(sMtlName);

    AZStd::unique_lock<AZStd::recursive_mutex> lock(m_materialMapMutex);
    MtlNameMap::const_iterator it = m_mtlNameMap.find(name);

    _smart_ptr<IMaterial> pMtl = 0;

    if (it != m_mtlNameMap.end())
    {
        pMtl = it->second;
        pMtl = MakeMaterialFromXml(name, mtlNode, false, 0, pMtl);
        return pMtl;
    }

    if (!pMtl)
    {
        pMtl = MakeMaterialFromXml(name, mtlNode, false);
    }

    return pMtl;
}

//////////////////////////////////////////////////////////////////////////
bool CMatMan::SaveMaterial(XmlNodeRef node, _smart_ptr<IMaterial> pMtl)
{
    // Saving.
    node->setAttr("MtlFlags", pMtl->GetFlags());
    node->setAttr("DccMaterialHash", pMtl->GetDccMaterialHash());

    SShaderItem& si = pMtl->GetShaderItem(0);
    SInputShaderResources m_shaderResources = SInputShaderResources(si.m_pShaderResources);

    if (!IsMultiSubMaterial(pMtl))
    {
        node->setAttr("Shader", si.m_pShader->GetName());
        node->setAttr("GenMask", si.m_pShader->GetGenerationMask());
        node->setAttr("SurfaceType", pMtl->GetSurfaceType() ? pMtl->GetSurfaceType()->GetName() : NULL);

        SInputShaderResources& sr = m_shaderResources;
        //if (!m_shaderName.IsEmpty() && (azstricmp(m_shaderName,"nodraw") != 0))
        {
            s_materialHelpers.SetXmlFromLighting(sr, node);
            s_materialHelpers.SetXmlFromTextures(sr, node);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Save out the link name if present
    //////////////////////////////////////////////////////////////////////////
    const char* szLinkName = pMtl->GetMaterialLinkName();
    if (szLinkName != 0 && strlen(szLinkName))
    {
        XmlNodeRef pLinkName = node->newChild("MaterialLinkName");
        pLinkName->setAttr("name", szLinkName);
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if we have vertex deform.
    //////////////////////////////////////////////////////////////////////////
    s_materialHelpers.SetXmlFromVertexDeform(m_shaderResources, node);

    if (pMtl->GetSubMtlCount() > 0)
    {
        // Serialize sub materials.
        XmlNodeRef childsNode = node->newChild("SubMaterials");
        for (int i = 0; i < pMtl->GetSubMtlCount(); i++)
        {
            _smart_ptr<IMaterial> pSubMtl = pMtl->GetSubMtl(i);
            if (pSubMtl && IsPureChild(pSubMtl))
            {
                XmlNodeRef mtlNode = childsNode->newChild("Material");
                mtlNode->setAttr("Name", pSubMtl->GetName());
                SaveMaterial(mtlNode, pSubMtl);
            }
            else
            {
                XmlNodeRef mtlNode = childsNode->newChild("MaterialRef");
                if (pSubMtl)
                {
                    mtlNode->setAttr("Name", pSubMtl->GetName());
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Save public parameters.
    //////////////////////////////////////////////////////////////////////////
    /*
      if (m_publicVarsCache)
      {
        node->addChild( m_publicVarsCache );
      }
      else */

    if (!m_shaderResources.m_ShaderParams.empty())
    {
        XmlNodeRef publicsNode = node->newChild("PublicParams");
        s_materialHelpers.SetXmlFromShaderParams(m_shaderResources, publicsNode);
    }

    //////////////////////////////////////////////////////////////////////////
    // Save material layers data
    //////////////////////////////////////////////////////////////////////////

    bool bMaterialLayers = false;
    for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
    {
        const IMaterialLayer* pLayer = pMtl->GetLayer(l);
        if (pLayer && pLayer->GetShaderItem().m_pShader && strlen(pLayer->GetShaderItem().m_pShader->GetName()) != 0)
        {
            bMaterialLayers = true;
            break;
        }
    }

    if (bMaterialLayers)
    {
        XmlNodeRef mtlLayersNode = node->newChild("MaterialLayers");
        for (int l(0); l < MTL_LAYER_MAX_SLOTS; ++l)
        {
            XmlNodeRef layerNode = mtlLayersNode->newChild("Layer");
            const IMaterialLayer* pLayer = pMtl->GetLayer(l);
            if (pLayer && pLayer->GetShaderItem().m_pShader && strlen(pLayer->GetShaderItem().m_pShader->GetName()) != 0)
            {
                SInputShaderResources shaderRes(pLayer->GetShaderItem().m_pShaderResources);

                layerNode->setAttr("Name", pLayer->GetShaderItem().m_pShader->GetName());
                layerNode->setAttr("NoDraw", pLayer->GetShaderItem().m_pShader->GetFlags() & MTL_LAYER_USAGE_NODRAW);
                layerNode->setAttr("FadeOut", pLayer->GetShaderItem().m_pShader->GetFlags() & MTL_LAYER_USAGE_FADEOUT);

                if (!shaderRes.m_ShaderParams.empty())
                {
                    XmlNodeRef publicsNode = layerNode->newChild("PublicParams");
                    s_materialHelpers.SetXmlFromShaderParams(shaderRes, publicsNode);
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::PreloadLevelMaterials()
{
    LOADING_TIME_PROFILE_SECTION;

    //bool bMtlfCacheExist = GetISystem()->GetIResourceManager()->LoadLevelCachePak( MTL_LEVEL_CACHE_PAK,"" );
    //if (!bMtlfCacheExist)
    //return;

    PrintMessage("==== Starting Loading Level Materials ====");
    float fStartTime = GetCurAsyncTimeSec();

    auto pResList = GetISystem()->GetIResourceManager()->GetLevelResourceList();

    if (!pResList)
    {
        Error("Error loading level Materials: resource list is NULL");
        return;
    }

    int nCounter = 0;
    int nInLevelCacheCount = 0;

    _smart_ptr<IXmlParser> pXmlParser = GetISystem()->GetXmlUtils()->CreateXmlParser();

    // Request objects loading from Streaming System.
    CryPathString mtlName;
    CryPathString mtlFilename;
    CryPathString mtlCacheFilename;
    for (const char* sName = pResList->GetFirst(); sName != NULL; sName = pResList->GetNext())
    {
        if (strstr(sName, ".mtl") == 0 && strstr(sName, ".binmtl") == 0)
        {
            continue;
        }

        mtlFilename = sName;

        mtlName = sName;
        PathUtil::RemoveExtension(mtlName);

        if (FindMaterial(mtlName))
        {
            continue;
        }

        // Load this material as un-removable.
        _smart_ptr<IMaterial> pMtl = LoadMaterial(mtlName, false, true);
        if (pMtl)
        {
            nCounter++;
        }

        //This loop can take a few seconds, so we should refresh the loading screen and call the loading tick functions to ensure that no big gaps in coverage occur.
        SYNCHRONOUS_LOADING_TICK();
    }

    //GetISystem()->GetIResourceManager()->UnloadLevelCachePak( MTL_LEVEL_CACHE_PAK );
    PrintMessage("==== Finished loading level Materials: %d  mtls loaded (%d from LevelCache) in %.1f sec ====", nCounter, nInLevelCacheCount, GetCurAsyncTimeSec() - fStartTime);
}


//////////////////////////////////////////////////////////////////////////
void CMatMan::PreloadDecalMaterials()
{
    LOADING_TIME_PROFILE_SECTION;

    float fStartTime = GetCurAsyncTimeSec();

    bool bVerboseLogging = GetCVars()->e_StatObjPreload > 1;
    int nCounter = 0;

    // Wildcards load.
    CryPathString sPath = PathUtil::Make(CryPathString(MATERIAL_DECALS_FOLDER), CryPathString(MATERIAL_DECALS_SEARCH_WILDCARD));
    PrintMessage("===== Loading all Decal materials from a folder: %s =====", sPath.c_str());

    AZStd::vector<AZStd::string> mtlFiles;
    SDirectoryEnumeratorHelper dirHelper;
    dirHelper.ScanDirectoryRecursive(gEnv->pCryPak, "", MATERIAL_DECALS_FOLDER, MATERIAL_DECALS_SEARCH_WILDCARD, mtlFiles);

    for (int i = 0, num = (int)mtlFiles.size(); i < num; i++)
    {
        CryPathString sMtlName{ mtlFiles[i].data(), mtlFiles[i].size() };
        PathUtil::RemoveExtension(sMtlName);

        if (bVerboseLogging)
        {
            CryLog("Preloading Decal Material: %s", sMtlName.c_str());
        }

        _smart_ptr<IMaterial> pMtl = LoadMaterial(sMtlName.c_str(), false, true); // Load material as non-removable
        if (pMtl)
        {
            nCounter++;
        }
    }
    PrintMessage("==== Finished Loading Decal Materials: %d  mtls loaded in %.1f sec ====", nCounter, GetCurAsyncTimeSec() - fStartTime);
}

//////////////////////////////////////////////////////////////////////////
void CMatMan::ShutDown()
{
    CryLogAlways("shutting down mat man\n");
    {
        AZStd::unique_lock<AZStd::recursive_mutex> lock(m_materialMapMutex);

        m_pXmlParser = 0;
        
        m_mtlNameMap.clear();
        m_pendingMaterialLoads.clear();
    }

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_nonRemovablesMutex);
        stl::free_container(m_nonRemovables);
    }

    // Free default materials
    m_pDefaultMtl = nullptr;
    m_pDefaultTerrainLayersMtl = nullptr;
    m_pNoDrawMtl = nullptr;
    m_pDefaultHelperMtl = nullptr;
    m_pDefaultLayersMtl = nullptr; 

 

    m_pSurfaceTypeManager->RemoveAll();
    m_bInitialized = false;
}


void CMatMan::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_pDefaultMtl);
    pSizer->AddObject(m_pDefaultLayersMtl);
    pSizer->AddObject(m_pDefaultTerrainLayersMtl);
    pSizer->AddObject(m_pNoDrawMtl);
    pSizer->AddObject(m_pDefaultHelperMtl);
    pSizer->AddObject(m_pSurfaceTypeManager);
    pSizer->AddObject(m_pXmlParser);

    pSizer->AddObject(m_mtlNameMap);
    pSizer->AddObject(m_pendingMaterialLoads);
    pSizer->AddObject(m_nonRemovables);
}

void CMatMan::UpdateShaderItems()
{
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);

    for (MtlNameMap::iterator iter = m_mtlNameMap.begin(); iter != m_mtlNameMap.end(); ++iter)
    {
        CMatInfo* pMaterial = static_cast<CMatInfo*>(iter->second.get());
        pMaterial->UpdateShaderItems();
    }
}

void CMatMan::RefreshMaterialRuntime()
{
    RefreshShaderResourceConstants();
}

void CMatMan::RefreshShaderResourceConstants()
{
    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_materialMapMutex);

    for (MtlNameMap::iterator iter = m_mtlNameMap.begin(); iter != m_mtlNameMap.end(); ++iter)
    {
        CMatInfo* pMaterial = static_cast<CMatInfo*>(iter->second.get());
        pMaterial->RefreshShaderResourceConstants();
    }
}

// override from LegacyAssetEventBus::Handler
// Notifies listeners that a file changed
void CMatMan::OnFileChanged(AZStd::string assetPath)
{
    _smart_ptr<IMaterial> mat = FindMaterial(assetPath.c_str());

    //Reload material pointer in place if the material is found
    if (mat)
    {
        ReloadMaterial(mat);
    }
}

void CMatMan::OnFileRemoved(AZStd::string assetPath)
{
    _smart_ptr<IMaterial> mat = FindMaterial(assetPath.c_str());

    //Reload the material in place to a place holder
    if (mat)
    {
        Unregister(mat);
        CreateMaterialPlaceholder(mat->GetName(), mat->GetFlags(), szDefaultSolid, mat);
    }
}

