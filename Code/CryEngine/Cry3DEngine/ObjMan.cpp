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

// Description : Loading trees, buildings, register/unregister entities for rendering


#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "ObjectsTree.h"
#include <IResourceManager.h>
#include "DecalRenderNode.h"
#include <CryPath.h>
#include <cctype>

#include <StatObjBus.h>

#include <LoadScreenBus.h>

#define BRUSH_LIST_FILE "brushlist.txt"
#define CGF_LEVEL_CACHE_PAK "cgf.pak"

namespace BoneNames
{
    const char* Cloth = "cloth";
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CObjManager::GetStaticObjectByTypeID(int nTypeID, int nSID)
{
    assert(nSID >= 0 && nSID < m_lstStaticTypes.Count());

    if (nTypeID >= 0 && nTypeID < m_lstStaticTypes[nSID].Count())
    {
        return m_lstStaticTypes[nSID][nTypeID].pStatObj;
    }

    return 0;
}

IStatObj* CObjManager::FindStaticObjectByFilename(const char* filename)
{
    string lowerFileName(filename);
    return stl::find_in_map(m_nameToObjectMap, CONST_TEMP_STRING(lowerFileName.MakeLower()), NULL);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::UnloadObjects(bool bDeleteAll)
{

    CleanStreamingData();

    m_pRMBox = 0;

    m_decalsToPrecreate.resize(0);

    // Clear all objects that are in the garbage collector.
    ClearStatObjGarbage();

    stl::free_container(m_checkForGarbage);
    m_bGarbageCollectionEnabled = false;

    if (bDeleteAll)
    {
        {
            AZStd::unique_lock<AZStd::recursive_mutex> loadLock(m_loadMutex);

            m_lockedObjects.clear(); // Lock/Unlock resources will not work with this.

            // Release default stat obj.
            m_pDefaultCGF = 0;

            m_nameToObjectMap.clear();
            m_lstLoadedObjects.clear();
        }

        int nNumLeaks = 0;
        std::vector<CStatObj*> garbage;
        for (CStatObj* pStatObj = CStatObj::get_intrusive_list_root(); pStatObj; pStatObj = pStatObj->m_next_intrusive)
        {
            garbage.push_back(pStatObj);

#if !defined(_RELEASE)
            if (!pStatObj->IsDefaultObject())
            {
                nNumLeaks++;
                Warning("StatObj not deleted: %s (%s)  RefCount: %d", pStatObj->m_szFileName.c_str(), pStatObj->m_szGeomName.c_str(), pStatObj->m_nUsers);
            }
#endif //_RELEASE
        }

#ifndef _RELEASE
        // deleting leaked objects
        if (nNumLeaks > 0)
        {
            Warning("CObjManager::CheckObjectLeaks: %d object(s) found in memory", nNumLeaks);
        }
#endif //_RELEASE

        for (int i = 0, num = (int)garbage.size(); i < num; i++)
        {
            CStatObj* pStatObj = garbage[i];
            pStatObj->ShutDown();
        }
        for (int i = 0, num = (int)garbage.size(); i < num; i++)
        {
            CStatObj* pStatObj = garbage[i];
            delete pStatObj;
        }

#ifdef POOL_STATOBJ_ALLOCS
        assert(m_statObjPool->GetTotalMemory().nUsed == 0);
#endif
    }
    m_bGarbageCollectionEnabled = true;

#ifdef POOL_STATOBJ_ALLOCS
    m_statObjPool->FreeMemoryIfEmpty();
#endif

    //If this collection is not cleared on unload then character materials will
    //leak and most likely crash the engine across level loads.
    stl::free_container(m_collectedMaterials);
    stl::free_container(m_decalsToPrecreate);
    stl::free_container(m_tmpAreas0);
    stl::free_container(m_tmpAreas1);
    for (int nSID = 0; nSID < m_lstStaticTypes.Count(); nSID++)
    {
        m_lstStaticTypes[nSID].Free();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::CleanStreamingData()
{
    stl::free_container(m_arrStreamingNodeStack);

    stl::free_container(m_arrStreamableToRelease);
    stl::free_container(m_arrStreamableToLoad);
    stl::free_container(m_arrStreamableToDelete);
}

//////////////////////////////////////////////////////////////////////////
// class for asyncronous preloading of level CGF's
//////////////////////////////////////////////////////////////////////////
struct CLevelStatObjLoader
    : public IStreamCallback
    , public Cry3DEngineBase
{
    int m_nTasksNum;

    CLevelStatObjLoader()
    {
        m_nTasksNum = 0;
    }

    void StartStreaming(const char* pFileName)
    {
        m_nTasksNum++;

        // request the file
        StreamReadParams params;
        params.dwUserData = 0;
        params.nSize = 0;
        params.pBuffer = NULL;
        params.nLoadTime = 0;
        params.nMaxLoadTime = 0;
        params.ePriority = estpUrgent;
        GetSystem()->GetStreamEngine()->StartRead(eStreamTaskTypeGeometry, pFileName, this, &params);
    }

    virtual void StreamOnComplete (IReadStream* pStream, unsigned nError)
    {
        if (!nError)
        {
            string szName = pStream->GetName();
            // remove game folder from path
            const char* szInGameName = strstr(szName, "\\");
            // load CGF from memory
            GetObjManager()->LoadStatObjUnsafeManualRef(szInGameName + 1, NULL, NULL, true, 0, pStream->GetBuffer(), pStream->GetBytesRead());
        }

        m_nTasksNum--;
    }
};

//////////////////////////////////////////////////////////////////////////
// Preload in efficient way all CGF's used in level
//////////////////////////////////////////////////////////////////////////
void CObjManager::PreloadLevelObjects()
{
    LOADING_TIME_PROFILE_SECTION;

    // Starting a new level, so make sure the round ids are ahead of what they were in the last level
    m_nUpdateStreamingPrioriryRoundId += 8;
    m_nUpdateStreamingPrioriryRoundIdFast += 8;

    PrintMessage("Starting loading level CGF's ...");
    INDENT_LOG_DURING_SCOPE();

    float fStartTime = GetCurAsyncTimeSec();

    bool bCgfCacheExist = false;
    if (GetCVars()->e_StreamCgf != 0)
    {
        // Only when streaming enable use no-mesh cgf pak.
        //bCgfCacheExist = GetISystem()->GetIResourceManager()->LoadLevelCachePak( CGF_LEVEL_CACHE_PAK,"" );
    }
    auto pResList = GetISystem()->GetIResourceManager()->GetLevelResourceList();

    // Construct streamer object
    CLevelStatObjLoader cgfStreamer;

    CryPathString cgfFilename;
    int nCgfCounter = 0;
    int nInLevelCacheCount = 0;

    bool bVerboseLogging = GetCVars()->e_StatObjPreload > 1;


    //////////////////////////////////////////////////////////////////////////
    // Enumerate all .CGF inside level from the "brushlist.txt" file.
    {
        string brushListFilename = Get3DEngine()->GetLevelFilePath(BRUSH_LIST_FILE);
        CCryFile file;
        if (file.Open(brushListFilename.c_str(), "rb") && file.GetLength() > 0)
        {
            int nFileLength = file.GetLength();
            char* buf = new char[nFileLength + 1];
            buf[nFileLength] = 0; // Null terminate
            file.ReadRaw(buf, nFileLength);

            // Parse file, every line in a file represents a resource filename.
            char seps[] = "\r\n";
            char *next_token = nullptr;
            char* token = azstrtok(buf, 0, seps, &next_token);
            while (token != NULL)
            {
                int nAliasLen = sizeof("%level%") - 1;
                if (strncmp(token, "%level%", nAliasLen) == 0)
                {
                    cgfFilename = Get3DEngine()->GetLevelFilePath(token + nAliasLen);
                }
                else
                {
                    cgfFilename = token;
                }

                if (bVerboseLogging)
                {
                    CryLog("%s", cgfFilename.c_str());
                }
                // Do not use streaming for the Brushes from level.pak.
                GetObjManager()->LoadStatObjUnsafeManualRef(cgfFilename.c_str(), NULL, 0, false, 0);
                //cgfStreamer.StartStreaming(cgfFilename.c_str());
                nCgfCounter++;

                token = azstrtok(NULL, 0, seps, &next_token);

                //This loop can take a few seconds, so we should refresh the loading screen and call the loading tick functions to ensure that no big gaps in coverage occur.
                SYNCHRONOUS_LOADING_TICK();
            }
            delete []buf;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Request objects loading from Streaming System.
    if (pResList)
    {
        if (const char* pCgfName = pResList->GetFirst())
        {
            while (pCgfName)
            {
                if (strstr(pCgfName, ".cgf"))
                {
                    const char* sLodName = strstr(pCgfName, "_lod");
                    if (sLodName && (sLodName[4] >= '0' && sLodName[4] <= '9'))
                    {
                        // Ignore Lod files.
                        pCgfName = pResList->GetNext();
                        continue;
                    }

                    cgfFilename = pCgfName;

                    if (bVerboseLogging)
                    {
                        CryLog("%s", cgfFilename.c_str());
                    }
                    IStatObj* pStatObj = GetObjManager()->LoadStatObjUnsafeManualRef(cgfFilename.c_str(), NULL, 0, true, 0);
                    if (pStatObj)
                    {
                        if (pStatObj->IsMeshStrippedCGF())
                        {
                            nInLevelCacheCount++;
                        }
                    }
                    // cgfStreamer.StartStreaming(cgfFilename.c_str());
                    nCgfCounter++;

                    // This loop can take a few seconds, so we should refresh the loading screen and call the loading tick functions to
                    // ensure that no big gaps in coverage occur.
                    SYNCHRONOUS_LOADING_TICK();
                }

                pCgfName = pResList->GetNext();
            }
        }
    }

    //  PrintMessage("Finished requesting level CGF's: %d objects in %.1f sec", nCgfCounter, GetCurAsyncTimeSec()-fStartTime);

    // Continue updating streaming system until all CGF's are loaded
    if (cgfStreamer.m_nTasksNum > 0)
    {
        LOADING_TIME_PROFILE_SECTION_NAMED("CObjManager::PreloadLevelObjects_StreamEngine_Update");
        GetSystem()->GetStreamEngine()->UpdateAndWait();
    }

    if (bCgfCacheExist)
    {
        //GetISystem()->GetIResourceManager()->UnloadLevelCachePak( CGF_LEVEL_CACHE_PAK );
    }

    float dt = GetCurAsyncTimeSec() - fStartTime;
    PrintMessage("Finished loading level CGF's: %d objects loaded (%d from LevelCache) in %.1f sec", nCgfCounter, nInLevelCacheCount, dt);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create / delete object
//////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
T CObjManager::LoadStatObjInternal(const char* filename,
    const char* _szGeomName,
    IStatObj::SSubObject** ppSubObject,
    bool bUseStreaming,
    unsigned long nLoadingFlags,
    const void* pData,
    int nDataSize,
    [[maybe_unused]] const char* szBlockName)
{
    LoadDefaultCGF(filename, nLoadingFlags);

    LOADING_TIME_PROFILE_SECTION;
    
    if (ppSubObject)
    {
        *ppSubObject = NULL;
    }

    AZStd::unique_lock<AZStd::recursive_mutex> loadLock(m_loadMutex);

    if (!strcmp(filename, "NOFILE"))
    { // make empty object to be filled from outside
        CStatObj* pObject = new CStatObj();
        m_lstLoadedObjects.insert(pObject);

        return pObject;
    }

    // Normalize file name
    // Remap %level% alias if needed and unify filename
    char normalizedFilename[_MAX_PATH];
    NormalizeLevelName(filename, normalizedFilename);

    bool bForceBreakable = strstr(normalizedFilename, "break") != 0;
    if (_szGeomName && !strcmp(_szGeomName, "#ForceBreakable"))
    {
        bForceBreakable = true;
        _szGeomName = 0;
    }

    // Try to find already loaded object
    IStatObj* pObject = 0;

    int flagCloth = 0;
    if (_szGeomName && !strcmp(_szGeomName, BoneNames::Cloth))
    {
        _szGeomName = 0;
        flagCloth = STATIC_OBJECT_DYNAMIC | STATIC_OBJECT_CLONE;
    }
    else
    {
        // This branch needs to be handled carefully to avoid returning an object that is in the process of being deleted by ClearStatObjGarbage on another thread
        // It is important that ClearStatObjGarbage is not run during this time (done via m_loadMutex)

        AZStd::string lowerFileName(normalizedFilename);
        int(* toLowerFunc)(int) = std::tolower; //Needed to help the compiler figure out the right tolower to use on android
        AZStd::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), toLowerFunc);

        pObject = stl::find_in_map(m_nameToObjectMap, CONST_TEMP_STRING(lowerFileName.c_str()), NULL);
        if (pObject)
        {
            assert(!pData);

            return (T)LoadFromCacheNoRef(pObject, bUseStreaming, nLoadingFlags, _szGeomName, ppSubObject);
        }
    }

    // Load new CGF
    return LoadNewCGF(pObject, flagCloth, bUseStreaming, bForceBreakable, nLoadingFlags, normalizedFilename, pData, nDataSize, filename, _szGeomName, ppSubObject);
}

IStatObj* CObjManager::LoadStatObjUnsafeManualRef(const char* filename,
    const char* _szGeomName,
    IStatObj::SSubObject** ppSubObject,
    bool bUseStreaming,
    unsigned long nLoadingFlags,
    const void* pData,
    int nDataSize,
    const char* szBlockName)
{
    return LoadStatObjInternal<IStatObj*>(filename, _szGeomName, ppSubObject, bUseStreaming, nLoadingFlags, pData, nDataSize, szBlockName);
}

_smart_ptr<IStatObj> CObjManager::LoadStatObjAutoRef(const char* filename,
    const char* _szGeomName,
    IStatObj::SSubObject** ppSubObject,
    bool bUseStreaming,
    unsigned long nLoadingFlags,
    const void* pData,
    int nDataSize,
    const char* szBlockName)
{
    return LoadStatObjInternal<_smart_ptr<IStatObj> >(filename, _szGeomName, ppSubObject, bUseStreaming, nLoadingFlags, pData, nDataSize, szBlockName);
}

template <size_t SIZE_IN_CHARS>
void CObjManager::NormalizeLevelName(const char* filename, char(&normalizedFilename)[SIZE_IN_CHARS])
{
    const int nAliasNameLen = sizeof("%level%") - 1;

    if (strncmp(filename, "%level%", nAliasNameLen) == 0)
    {
        cry_strcpy(normalizedFilename, Get3DEngine()->GetLevelFilePath(filename + nAliasNameLen));
    }
    else
    {
        cry_strcpy(normalizedFilename, filename);
    }

    PREFAST_SUPPRESS_WARNING(6054) // normalizedFilename is null terminated
    std::replace(normalizedFilename, normalizedFilename + strlen(normalizedFilename), '\\', '/'); // To Unix Path
}

void CObjManager::LoadDefaultCGF(const char* filename, unsigned long nLoadingFlags)
{
    string fixedFileName(filename);
    fixedFileName = PathUtil::ToUnixPath(fixedFileName);

    if (!m_pDefaultCGF && azstricmp(fixedFileName.c_str(), DEFAULT_CGF_NAME) != 0)
    {
        // Load default object if not yet loaded.
        const char* sDefaulObjFilename = DEFAULT_CGF_NAME;
        // prepare default object
        m_pDefaultCGF = LoadStatObjUnsafeManualRef(sDefaulObjFilename, NULL, NULL, false, nLoadingFlags);
        if (!m_pDefaultCGF)
        {
            Error("CObjManager::LoadStatObj: Default object not found (%s)", sDefaulObjFilename);
            m_pDefaultCGF = new CStatObj();
        }
        m_pDefaultCGF->SetDefaultObject(true);
    }
}

IStatObj* CObjManager::LoadNewCGF(IStatObj* pObject, int flagCloth, bool bUseStreaming, bool bForceBreakable, unsigned long nLoadingFlags, const char* normalizedFilename, const void* pData, int nDataSize, const char* originalFilename, const char* geomName, IStatObj::SSubObject** ppSubObject)
{
    pObject = new CStatObj();
    pObject->SetFlags(pObject->GetFlags() | flagCloth);

    bUseStreaming &= (GetCVars()->e_StreamCgf != 0);

    if (bUseStreaming)
    {
        pObject->SetCanUnload(true);
    }
    if (bForceBreakable)
    {
        nLoadingFlags |= IStatObj::ELoadingFlagsForceBreakable;
    }

    if (!pObject->LoadCGF(normalizedFilename, strstr(normalizedFilename, "_lod") != NULL, nLoadingFlags, pData, nDataSize))
    {
        Error("Failed to load cgf: %s", originalFilename);
        // object not found
        // if geom name is specified - just return 0
        if (geomName && geomName[0])
        {
            delete pObject;
            return nullptr;
        }

        // make unique default CGF for every case of missing CGF, this will make export process more reliable and help finding missing CGF's in pure game
        /*      pObject->m_bCanUnload = false;
        if (m_bEditor && pObject->LoadCGF( DEFAULT_CGF_NAME, false, nLoadingFlags, pData, nDataSize ))
        {
        pObject->m_szFileName = sFilename;
        pObject->m_bDefaultObject = true;
        }
        else*/
        {
            delete pObject;
            return m_pDefaultCGF;
        }
    }

    // now try to load lods
    if (!pObject->AreLodsLoaded())
    {
        pObject->LoadLowLODs(bUseStreaming, nLoadingFlags);
    }

    if (!pObject->IsUnloadable())
    { // even if streaming is disabled we register object for potential streaming (streaming system will never unload it)
        pObject->DisableStreaming();
    }

    // sub meshes merging
    pObject->TryMergeSubObjects(false);

    m_lstLoadedObjects.insert(pObject);
    m_nameToObjectMap[pObject->GetFileName().MakeLower()] = pObject;

    if (geomName && geomName[0])
    {
        // Return SubObject.
        CStatObj::SSubObject* pSubObject = pObject->FindSubObject(geomName);
        if (!pSubObject || !pSubObject->pStatObj)
        {
            return 0;
        }
        if (pSubObject->pStatObj)
        {
            if (ppSubObject)
            {
                *ppSubObject = pSubObject;
            }
            return (CStatObj*)pSubObject->pStatObj;
        }
    }

#if AZ_LOADSCREENCOMPONENT_ENABLED
    if (GetISystem() && GetISystem()->GetGlobalEnvironment() && GetISystem()->GetGlobalEnvironment()->mMainThreadId == CryGetCurrentThreadId())
    {
        EBUS_EVENT(LoadScreenBus, UpdateAndRender);
    }
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
    return pObject;
}

IStatObj* CObjManager::LoadFromCacheNoRef(IStatObj* pObject, bool bUseStreaming, unsigned long nLoadingFlags, const char* geomName, IStatObj::SSubObject** ppSubObject)
{
    if (!bUseStreaming && pObject->IsUnloadable())
    {
        pObject->DisableStreaming();
    }

    if (!pObject->AreLodsLoaded())
    {
        pObject->LoadLowLODs(bUseStreaming, nLoadingFlags);
    }

    if (geomName && geomName[0])
    {
        // Return SubObject.
        IStatObj::SSubObject* pSubObject = pObject->FindSubObject(geomName);
        if (!pSubObject || !pSubObject->pStatObj)
        {
            return nullptr;
        }

        if (ppSubObject)
        {
            *ppSubObject = pSubObject;
        }

        // UnregisterForGarbage needs to be called on pObject before returning
        UnregisterForGarbage(pObject);

        return (CStatObj*)pSubObject->pStatObj;
    }

    UnregisterForGarbage(pObject);

    return pObject;
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::InternalDeleteObject(IStatObj* pObject)
{
    assert(pObject);

    AZStd::lock_guard<AZStd::recursive_mutex> loadLock(m_loadMutex);

    if (!m_bLockCGFResources && !IsResourceLocked(pObject->GetFileName()))
    {
        LoadedObjects::iterator it = m_lstLoadedObjects.find(pObject);
        if (it != m_lstLoadedObjects.end())
        {
            m_lstLoadedObjects.erase(it);
            m_nameToObjectMap.erase(pObject->GetFileName().MakeLower());
        }
        else
        {
            //Warning( "CObjManager::ReleaseObject called on object not loaded in ObjectManager %s",pObject->m_szFileName.c_str() );
            //return false;
        }

        delete pObject;
        return true;
    }
    else if (m_bLockCGFResources)
    {
        // Put them to locked stat obj list.
        stl::push_back_unique(m_lockedObjects, pObject);
    }

    return false;
}

IStatObj* CObjManager::AllocateStatObj()
{
#ifdef POOL_STATOBJ_ALLOCS
    return (IStatObj*)m_statObjPool->Allocate();
#else
    return (IStatObj*)malloc(sizeof(CStatObj));
#endif
}

void CObjManager::FreeStatObj(IStatObj* pObj)
{
#ifdef POOL_STATOBJ_ALLOCS
    m_statObjPool->Deallocate(pObj);
#else
    free(pObj);
#endif
}

CObjManager::CObjManager()
    : m_pDefaultCGF (NULL)
    , m_decalsToPrecreate()
    , m_bNeedProcessObjectsStreaming_Finish(false)
    , m_CullThread()
    , m_fGSMMaxDistance(0)
    , m_bLockCGFResources(false)
    , m_sunAnimIndex(0)
    , m_sunAnimSpeed(0)
    , m_sunAnimPhase(0)
    , m_nNextPrecachePointId(0)
    , m_bCameraPrecacheOverridden(false)
    , m_fILMul(1.f)
    , m_fSSAOAmount(1.f)
    , m_fSSAOContrast(1.f)
    , m_pRMBox(nullptr)
    , m_bGarbageCollectionEnabled(true)
{
#ifdef POOL_STATOBJ_ALLOCS
    m_statObjPool = new stl::PoolAllocator<sizeof(CStatObj), stl::PSyncMultiThread, alignof(CStatObj)>(stl::FHeap().PageSize(64)); // 20Kb per page
#endif

    m_vStreamPreCachePointDefs.Add(SObjManPrecachePoint());
    m_vStreamPreCacheCameras.Add(SObjManPrecacheCamera());

    m_pObjManager = this;

    m_vSunColor.Set(0, 0, 0);
    m_rainParams.nUpdateFrameID = -1;
    m_rainParams.fAmount = 0.f;
    m_rainParams.fRadius = 1.f;
    m_rainParams.vWorldPos.Set(0, 0, 0);
    m_rainParams.vColor.Set(1, 1, 1);
    m_rainParams.fFakeGlossiness = 0.5f;
    m_rainParams.fFakeReflectionAmount = 1.5f;
    m_rainParams.fDiffuseDarkening = 0.5f;
    m_rainParams.fRainDropsAmount = 0.5f;
    m_rainParams.fRainDropsSpeed = 1.f;
    m_rainParams.fRainDropsLighting = 1.f;
    m_rainParams.fMistAmount = 3.f;
    m_rainParams.fMistHeight = 8.f;
    m_rainParams.fPuddlesAmount = 1.5f;
    m_rainParams.fPuddlesMaskAmount = 1.0f;
    m_rainParams.fPuddlesRippleAmount = 2.0f;
    m_rainParams.fSplashesAmount = 1.3f;
    m_rainParams.bIgnoreVisareas = false;
    m_rainParams.bDisableOcclusion = false;


#ifdef SUPP_HWOBJ_OCCL
    if (GetRenderer()->GetFeatures() & RFT_OCCLUSIONTEST)
    {
        m_pShaderOcclusionQuery = GetRenderer()->EF_LoadShader("OcclusionTest");
    }
    else
    {
        m_pShaderOcclusionQuery = 0;
    }
#endif
    
    m_decalsToPrecreate.reserve(128);

    // init queue for check occlusion
    m_CheckOcclusionQueue.Init(GetCVars()->e_CheckOcclusionQueueSize);
    m_CheckOcclusionOutputQueue.Init(GetCVars()->e_CheckOcclusionOutputQueueSize);

    StatInstGroupEventBus::Handler::BusConnect();
}

// make unit box for occlusion test
void CObjManager::MakeUnitCube()
{
    if (m_pRMBox)
    {
        return;
    }

    SVF_P3F_C4B_T2F arrVerts[8];
    arrVerts[0].xyz = Vec3(0, 0, 0);
    arrVerts[1].xyz = Vec3(1, 0, 0);
    arrVerts[2].xyz = Vec3(0, 0, 1);
    arrVerts[3].xyz = Vec3(1, 0, 1);
    arrVerts[4].xyz = Vec3(0, 1, 0);
    arrVerts[5].xyz = Vec3(1, 1, 0);
    arrVerts[6].xyz = Vec3(0, 1, 1);
    arrVerts[7].xyz = Vec3(1, 1, 1);

    //      6-------7
    //   /         /|
    //  2-------3   |
    //  |        |  |
    //  |   4    | 5
    //  |        |/
    //  0-------1

    static const vtx_idx arrIndices[] =
    {
        // front + back
        1, 0, 2,
        2, 3, 1,
        5, 6, 4,
        5, 7, 6,
        // left + right
        0, 6, 2,
        0, 4, 6,
        1, 3, 7,
        1, 7, 5,
        // top + bottom
        3, 2, 6,
        6, 7, 3,
        1, 4, 0,
        1, 5, 4
    };

    m_pRMBox = GetRenderer()->CreateRenderMeshInitialized(
            arrVerts,
            sizeof(arrVerts) / sizeof(arrVerts[0]),
            eVF_P3F_C4B_T2F,
            arrIndices,
            sizeof(arrIndices) / sizeof(arrIndices[0]),
            prtTriangleList,
            "OcclusionQueryCube", "OcclusionQueryCube",
            eRMT_Static);

    m_pRMBox->SetChunk(NULL, 0, sizeof(arrVerts) / sizeof(arrVerts[0]), 0, sizeof(arrIndices) / sizeof(arrIndices[0]), 1.0f, eVF_P3F_C4B_T2F, 0);

    m_bGarbageCollectionEnabled = true;
}

CObjManager::~CObjManager()
{
    StatInstGroupEventBus::Handler::BusDisconnect();

    // free default object
    m_pDefaultCGF = 0;

    // free brushes
    /*  assert(!m_lstBrushContainer.Count());
        for(int i=0; i<m_lstBrushContainer.Count(); i++)
        {
        if(m_lstBrushContainer[i]->GetEntityStatObj())
          ReleaseObject((CStatObj*)m_lstBrushContainer[i]->GetEntityStatObj());
            delete m_lstBrushContainer[i];
        }
        m_lstBrushContainer.Reset();
    */
    UnloadObjects(true);
#ifdef POOL_STATOBJ_ALLOCS
    delete m_statObjPool;
#endif
}


// mostly xy size
float CObjManager::GetXYRadius(int type, int nSID)
{
    assert(nSID >= 0 && nSID < m_lstStaticTypes.Count());

    if ((m_lstStaticTypes[nSID].Count() <= type || !m_lstStaticTypes[nSID][type].pStatObj))
    {
        return 0.0f;
    }

    Vec3 vSize = m_lstStaticTypes[nSID][type].pStatObj->GetBoxMax() - m_lstStaticTypes[nSID][type].pStatObj->GetBoxMin();
    vSize.z *= 0.5f;

    float fXYRadius = vSize.GetLength() * 0.5f;

    return fXYRadius;
}

bool CObjManager::GetStaticObjectBBox(int nType, Vec3& vBoxMin, Vec3& vBoxMax, int nSID)
{
    assert(nSID >= 0 && nSID < m_lstStaticTypes.Count());

    if ((m_lstStaticTypes[nSID].Count() <= nType || !m_lstStaticTypes[nSID][nType].pStatObj))
    {
        return false;
    }

    vBoxMin = m_lstStaticTypes[nSID][nType].pStatObj->GetBoxMin();
    vBoxMax = m_lstStaticTypes[nSID][nType].pStatObj->GetBoxMax();

    return true;
}


void CObjManager::AddDecalToRenderer(float fDistance,
    _smart_ptr<IMaterial> pMat,
    const uint8 sortPrio,
    Vec3 right,
    Vec3 up,
    const UCol& ucResCol,
    [[maybe_unused]] const uint8 uBlendType,
    const Vec3& vAmbientColor,
    Vec3 vPos,
    const int nAfterWater,
    const SRenderingPassInfo& passInfo,
    const SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    // repeated objects are free imedeately in renderer
    CRenderObject* pOb(GetIdentityCRenderObject(passInfo.ThreadID()));
    if (!pOb)
    {
        return;
    }

    // prepare render object
    pOb->m_fDistance = fDistance;
    pOb->m_nTextureID = -1;
    pOb->m_fAlpha = (float)ucResCol.bcolor[3] / 255.f;
    pOb->m_II.m_AmbColor = vAmbientColor;
    pOb->m_fSort = 0;
    pOb->m_ObjFlags |= FOB_DECAL;
    pOb->m_nSort = sortPrio;

    SVF_P3F_C4B_T2F pVerts[4];
    uint16 pIndices[6];

    // TODO: determine whether this is a decal on opaque or transparent geometry
    // (put it in the respective renderlist for correct shadowing)
    // fill general vertex data
    pVerts[0].xyz = (-right - up) + vPos;
    pVerts[0].st = Vec2(0, 1);
    pVerts[0].color.dcolor = ~0;

    pVerts[1].xyz = (right - up) + vPos;
    pVerts[1].st = Vec2(1, 1);
    pVerts[1].color.dcolor = ~0;

    pVerts[2].xyz = (right + up) + vPos;
    pVerts[2].st = Vec2(1, 0);
    pVerts[2].color.dcolor = ~0;

    pVerts[3].xyz = (-right + up) + vPos;
    pVerts[3].st = Vec2(0, 0);
    pVerts[3].color.dcolor = ~0;

    // prepare tangent space (tangent, bitangent) and fill it in
    Vec3 rightUnit(right.GetNormalized());
    Vec3 upUnit(up.GetNormalized());

    SPipTangents pTangents[4];

    pTangents[0] = SPipTangents(rightUnit, -upUnit, -1);
    pTangents[1] = pTangents[0];
    pTangents[2] = pTangents[0];
    pTangents[3] = pTangents[0];

    // fill decals topology (two triangles)
    pIndices[0] = 0;
    pIndices[1] = 1;
    pIndices[2] = 2;

    pIndices[3] = 0;
    pIndices[4] = 2;
    pIndices[5] = 3;

    GetRenderer()->EF_AddPolygonToScene(pMat->GetShaderItem(), 4, pVerts, pTangents, pOb, passInfo, pIndices, 6, nAfterWater, rendItemSorter);
}


void CObjManager::GetMemoryUsage(class ICrySizer* pSizer) const
{
    {
        SIZER_COMPONENT_NAME(pSizer, "Self");
        pSizer->AddObject(this, sizeof(*this));
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "StaticTypes");
        pSizer->AddObject(m_lstStaticTypes);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "CMesh");
        CIndexedMesh* pMesh = CIndexedMesh::get_intrusive_list_root();
        while (pMesh)
        {
            pSizer->AddObject(pMesh);
            pMesh = pMesh->m_next_intrusive;
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "StatObj");
        for (CStatObj* pStatObj = CStatObj::get_intrusive_list_root(); pStatObj; pStatObj = pStatObj->m_next_intrusive)
        {
            pStatObj->GetMemoryUsage(pSizer);
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "EmptyNodes");
        pSizer->AddObject(COctreeNode::m_arrEmptyNodes);
    }
}

// retrieves the bandwidth calculations for the audio streaming
void CObjManager::GetBandwidthStats([[maybe_unused]] float* fBandwidthRequested)
{
#if !defined (_RELEASE)
    if (fBandwidthRequested && CStatObj::s_fStreamingTime != 0.0f)
    {
        *fBandwidthRequested = (CStatObj::s_nBandwidth / CStatObj::s_fStreamingTime) / 1024.0f;
    }
#endif
}

void CObjManager::ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax)
{
    PodArray<SRNInfo> lstEntitiesInArea;

    AABB vBoxAABB(vBoxMin, vBoxMax);

    Get3DEngine()->MoveObjectsIntoListGlobal(&lstEntitiesInArea, &vBoxAABB, true);

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->MoveObjectsIntoList(&lstEntitiesInArea, vBoxAABB, true);
    }

    int nChanged = 0;
    for (int i = 0; i < lstEntitiesInArea.Count(); i++)
    {
        IVisArea* pPrevArea = lstEntitiesInArea[i].pNode->GetEntityVisArea();
        Get3DEngine()->UnRegisterEntityDirect(lstEntitiesInArea[i].pNode);

        if (lstEntitiesInArea[i].pNode->GetRenderNodeType() == eERType_Decal)
        {
            ((CDecalRenderNode*)lstEntitiesInArea[i].pNode)->RequestUpdate();
        }

        Get3DEngine()->RegisterEntity(lstEntitiesInArea[i].pNode);
        if (pPrevArea != lstEntitiesInArea[i].pNode->GetEntityVisArea())
        {
            nChanged++;
        }
    }
}


void CObjManager::FreeNotUsedCGFs()
{
    AZStd::vector<CStatObj*> garbageList;

    {
        AZStd::lock_guard<AZStd::recursive_mutex> loadLock(m_loadMutex);

        m_lockedObjects.clear();

        if (!m_bLockCGFResources)
        {
            //Timur, You MUST use next here, or with erase you invalidating
            LoadedObjects::iterator next;
            for (LoadedObjects::iterator it = m_lstLoadedObjects.begin(); it != m_lstLoadedObjects.end(); it = next)
            {
                next = it;
                ++next;
                CStatObj* p = (CStatObj*)(*it);
                if (p->m_nUsers <= 0)
                {
                    // Add to the list and run CheckForGarbage afterwards so we don't have to hold two locks at once
                    garbageList.push_back(p);
                }
            }
        }
    }

    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_garbageMutex);
        for (auto* object : garbageList)
        {
            CheckForGarbage(object);
        }
    }

    ClearStatObjGarbage();
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount)
{
    AZStd::lock_guard<AZStd::recursive_mutex> loadLock(m_loadMutex);

    if (!pObjectsArray)
    {
        nCount = m_lstLoadedObjects.size();
        return;
    }

    CObjManager::LoadedObjects::iterator it = m_lstLoadedObjects.begin();
    for (int i = 0; i < nCount && it != m_lstLoadedObjects.end(); i++, it++)
    {
        pObjectsArray[i] = *it;
    }
}

void StatInstGroup::Update([[maybe_unused]] CVars* pCVars, [[maybe_unused]] int nGeomDetailScreenRes)
{
    m_dwRndFlags = 0;

    static ICVar* pObjShadowCastSpec = gEnv->pConsole->GetCVar("e_ObjShadowCastSpec");
    if (nCastShadowMinSpec <= pObjShadowCastSpec->GetIVal())
    {
        m_dwRndFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
    }

    if (bDynamicDistanceShadows)
    {
        m_dwRndFlags |= ERF_DYNAMIC_DISTANCESHADOWS;
    }
    if (bHideability)
    {
        m_dwRndFlags |= ERF_HIDABLE;
    }
    if (bHideabilitySecondary)
    {
        m_dwRndFlags |= ERF_HIDABLE_SECONDARY;
    }
    if (!bAllowIndoor)
    {
        m_dwRndFlags |= ERF_OUTDOORONLY;
    }

    uint32 nSpec = (uint32)minConfigSpec;
    if (nSpec != 0)
    {
        m_dwRndFlags &= ~ERF_SPEC_BITS_MASK;
        m_dwRndFlags |= (nSpec << ERF_SPEC_BITS_SHIFT) & ERF_SPEC_BITS_MASK;
    }

    if (GetStatObj())
    {
        fVegRadiusVert = GetStatObj()->GetRadiusVert();
        fVegRadiusHor = GetStatObj()->GetRadiusHors();
        fVegRadius = max(fVegRadiusVert, fVegRadiusHor);
    }
    else
    {
        fVegRadiusHor = fVegRadius = fVegRadiusVert = 0;
    }

#if defined(FEATURE_SVO_GI)
    _smart_ptr<IMaterial> pMat = pMaterial ? pMaterial : (pStatObj ? pStatObj->GetMaterial() : 0);
    if (pMat && (gEnv->pConsole->GetCVar("e_svoTI_Active") && 
        gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal() && 
        gEnv->pConsole->GetCVar("e_GI")->GetIVal()))
    {
        pMat->SetKeepLowResSysCopyForDiffTex();
    }
#endif
}

bool CObjManager::SphereRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const float fRadius, _smart_ptr<IMaterial> pMat)
{
    FUNCTION_PROFILER_3DENGINE;

    // get position offset and stride
    int nPosStride = 0;
    byte* pPos  = pRenderMesh->GetPosPtr(nPosStride, FSL_READ);

    // get indices
    vtx_idx* pInds = pRenderMesh->GetIndexPtr(FSL_READ);
    int nInds = pRenderMesh->GetIndicesCount();
    assert(nInds % 3 == 0);

    // test tris
    TRenderChunkArray& Chunks = pRenderMesh->GetChunks();
    for (int nChunkId = 0; nChunkId < Chunks.size(); nChunkId++)
    {
        CRenderChunk* pChunk = &Chunks[nChunkId];
        if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        {
            continue;
        }

        // skip transparent and alpha test
        if (pMat)
        {
            const SShaderItem& shaderItem = pMat->GetShaderItem(pChunk->m_nMatID);
            if (!shaderItem.m_pShader || shaderItem.m_pShader->GetFlags() & EF_NODRAW)
            {
                continue;
            }
        }

        int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
        for (int i = pChunk->nFirstIndexId; i < nLastIndexId; i += 3)
        {
            assert((int)pInds[i + 0] < pRenderMesh->GetVerticesCount());
            assert((int)pInds[i + 1] < pRenderMesh->GetVerticesCount());
            assert((int)pInds[i + 2] < pRenderMesh->GetVerticesCount());

            // get triangle vertices
            Vec3 v0 = (*(Vec3*)&pPos[nPosStride * pInds[i + 0]]);
            Vec3 v1 = (*(Vec3*)&pPos[nPosStride * pInds[i + 1]]);
            Vec3 v2 = (*(Vec3*)&pPos[nPosStride * pInds[i + 2]]);

            AABB triBox;
            triBox.min = v0;
            triBox.max = v0;
            triBox.Add(v1);
            triBox.Add(v2);

            if (Overlap::Sphere_AABB(Sphere(vInPos, fRadius), triBox))
            {
                return true;
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::ClearStatObjGarbage()
{
    FUNCTION_PROFILER_3DENGINE;

    // No work? Exit early before attempting to take any locks
    if (m_checkForGarbage.empty())
    {
        return;
    }

    // We have to take the load lock here because InternalDeleteObject needs this lock and loadMutex has to be locked before garbageMutex
    // Additionally, we need to hold one of these locks for the entire duration of this function to prevent the loading thread from using an object that is about to be deleted
    // To avoid stalls due to the threads loading files, try to get the lock and if it fails simply try again next frame unless there are too many garbage objects.
    AZStd::unique_lock<AZStd::recursive_mutex> loadLock(m_loadMutex, AZStd::try_to_lock_t());
    if (!loadLock.owns_lock())
    {
        if (m_checkForGarbage.size() > s_maxPendingGarbageObjects)
        {
            AZ_PROFILE_SCOPE_STALL(AZ::Debug::ProfileCategory::ThreeDEngine, "StatObjGarbage overflow");
            // There are too many objects pending garbage collection so force a clear this frame even if loading is happening.
            loadLock.lock();
        }
        else
        {
            return;
        }
    }

    AZStd::vector<IStatObj*> garbage;

    // We might need to perform the entire garbage collection logic more than once because the call to
    // pStatObj->Shutdown() has the potential to add separate LOD models back onto the m_checkForGarbage list.
    // If we only run the logic once, we might exit the ClearStatObjGarbage() function with garbage that hasn't been
    // fully cleared.
    while (!m_checkForGarbage.empty())
    {
        {
            AZStd::unique_lock<AZStd::recursive_mutex> garbageLock(m_garbageMutex);

            // Make sure all stat objects inside this array are unique.
            // Only check explicitly added objects.
            IStatObj* pStatObj;

            while (!m_checkForGarbage.empty())
            {
                pStatObj = m_checkForGarbage.back();
                m_checkForGarbage.pop_back();

                if (pStatObj->CheckGarbage())
                {
                    // Check if it must be released.
                    int nChildRefs = pStatObj->CountChildReferences();

                    if (pStatObj->GetUserCount() <= 0 && nChildRefs <= 0)
                    {
                        garbage.push_back(pStatObj);
                    }
                    else
                    {
                        pStatObj->SetCheckGarbage(false);
                    }
                }
            }
        }

        // First ShutDown object clearing all pointers.
        for (int i = 0, num = (int)garbage.size(); i < num; i++)
        {
            IStatObj* pStatObj = garbage[i];

            if (!m_bLockCGFResources && !IsResourceLocked(pStatObj->GetFileName()))
            {
                // only shutdown object if it can be deleted by InternalDeleteObject()
                pStatObj->ShutDown();
            }
        }

        // Then delete all garbage objects.
        for (int i = 0, num = (int)garbage.size(); i < num; i++)
        {
            IStatObj* pStatObj = garbage[i];
            InternalDeleteObject(pStatObj);
        }

        garbage.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
IRenderMesh* CObjManager::GetRenderMeshBox()
{
    if (!m_pRMBox)
    {
        MakeUnitCube();
    }
    return m_pRMBox;
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::CheckForGarbage(IStatObj* pObject)
{
    if (m_bGarbageCollectionEnabled &&
        !pObject->CheckGarbage())
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_garbageMutex);

        pObject->SetCheckGarbage(true);
        m_checkForGarbage.push_back(pObject);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::UnregisterForGarbage(IStatObj* pObject)
{
    CRY_ASSERT(pObject);

    if (m_bGarbageCollectionEnabled &&
        pObject->CheckGarbage())
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_garbageMutex);

        if (!m_checkForGarbage.empty())
        {
            auto iterator = std::find(m_checkForGarbage.begin(), m_checkForGarbage.end(), pObject);

            if (iterator != m_checkForGarbage.end())
            {
                m_checkForGarbage.erase(iterator);
            }
        }

        pObject->SetCheckGarbage(false);
    }
}

void CObjManager::MakeDepthCubemapRenderItemList(CVisArea* pReceiverArea, const AABB& cubemapAABB, [[maybe_unused]] int renderNodeFlags, PodArray<struct IShadowCaster*>* objectsList, const SRenderingPassInfo& passInfo)
{
    if (pReceiverArea)
    {
        if (pReceiverArea->m_pObjectsTree)
        {
            pReceiverArea->m_pObjectsTree->FillDepthCubemapRenderList(cubemapAABB, passInfo, objectsList);
        }
    }
    else
    {
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->FillDepthCubemapRenderList(cubemapAABB, passInfo, objectsList);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// StatInstGroupEventBus
//////////////////////////////////////////////////////////////////////////
StatInstGroupId CObjManager::GenerateStatInstGroupId()
{
    // generate new id
    StatInstGroupId id = StatInstGroupEvents::s_InvalidStatInstGroupId;
    for (StatInstGroupId i = 0; i < std::numeric_limits<StatInstGroupId>::max(); ++i)
    {
        if (m_usedIds.find(i) == m_usedIds.end())
        {
            id = i;
            break;
        }
    }

    if (id == StatInstGroupEvents::s_InvalidStatInstGroupId)
    {
        return id;
    }

    // Mark id as used
    m_usedIds.insert(id);
    return id;
}

void CObjManager::ReleaseStatInstGroupId(StatInstGroupId statInstGroupId)
{
    // Free id for this object
    m_usedIds.erase(statInstGroupId);
}

void CObjManager::ReleaseStatInstGroupIdSet(const AZStd::unordered_set<StatInstGroupId>& statInstGroupIdSet)
{
    for (auto groupId : statInstGroupIdSet)
    {
        m_usedIds.erase(groupId);
    }
}

void CObjManager::ReserveStatInstGroupIdRange(StatInstGroupId from, StatInstGroupId to)
{
    for (StatInstGroupId id = from; id < to; ++id)
    {
        m_usedIds.insert(id);
    }
}


