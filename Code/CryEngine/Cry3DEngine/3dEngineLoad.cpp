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

// Description : Level loading


#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "Ocean.h"
#include "DecalManager.h"
#include "MatMan.h"
#include "IndexedMesh.h"
#include "SkyLightManager.h"
#include "ObjectsTree.h"
#include "LightEntity.h"
#include "TimeOfDay.h"
#include "LightEntity.h"
#include "RenderMeshMerger.h"
#include "FogVolumeRenderNode.h"
#include "IDeferredCollisionEvent.h"
#include <IMaterialEffects.h>
#include "ClipVolumeManager.h"
#include "Environment/OceanEnvironmentBus.h"

#include <I3DEngine.h>
#include <LoadScreenBus.h>
#include <StatObjBus.h>
#include "AzCore/Component/TickBus.h"

//------------------------------------------------------------------------------
#define LEVEL_DATA_FILE "LevelData.xml"
#define CUSTOM_MATERIALS_FILE "Materials.xml"
#define PARTICLES_FILE "LevelParticles.xml"
#define SHADER_LIST_FILE "ShadersList.txt"
#define LEVEL_CONFIG_FILE "Level.cfg"
#define LEVEL_EDITOR_CONFIG_FILE "Editor.cfg"
//------------------------------------------------------------------------------
const uint32    MAX_ACTIVE_BREEZE_POINTS = 99;
//------------------------------------------------------------------------------

inline Vec3 StringToVector(const char* str)
{
    Vec3 vTemp(0, 0, 0);
    float x, y, z;
    if (azsscanf(str, "%f,%f,%f", &x, &y, &z) == 3)
    {
        vTemp(x, y, z);
    }
    else
    {
        vTemp(0, 0, 0);
    }
    return vTemp;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadEmptyLevel()
{
    CreateOctree(0.0f);
    LoadDefaultAssets();
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetLevelPath(const char* szFolderName)
{
    // make folder path
    assert(strlen(szFolderName) < 1024);
    azstrcpy(m_szLevelFolder, AZ_ARRAY_SIZE(m_szLevelFolder), szFolderName);
    if (strlen(m_szLevelFolder) > 0)
    {
        if (m_szLevelFolder[strlen(m_szLevelFolder) - 1] != '/')
        {
            azstrcat(m_szLevelFolder, AZ_ARRAY_SIZE (m_szLevelFolder), "/");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadDefaultAssets()
{
    GetRenderer()->InitSystemResources(FRR_SYSTEM_RESOURCES);

    //Add a call to refresh the loading screen and call the loading tick functions to ensure that no big gaps in coverage occur.
    SYNCHRONOUS_LOADING_TICK();

#if !defined(SYS_ENV_AS_STRUCT)
    PREFAST_ASSUME(gEnv);
#endif

    GetMatMan()->InitDefaults();

    m_nBlackTexID = GetRenderer()->EF_LoadTexture("EngineAssets/Textures/black.dds", FT_DONT_STREAM)->GetTextureID();
    m_nBlackCMTexID = GetRenderer()->EF_LoadTexture("EngineAssets/Textures/BlackCM.dds", FT_DONT_RELEASE | FT_DONT_STREAM)->GetTextureID();

    m_pMatFogVolEllipsoid = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/FogVolumeEllipsoid", false);
    m_pMatFogVolBox = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/FogVolumeBox", false);

    if (!m_pRESky)
    {
        m_pRESky = (CRESky*)GetRenderer()->EF_CreateRE(eDATA_Sky); //m_pRESky->m_fAlpha = 1.f;
    }
    if (!m_pREHDRSky)
    {
        m_pREHDRSky = (CREHDRSky*)GetRenderer()->EF_CreateRE(eDATA_HDRSky);
    }

    if (!m_ptexIconLowMemoryUsage)
    {
        m_ptexIconLowMemoryUsage = gEnv->pRenderer->EF_LoadDefaultTexture("LowMemoryUsage");
    }

    if (!m_ptexIconAverageMemoryUsage)
    {
        m_ptexIconAverageMemoryUsage = gEnv->pRenderer->EF_LoadDefaultTexture("AverageMemoryUsage");
    }

    if (!m_ptexIconHighMemoryUsage)
    {
        m_ptexIconHighMemoryUsage = gEnv->pRenderer->EF_LoadDefaultTexture("HighMemoryUsage");
    }

    if (!m_ptexIconEditorConnectedToConsole)
    {
        m_ptexIconEditorConnectedToConsole = gEnv->pRenderer->EF_LoadDefaultTexture("LivePreview");
    }
}


//////////////////////////////////////////////////////////////////////////
bool C3DEngine::InitLevelForEditor([[maybe_unused]] const char* szFolderName, [[maybe_unused]] const char* szMissionName)
{
#if defined(CONSOLE)
    CRY_ASSERT_MESSAGE(0, "InitLevelForEditor not supported on consoles yet");
    return false;
#else
    LOADING_TIME_PROFILE_SECTION;

    m_bEditor = true;
    m_bAreaActivationInUse = false;
    m_bLayersActivated = true;

    ClearDebugFPSInfo();


    if (!szFolderName || !szFolderName[0])
    {
        Warning("C3DEngine::LoadLevel: Level name is not specified");
        return 0;
    }

    if (!szMissionName || !szMissionName[0])
    {
        Warning("C3DEngine::LoadLevel: Mission name is not specified");
    }

    char szMissionNameBody[256] = "NoMission";
    if (!szMissionName)
    {
        szMissionName = szMissionNameBody;
    }

    SetLevelPath(szFolderName);

    // Load console vars specific to this level.
    if (GetPak()->IsFileExist(GetLevelFilePath(LEVEL_CONFIG_FILE)))
    {
        GetISystem()->LoadConfiguration(GetLevelFilePath(LEVEL_CONFIG_FILE));
    }

    if (GetPak()->IsFileExist(GetLevelFilePath(LEVEL_EDITOR_CONFIG_FILE)))
    {
        GetISystem()->LoadConfiguration(GetLevelFilePath(LEVEL_EDITOR_CONFIG_FILE));
    }

    if (!m_pObjManager)
    {
        m_pObjManager = CryAlignedNew<CObjManager>();
    }

    if (!m_pVisAreaManager)
    {
        m_pVisAreaManager = new CVisAreaManager();
    }

    CRY_ASSERT(m_pClipVolumeManager->GetClipVolumeCount() == 0);

    if (m_pSkyLightManager)
    {
        m_pSkyLightManager->InitSkyDomeMesh();
    }

    // recreate decals
    SAFE_DELETE(m_pDecalManager);
    m_pDecalManager = new CDecalManager();

    // restore game state
    EnableOceanRendering(true);
    m_pObjManager->SetLockCGFResources(0);

    LoadDefaultAssets();

    {
        const char* SettingsFileName = GetLevelFilePath("ScreenshotMap.Settings");

        AZ::IO::HandleType metaFileHandle = gEnv->pCryPak->FOpen(SettingsFileName, "r");
        if (metaFileHandle != AZ::IO::InvalidHandle)
        {
            char Data[1024 * 8];
            gEnv->pCryPak->FRead(Data, sizeof(Data), metaFileHandle);
            azsscanf(Data, "<Map CenterX=\"%f\" CenterY=\"%f\" SizeX=\"%f\" SizeY=\"%f\" Height=\"%f\"  Quality=\"%d\" Orientation=\"%d\" />",
                &GetCVars()->e_ScreenShotMapCenterX,
                &GetCVars()->e_ScreenShotMapCenterY,
                &GetCVars()->e_ScreenShotMapSizeX,
                &GetCVars()->e_ScreenShotMapSizeY,
                &GetCVars()->e_ScreenShotMapCamHeight,
                &GetCVars()->e_ScreenShotQuality,
                &GetCVars()->e_ScreenShotMapOrientation);
            gEnv->pCryPak->FClose(metaFileHandle);
        }
    }


    GetObjManager()->LoadOcclusionMesh(szFolderName);

    //  delete m_pObjectsTree[nSID];
    //  m_pObjectsTree[nSID] = NULL;
    return (true);
#endif
}

bool C3DEngine::LevelLoadingInProgress()
{
    return Cry3DEngineBase::m_bLevelLoadingInProgress;
}

bool C3DEngine::LoadCompiledOctreeForEditor()
{
    // Load LevelData.xml File.
    XmlNodeRef xmlLevelData = GetSystem()->LoadXmlFromFile(GetLevelFilePath(LEVEL_DATA_FILE));

    if (xmlLevelData == 0)
    {
        Error("C3DEngine::LoadLevel: xml file not found (files missing?)"); // files missing ?
        return false;
    }

    std::vector<struct IStatObj*>* pStatObjTable = NULL;
    std::vector<_smart_ptr<IMaterial> >* pMatTable = NULL;

    int nSID = 0;

    XmlNodeRef nodeRef = xmlLevelData->findChild("SurfaceTypes");

    LoadCollisionClasses(xmlLevelData->findChild("CollisionClasses"));

    SAFE_DELETE(pStatObjTable);
    SAFE_DELETE(pMatTable);

    return true;
}

bool C3DEngine::LoadVisAreas(std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable)
{
    LOADING_TIME_PROFILE_SECTION;

    PrintMessage("===== Loading %s =====", COMPILED_VISAREA_MAP_FILE_NAME);

    // open file
    AZ::IO::HandleType fileHandle = GetPak()->FOpen(GetLevelFilePath(COMPILED_VISAREA_MAP_FILE_NAME), "rbx");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    // read header
    SVisAreaManChunkHeader header;
    if (!GetPak()->FRead(&header, 1, fileHandle, false))
    {
        GetPak()->FClose(fileHandle);
        return 0;
    }

    SwapEndian(header, (header.nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

    if (header.nChunkSize)
    {
        assert(!m_pVisAreaManager);
        m_pVisAreaManager = new CVisAreaManager();
        if (!m_pVisAreaManager->Load(fileHandle, header.nChunkSize, &header, *ppStatObjTable, *ppMatTable))
        {
            delete m_pVisAreaManager;
            m_pVisAreaManager = NULL;
        }
    }

    assert(GetPak()->FEof(fileHandle));

    GetPak()->FClose(fileHandle);

    return m_pVisAreaManager != NULL;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::UnloadLevel()
{
    if (!m_levelLoaded)
    {
        return;
    }
    GetRenderer()->EnableLevelUnloading(true);

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_UNLOAD, 0, 0);
    // Run any events that were queued against the system tick bus before we start tearing down systems and deleting things
    // This should prevent any stale events from running when the next level is loaded
    AZ::SystemTickBus::ExecuteQueuedEvents();

    GetRenderer()->EnableLevelUnloading(false);

    m_bInUnload = true;
    m_szLevelFolder[0] = 0;

    GetRenderer()->FlushRTCommands(true, true, true);

    SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::ReleaseData);
    InstanceStatObjEventBus::Broadcast(&InstanceStatObjEventBus::Events::ReleaseData);

    FreeRNTmpDataPool();

    if (m_pSkyLightManager)
    {
        m_pSkyLightManager->ReleaseSkyDomeMesh();
    }

    ResetPostEffects();

    // delete decal manager
    if (m_pDecalManager)
    {
        CryComment("Deleting Decals");
        SAFE_DELETE(m_pDecalManager);
        CryComment("done");
    }

    if (m_pOcean)
    {
        CryComment("Deleting Ocean");
        SAFE_DELETE(m_pOcean);
        CryComment("done");
    }

    // delete outdoor objects
    CryComment("Deleting Octree");
    DestroyOctree();
    SAFE_DELETE(m_pObjectsTree);
    m_pObjectsTree = nullptr;

    // delete indoors
    if (m_pVisAreaManager)
    {
        CryComment("Deleting VisAreas");
        SAFE_DELETE(m_pVisAreaManager);
        CryComment("done");
    }

    CRY_ASSERT(m_pClipVolumeManager->GetClipVolumeCount() == 0);

    m_LightVolumesMgr.Reset();

    m_pTerrainWaterMat = 0;
    m_nWaterBottomTexId = 0;


    //////////////////////////////////////////////////////////////////////////
    CryComment("Removing Lights ...");
    DeleteAllStaticLightSources();
    SAFE_DELETE(m_pSun);
    CryComment("done");
    //////////////////////////////////////////////////////////////////////////


    CleanLevelShaders();

    if (m_pRESky)
    {
        m_pRESky->Release(true);
    }
    if (m_pREHDRSky)
    {
        m_pREHDRSky->Release(true);
    }
    m_pRESky = 0;
    m_pREHDRSky = 0;
    stl::free_container(m_skyMatName);
    stl::free_container(m_skyLowSpecMatName);
    m_previousSkyType = -1;

    if (m_nCloudShadowTexId)
    {
        ITexture* tex = GetRenderer()->EF_GetTextureByID(m_nCloudShadowTexId);
        if (tex)
        {
            tex->Release();
        }

        m_nCloudShadowTexId = 0;
        GetRenderer()->SetCloudShadowsParams(0, Vec3(0, 0, 0), 1, false, 1);
        SetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_ENABLE, Vec3(0, 0, 0));
    }

    if (m_nNightMoonTexId)
    {
        ITexture* tex = GetRenderer()->EF_GetTextureByID(m_nNightMoonTexId);
        if (tex)
        {
            tex->Release();
        }

        m_nNightMoonTexId = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    if (m_pObjManager)
    {
        bool bDeleteAll = !m_bEditor || m_bInShutDown;
        CryComment("Deleting Static Objects");
        m_pObjManager->UnloadObjects(bDeleteAll);
        m_pObjManager->GetCullThread().UnloadLevel();
        CryComment("done");
    }

    AZ_Assert(m_pObjectsTree == nullptr, "m_pObjectsTree == nullptr");
    COctreeNode::StaticReset();

    //////////////////////////////////////////////////////////////////////////
    // Force delete all materials.
    //////////////////////////////////////////////////////////////////////////
    if (GetMatMan() && !m_bEditor)
    {
        // Should be after deleting all meshes.
        // We force delete all materials.
        CryComment("Deleting Materials");
        GetMatMan()->ShutDown();
        CryComment("done");
    }

    //Set default icons to nullptr here, texture manager will take care of the memory release
    m_ptexIconAverageMemoryUsage = nullptr;
    m_ptexIconLowMemoryUsage = nullptr;
    m_ptexIconHighMemoryUsage = nullptr;
    m_ptexIconEditorConnectedToConsole = nullptr;

    if (m_pOpticsManager && !gEnv->IsEditor())
    {
        m_pOpticsManager->Reset();
    }

    //////////////////////////////////////////////////////////////////////////

    stl::free_container(m_lstAlwaysVisible);
    if (m_decalRenderNodes.empty())
    {
        stl::free_container(m_decalRenderNodes);
    }
    stl::free_container(m_lstPerObjectShadows);
    m_nCustomShadowFrustumCount = 0;

    Cry3DEngineBase::m_pRenderMeshMerger->Reset();

    SAFE_DELETE(m_pTimeOfDay);
    CLightEntity::StaticReset();
    CVisArea::StaticReset();
    CFogVolumeRenderNode::StaticReset();

    GetRenderer()->FlushRTCommands(true, true, true);

    IDeferredPhysicsEventManager* pPhysEventManager = GetDeferredPhysicsEventManager();
    if (pPhysEventManager)
    {
        pPhysEventManager->ClearDeferredEvents();
    }

    m_PhysicsAreaUpdates.Reset();
    for (size_t i = 0; i < NUM_BENDING_POOLS; stl::free_container(m_bendingPool[i++]))
    {
        ;
    }

    //////////////////////////////////////////////////////////////////////////
    // clear data used for SRenderingPass
    stl::free_container(m_RenderingPassCameras[0]);
    stl::free_container(m_RenderingPassCameras[1]);
    stl::free_container(m_deferredRenderComponentStreamingPriorityUpdates);

    stl::free_container(m_lstCustomShadowFrustums);

    m_nWindSamplePositions = 0;
    if (m_pWindSamplePositions)
    {
        CryModuleMemalignFree(m_pWindSamplePositions);
        m_pWindSamplePositions = NULL;
    }

    stl::free_container(m_collisionClasses);
    m_levelLoaded = false;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadFlaresData()
{
    string flareExportListPath = gEnv->p3DEngine->GetLevelFilePath(FLARE_EXPORT_FILE);
    XmlNodeRef pFlareRootNode = gEnv->pSystem->LoadXmlFromFile(flareExportListPath);

    if (pFlareRootNode == NULL)
    {
        return;
    }

    int nFlareExportFileVer = 0;
    pFlareRootNode->getAttr("Version", nFlareExportFileVer);

    for (int i = 0, iCount(pFlareRootNode->getChildCount()); i < iCount; ++i)
    {
        XmlNodeRef pFlareNode = pFlareRootNode->getChild(i);
        if (pFlareNode == NULL)
        {
            continue;
        }
        const char* flareName = NULL;
        if (!pFlareNode->getAttr("name", &flareName))
        {
            continue;
        }
        int nOutIndex(-1);

        if (nFlareExportFileVer == 0)
        {
            gEnv->pOpticsManager->Load(flareName, nOutIndex);
        }
        else if (nFlareExportFileVer == 1)
        {
            if (pFlareNode->getChildCount() == 0)
            {
                gEnv->pOpticsManager->Load(flareName, nOutIndex);
            }
            else if (pFlareNode->getChildCount() > 0)
            {
                gEnv->pOpticsManager->Load(pFlareNode, nOutIndex);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::LoadLevel(const char* szFolderName, const char* szMissionName)
{
    LOADING_TIME_PROFILE_SECTION;

    CRY_ASSERT(m_levelLoaded == false);

    PREFAST_ASSUME(gEnv);

    stl::scoped_set<bool> setInLoad(m_bInLoad, true);

    m_bInUnload = false;
    m_bAreaActivationInUse = false;
    m_bLayersActivated = false;
    m_eShadowMode = ESM_NORMAL;

    m_vPrevMainFrameCamPos.Set(-1000000.f, -1000000.f, -1000000.f);
    m_vAverageCameraMoveDir = Vec3(0);
    m_fAverageCameraSpeed = 0;

    ClearDebugFPSInfo();

#if !defined(CONSOLE)
    m_bEditor = false;
#endif

    assert(!m_bEditor);

    //////////////////////////////////////////////////////////////////////////

    if (!szFolderName || !szFolderName[0])
    {
        Warning("C3DEngine::LoadLevel: Level name is not specified");
        return 0;
    }

    if (!szMissionName || !szMissionName[0])
    {
        Warning("C3DEngine::LoadLevel: Mission name is not specified");
    }

    char szMissionNameBody[256] = "NoMission";
    if (!szMissionName)
    {
        szMissionName = szMissionNameBody;
    }

    SetLevelPath(szFolderName);

    if (GetPak()->IsFileExist(GetLevelFilePath(LEVEL_CONFIG_FILE)))
    {
        GetISystem()->LoadConfiguration(GetLevelFilePath(LEVEL_CONFIG_FILE));
    }

    { // check is LevelData.xml file exist
        char sMapFileName[_MAX_PATH];
        cry_strcpy(sMapFileName, m_szLevelFolder);
        cry_strcat(sMapFileName, LEVEL_DATA_FILE);
        if (!IsValidFile(sMapFileName))
        {
            PrintMessage("Error: Level not found: %s", sMapFileName);
            return 0;
        }
    }

    if (!m_pObjManager)
    {
        m_pObjManager = CryAlignedNew<CObjManager>();
    }

    CRY_ASSERT(m_pClipVolumeManager->GetClipVolumeCount() == 0);

    // Load and activate all shaders used by the level before activating any shaders
    if (!m_bEditor)
    {
        LoadUsedShadersList();
    }

#if AZ_LOADSCREENCOMPONENT_ENABLED
    // Make sure system resources are inited before displaying a load screen
    GetRenderer()->InitSystemResources(FRR_SYSTEM_RESOURCES);
    // IMPORTANT: This MUST be done AFTER the LoadConfiguration() above.
    EBUS_EVENT(LoadScreenBus, LevelStart);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    LoadDefaultAssets();

    if (m_pSkyLightManager)
    {
        m_pSkyLightManager->InitSkyDomeMesh();
        // set default render parameters.
        // for some reason this is not done later???
        m_pSkyLightManager->UpdateRenderParams();
    }

    // Load LevelData.xml File.
    XmlNodeRef xmlLevelData = GetSystem()->LoadXmlFromFile(GetLevelFilePath(LEVEL_DATA_FILE));

    if (xmlLevelData == 0)
    {
        Error("C3DEngine::LoadLevel: xml file not found (files missing?)"); // files missing ?
        return false;
    }

    // re-create decal manager
    SAFE_DELETE(m_pDecalManager);
    m_pDecalManager = new CDecalManager();

    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_MATERIALS);
    if (GetCVars()->e_PreloadMaterials)
    {
        // Preload materials.
        GetMatMan()->PreloadLevelMaterials();
    }
    if (GetCVars()->e_PreloadDecals)
    {
        // Preload materials.
        GetMatMan()->PreloadDecalMaterials();
    }

    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_OBJECTS);
    // preload level cgfs
    if (GetCVars()->e_StatObjPreload && !gEnv->IsEditor())
    {
        m_pObjManager->PreloadLevelObjects();
    }

    std::vector<struct IStatObj*>* pStatObjTable = NULL;
    std::vector<_smart_ptr<IMaterial> >* pMatTable = NULL;

    int nSID = 0;

    // load terrain
    XmlNodeRef nodeRef = xmlLevelData->findChild("SurfaceTypes");

    LoadCollisionClasses(xmlLevelData->findChild("CollisionClasses"));

    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_STATIC_WORLD);

#if defined(FEATURE_SVO_GI)
    if (gEnv->pConsole->GetCVar("e_GI")->GetIVal())
    {
        // Load SVOGI settings
        char szFileName[256];
        azsprintf(szFileName, "mission_%s.xml", szMissionName);
        XmlNodeRef xmlMission = GetSystem()->LoadXmlFromFile(Get3DEngine()->GetLevelFilePath(szFileName));
        if (xmlMission)
        {
            LoadTISettings(xmlMission->findChild("Environment"));
        }
    }
#endif

    // load indoors
    if (!LoadVisAreas(&pStatObjTable, &pMatTable))
    {
        Error("VisAreas file (%s) not found or file version error, please try to re-export the level", COMPILED_VISAREA_MAP_FILE_NAME);
        return false;
    }

    SAFE_DELETE(pStatObjTable);
    SAFE_DELETE(pMatTable);

    //Update loading screen and important tick functions
    SYNCHRONOUS_LOADING_TICK();

    PrintMessage("===== Loading mission settings from XML =====");

    //Update loading screen and important tick functions
    SYNCHRONOUS_LOADING_TICK();

    // load leveldata.xml
    m_pTerrainWaterMat = 0;
    m_nWaterBottomTexId = 0;
    LoadMissionDataFromXMLNode(szMissionName);

    //Update loading screen and important tick functions
    SYNCHRONOUS_LOADING_TICK();


    // init water if not initialized already (if no mission was found)
    if (!GetOcean())
    {
        PrintMessage("===== Creating Ocean =====");
        CreateOcean(m_pTerrainWaterMat, COcean::GetWaterLevelInfo());
    }

    PrintMessage("===== Load level physics data =====");
    LoadFlaresData();

    // restore game state
    EnableOceanRendering(true);
    m_pObjManager->SetLockCGFResources(false);

    PrintMessage("===== loading occlusion mesh =====");

    GetObjManager()->LoadOcclusionMesh(szFolderName);

    PrintMessage("===== Finished loading static world =====");

    m_skipedLayers.clear();

    if (gEnv->pMaterialEffects)
    {
        gEnv->pMaterialEffects->CompleteInit();
    }

    return (true);
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadCollisionClasses(XmlNodeRef node)
{
    m_collisionClasses.clear();
    if (node)
    {
        int count = node->getChildCount();
        m_collisionClasses.reserve(count);
        for (int i = 0; i < count; i++)
        {
            SCollisionClass cc(0, 0);
            XmlNodeRef xmlCC = node->getChild(i);
            xmlCC->getAttr("type", cc.type);
            xmlCC->getAttr("ignore", cc.ignore);
            m_collisionClasses.push_back(cc);
        }
    }
}

void C3DEngine::LoadMissionDataFromXMLNode(const char* szMissionName)
{
    LOADING_TIME_PROFILE_SECTION;

    GetRenderer()->MakeMainContextActive();

    // set default values
    m_vFogColor(1, 1, 1);
    m_fMaxViewDistHighSpec = 8000;
    m_fMaxViewDistLowSpec = 1000;
    m_vDefFogColor    = m_vFogColor;

    // mission environment
    if (szMissionName && szMissionName[0])
    {
        char szFileName[256];
        sprintf_s(szFileName, "mission_%s.xml", szMissionName);
        XmlNodeRef xmlMission = GetSystem()->LoadXmlFromFile(Get3DEngine()->GetLevelFilePath(szFileName));
        if (xmlMission)
        {
            LoadEnvironmentSettingsFromXML(xmlMission->findChild("Environment"), GetDefSID());
            LoadTimeOfDaySettingsFromXML(xmlMission->findChild("TimeOfDay"));
        }
        else
        {
            Error("C3DEngine::LoadMissionDataFromXMLNode: Mission file not found: %s", szFileName);
        }
    }
    else
    {
        Error("C3DEngine::LoadMissionDataFromXMLNode: Mission name is not defined");
    }
}

char* C3DEngine::GetXMLAttribText(XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szDefaultValue)
{
    static char szResText[128];

    cry_strcpy(szResText, szDefaultValue);

    XmlNodeRef nodeLevel = pInputNode->findChild(szLevel1);
    if (nodeLevel && nodeLevel->haveAttr(szLevel2))
    {
        cry_strcpy(szResText, nodeLevel->getAttr(szLevel2));
    }

    return szResText;
}

char* C3DEngine::GetXMLAttribText(XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szLevel3, const char* szDefaultValue)
{
    static char szResText[128];

    cry_strcpy(szResText, szDefaultValue);

    XmlNodeRef nodeLevel = pInputNode->findChild(szLevel1);
    if (nodeLevel)
    {
        nodeLevel = nodeLevel->findChild(szLevel2);
        if (nodeLevel)
        {
            cry_strcpy(szResText, nodeLevel->getAttr(szLevel3));
        }
    }

    return szResText;
}

void C3DEngine::UpdateMoonDirection()
{
    float moonLati(-gf_PI + gf_PI* m_moonRotationLatitude / 180.0f);
    float moonLong(0.5f * gf_PI - gf_PI* m_moonRotationLongitude / 180.0f);

    float sinLon(sinf(moonLong));
    float cosLon(cosf(moonLong));
    float sinLat(sinf(moonLati));
    float cosLat(cosf(moonLati));

    m_moonDirection = Vec3(sinLon * cosLat, sinLon * sinLat, cosLon);
}

void C3DEngine::LoadEnvironmentSettingsFromXML(XmlNodeRef pInputNode, [[maybe_unused]] int nSID)
{
    PrintComment("Loading environment settings from XML ...");

    // set start and end time for dawn/dusk (to fade moon/sun light in and out)
    float dawnTime = (float)atof(GetXMLAttribText(pInputNode, "Lighting", "DawnTime", "355"));
    float dawnDuration = (float)atof(GetXMLAttribText(pInputNode, "Lighting", "DawnDuration", "10"));
    float duskTime = (float)atof(GetXMLAttribText(pInputNode, "Lighting", "DuskTime", "365"));
    float duskDuration = (float)atof(GetXMLAttribText(pInputNode, "Lighting", "DuskDuration", "10"));

    m_dawnStart = (dawnTime - dawnDuration * 0.5f) / 60.0f;
    m_dawnEnd = (dawnTime + dawnDuration * 0.5f) / 60.0f;
    m_duskStart = 12.0f + (duskTime - duskDuration * 0.5f) / 60.0f;
    m_duskEnd = 12.0f + (duskTime + duskDuration * 0.5f) / 60.0f;

    if (m_dawnEnd > m_duskStart)
    {
        m_duskEnd += m_dawnEnd - m_duskStart;
        m_duskStart = m_dawnEnd;
    }


    // get moon info
    m_moonRotationLatitude = (float)atof(GetXMLAttribText(pInputNode, "Moon", "Latitude", "240"));
    m_moonRotationLongitude = (float)atof(GetXMLAttribText(pInputNode, "Moon", "Longitude", "45"));
    UpdateMoonDirection();

    m_nightMoonSize = (float)atof(GetXMLAttribText(pInputNode, "Moon", "Size", "0.5"));

    {
        char moonTexture[256];
        cry_strcpy(moonTexture, GetXMLAttribText(pInputNode, "Moon", "Texture", ""));

        ITexture* pTex(0);
        if (moonTexture[0] != '\0')
        {
            pTex = GetRenderer()->EF_LoadTexture(moonTexture, FT_DONT_STREAM);
        }

        m_nNightMoonTexId = pTex ? pTex->GetTextureID() : 0;
    }

    // max view distance
    m_fMaxViewDistHighSpec = (float)atol(GetXMLAttribText(pInputNode, "Fog", "ViewDistance", "8000"));
    m_fMaxViewDistLowSpec = (float)atol(GetXMLAttribText(pInputNode, "Fog", "ViewDistanceLowSpec", "1000"));
    m_fMaxViewDistScale = 1.f;

    m_volFogGlobalDensityMultiplierLDR = (float)max(atof(GetXMLAttribText(pInputNode, "Fog", "LDRGlobalDensMult", "1.0")), 0.0);

    // SkyBox
    const string skyMaterialName = GetXMLAttribText(pInputNode, "SkyBox", "Material", "Materials/Sky/Sky");
    const string skyLowSpecMaterialName = GetXMLAttribText(pInputNode, "SkyBox", "MaterialLowSpec", "Materials/Sky/Sky");
    SetSkyMaterialPath(skyMaterialName);
    SetSkyLowSpecMaterialPath(skyLowSpecMaterialName);
    LoadSkyMaterial();

    m_fSkyBoxAngle = (float)atof(GetXMLAttribText(pInputNode, "SkyBox", "Angle", "0.0"));
    m_fSkyBoxStretching = (float)atof(GetXMLAttribText(pInputNode, "SkyBox", "Stretching", "1.0"));

    // set terrain water (aka the infinite ocean), sun road and bottom shaders
    if (OceanToggle::IsActive())
    {
        AZStd::string szOceanMatName = OceanRequest::GetOceanMaterialName();
        m_pTerrainWaterMat = GetMatMan()->LoadMaterial(szOceanMatName.c_str(), false);
    }
    else
    {
        char szTerrainWaterMatName[256];
        cry_strcpy(szTerrainWaterMatName, GetXMLAttribText(pInputNode, "Ocean", "Material", "EngineAssets/Materials/Water/Ocean_default"));
        m_pTerrainWaterMat = szTerrainWaterMatName[0] ? GetMatMan()->LoadMaterial(szTerrainWaterMatName, false) : nullptr;
    }
    CreateOcean(m_pTerrainWaterMat, COcean::GetWaterLevelInfo());

    m_oceanWindDirection = (float) atof(GetXMLAttribText(pInputNode, "OceanAnimation", "WindDirection", "1.0"));
    m_oceanWindSpeed = (float) atof(GetXMLAttribText(pInputNode, "OceanAnimation", "WindSpeed", "4.0"));
    m_oceanWavesSpeed = (float) atof(GetXMLAttribText(pInputNode, "OceanAnimation", "WavesSpeed", "1.0"));
    m_oceanWavesAmount = (float) atof(GetXMLAttribText(pInputNode, "OceanAnimation", "WavesAmount", "1.5"));
    m_oceanWavesSize = (float) atof(GetXMLAttribText(pInputNode, "OceanAnimation", "WavesSize", "0.75"));

    // re-scale speed based on size - the smaller the faster waves move
    m_oceanWavesSpeed /= m_oceanWavesSize;

    m_oceanCausticsDistanceAtten = (float)atof(GetXMLAttribText(pInputNode, "Ocean", "CausticsDistanceAtten", "100.0"));
    m_oceanCausticsTiling = (float)atof(GetXMLAttribText(pInputNode, "Ocean", "CausticsTilling", "1.0"));
    m_oceanCausticDepth = (float)atof(GetXMLAttribText(pInputNode, "Ocean", "CausticDepth", "8.0"));
    m_oceanCausticIntensity = (float)atof(GetXMLAttribText(pInputNode, "Ocean", "CausticIntensity", "1.0"));

    // update relevant time of day settings
    ITimeOfDay* pTimeOfDay(GetTimeOfDay());
    if (pTimeOfDay)
    {
        CTimeOfDay::SEnvironmentInfo envTODInfo;
        {
            const char* pText  = GetXMLAttribText(pInputNode, "EnvState", "SunLinkedToTOD", "true");
            envTODInfo.bSunLinkedToTOD = !strcmp(pText, "true") || !strcmp(pText, "1");
        }
        // get rotation of sun around z axis (needed to define an arbitrary path over zenit for day/night cycle position calculations)
        envTODInfo.sunRotationLatitude = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "SunRotation", "240"));
        envTODInfo.sunRotationLongitude = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "Longitude", "90"));

        pTimeOfDay->SetEnvironmentSettings(envTODInfo);
        pTimeOfDay->Update(true, true);
    }

    {
        const char* pText = GetXMLAttribText(pInputNode, "EnvState", "SunShadowsMinSpec", "1");
        int nMinSpec = atoi(pText);
        if (nMinSpec > 0 && CheckMinSpec(nMinSpec))
        {
            m_bSunShadows = true;
        }
        else
        {
            m_bSunShadows = false;
        }
    }

    {
        const char* pText = GetXMLAttribText(pInputNode, "EnvState", "SunShadowsAdditionalCascadeMinSpec", "0");
        int nMinSpec = atoi(pText);
        if (nMinSpec > 0 && CheckMinSpec(nMinSpec))
        {
            m_nSunAdditionalCascades = 1;
        }
        else
        {
            m_nSunAdditionalCascades = 0;
        }
    }

    {
        m_nGsmCache = m_pConsole->GetCVar("r_ShadowsCache")->GetIVal();
    }

    {
        const char* pText = GetXMLAttribText(pInputNode, "Terrain", "HeightMapAO", "false");
        m_bHeightMapAoEnabled = !strcmp(pText, "true") || !strcmp(pText, "1");
    }

    {
        int nMinSpec = 3;//atoi(pText);
        m_fSunClipPlaneRange = 256.0;
        m_fSunClipPlaneRangeShift = 0.0f;

        if (nMinSpec > 0 && CheckMinSpec(nMinSpec))
        {
            m_fSunClipPlaneRange = (float)atof(GetXMLAttribText(pInputNode, "EnvState", "SunShadowsClipPlaneRange", "256.0"));

            float fSunClipPlaneRangeShift = (float)atof(GetXMLAttribText(pInputNode, "EnvState", "SunShadowsClipPlaneRangeShift", "0.0"));
            m_fSunClipPlaneRangeShift = clamp_tpl(fSunClipPlaneRangeShift / 100.0f, 0.0f, 1.0f);
        }
    }

    {
        const char* pText = GetXMLAttribText(pInputNode, "EnvState", "UseLayersActivation", "false");
        Get3DEngine()->m_bAreaActivationInUse = !strcmp(pText, "true") || !strcmp(pText, "1");
    }

    // load cloud shadow parameters
    {
        char cloudShadowTexture[256];
        cry_strcpy(cloudShadowTexture, GetXMLAttribText(pInputNode, "CloudShadows", "CloudShadowTexture", ""));

        ITexture* pTex = 0;
        if (cloudShadowTexture[0] != '\0')
        {
            pTex = GetRenderer()->EF_LoadTexture(cloudShadowTexture, FT_DONT_STREAM);
        }

        m_nCloudShadowTexId = pTex ? pTex->GetTextureID() : 0;

        // Get animation parameters
        const Vec3 cloudShadowSpeed = StringToVector(GetXMLAttribText(pInputNode, "CloudShadows", "CloudShadowSpeed", "0,0,0"));

        const float cloudShadowTiling = (float)atof(GetXMLAttribText(pInputNode, "CloudShadows", "CloudShadowTiling", "1.0"));
        const float cloudShadowBrightness = (float)atof(GetXMLAttribText(pInputNode, "CloudShadows", "CloudShadowBrightness", "1.0"));

        const char* pText = GetXMLAttribText(pInputNode, "CloudShadows", "CloudShadowInvert", "false");
        const bool cloudShadowInvert = !strcmp(pText, "true") || !strcmp(pText, "1");

        GetRenderer()->SetCloudShadowsParams(m_nCloudShadowTexId, cloudShadowSpeed, cloudShadowTiling, cloudShadowInvert, cloudShadowBrightness);
    }

    // Particle lighting params. <DEPRECATED> Remove for projects post C3, should be no particle lighting multipliers - have assets tweaked from start
    {
        m_fParticlesAmbientMultiplier = (float)atof(GetXMLAttribText(pInputNode, "ParticleLighting", "AmbientMul", "1.0"));
        m_fParticlesLightMultiplier = (float)atof(GetXMLAttribText(pInputNode, "ParticleLighting", "LightsMul", "1.0"));
    }

    {
        const char* pText = GetXMLAttribText(pInputNode, "VolFogShadows", "Enable", "false");
        const bool enable = !strcmp(pText, "true") || !strcmp(pText, "1");

        pText = GetXMLAttribText(pInputNode, "VolFogShadows", "EnableForClouds", "false");
        const bool enableForClouds = !strcmp(pText, "true") || !strcmp(pText, "1");

        SetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_ENABLE, Vec3(enable ? 1.0f : 0.0f, enableForClouds ? 1.0f : 0.0f, 0.0f));
    }

#if defined(FEATURE_SVO_GI)
    if (gEnv->pConsole->GetCVar("e_GI")->GetIVal())
    {
        LoadTISettings(pInputNode);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadTimeOfDaySettingsFromXML(XmlNodeRef node)
{
    if (node)
    {
        GetTimeOfDay()->Serialize(node, true);
        ITimeOfDay::SAdvancedInfo info;
        GetTimeOfDay()->GetAdvancedInfo(info);
        GetTimeOfDay()->SetTime(info.fStartTime, true);
    }
}

//! create static object containing empty IndexedMesh
IStatObj* C3DEngine::CreateStatObj()
{
    CStatObj* pStatObj = new CStatObj();
    pStatObj->m_pIndexedMesh = new CIndexedMesh();
    return pStatObj;
}

IStatObj* C3DEngine::CreateStatObjOptionalIndexedMesh(bool createIndexedMesh)
{
    CStatObj* pStatObj = new CStatObj();
    if (createIndexedMesh)
    {
        pStatObj->m_pIndexedMesh = new CIndexedMesh();
    }
    return pStatObj;
}

bool C3DEngine::RestoreTerrainFromDisk([[maybe_unused]] int nSID)
{
    ResetParticlesAndDecals();

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::LoadUsedShadersList()
{
    LOADING_TIME_PROFILE_SECTION;
    gEnv->pRenderer->EF_Query(EFQ_SetShaderCombinations);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::PrecreateDecals()
{
    LOADING_TIME_PROFILE_SECTION;

    CObjManager::DecalsToPrecreate& decals(GetObjManager()->GetDecalsToPrecreate());
    // pre-create ...
    if (GetCVars()->e_DecalsPreCreate)
    {
        CryLog("Pre-creating %d decals...", (int)decals.size());

        CObjManager::DecalsToPrecreate::iterator it(decals.begin());
        CObjManager::DecalsToPrecreate::iterator itEnd(decals.end());
        for (; it != itEnd; ++it)
        {
            IDecalRenderNode* pDecalRenderNode(*it);
            pDecalRenderNode->Precache();
        }

        CryLog(" done.\n");
    }
    else
    {
        CryLog("Skipped pre-creation of decals.\n");
    }

    // ... and discard list (even if pre-creation was skipped!)
    decals.resize(0);

    return true;
}

//////////////////////////////////////////////////////////////////////////
// Called by game when everything needed for level is loaded.
//////////////////////////////////////////////////////////////////////////
void C3DEngine::PostLoadLevel()
{
    LOADING_TIME_PROFILE_SECTION;

    CRY_ASSERT(m_levelLoaded == false);

    //////////////////////////////////////////////////////////////////////////
    // Submit water material to physics if the ocean exists
    //////////////////////////////////////////////////////////////////////////


    if (GetCVars()->e_PrecacheLevel)
    {
        // pre-create decals
        PrecreateDecals();
    }

    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_TEXTURES);

    GetRenderer()->PostLevelLoading();

    // refresh material constants pulled in from resources (such as textures)
    GetMatMan()->RefreshShaderResourceConstants();

    if (m_nGsmCache > 0)
    {
        m_CachedShadowsBounds.Reset();
        SetRecomputeCachedShadows(m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate);
    }
    m_levelLoaded = true;
}


int C3DEngine::SaveStatObj(IStatObj* pStatObj, TSerialize ser)
{
    if (!(pStatObj->GetFlags() & STATIC_OBJECT_GENERATED))
    {
        bool bVal = false;
        ser.Value("altered", bVal);
        ser.Value("file", pStatObj->GetFilePath());
        ser.Value("geom", pStatObj->GetGeoName());
    }
    else
    {
        bool bVal = true;
        ser.Value("altered", bVal);
        ser.Value("CloneSource", pStatObj->GetCloneSourceObject() ? pStatObj->GetCloneSourceObject()->GetFilePath() : "0");
        pStatObj->Serialize(ser);
    }

    return 1;
}

IStatObj* C3DEngine::LoadStatObj(TSerialize ser)
{
    bool bVal;
    IStatObj* pStatObj;
    ser.Value("altered", bVal);
    if (!bVal)
    {
        string fileName, geomName;
        ser.Value("file", fileName);
        ser.Value("geom", geomName);
        pStatObj = LoadStatObjUnsafeManualRef(fileName, geomName);
    }
    else
    {
        string srcObjName;
        ser.Value("CloneSource", srcObjName);
        if (*(const unsigned short*)(const char*)srcObjName != '0')
        {
            pStatObj = LoadStatObjUnsafeManualRef(srcObjName)->Clone(false, false, true);
        }
        else
        {
            pStatObj = CreateStatObj();
        }
        pStatObj->Serialize(ser);
    }
    return pStatObj;
}




