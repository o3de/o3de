/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFXConfig.h"
#include "EMotionFXManager.h"
#include "Importer/Importer.h"
#include "ActorManager.h"
#include "MotionManager.h"
#include "EventManager.h"
#include "SoftSkinManager.h"
#include "Importer/Importer.h"
#include "Recorder.h"
#include "MotionInstancePool.h"
#include "AnimGraphManager.h"
#include <MCore/Source/MCoreSystem.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/MemoryTracker.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/Job.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <Integration/Rendering/RenderActorSettings.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EMotionFXManager, EMotionFXManagerAllocator)

    // the global EMotion FX manager object
    AZ::EnvironmentVariable<EMotionFXManager*> gEMFX;

    //-----------------------------------------------------------------------------

    // initialize the global EMotion FX object
    bool Initializer::Init(InitSettings* initSettings)
    {
        // if we already have initialized, there is nothing to do
        if (gEMFX)
        {
            return true;
        }

        // create the new object
        gEMFX = AZ::Environment::CreateVariable<EMotionFXManager*>(kEMotionFXInstanceVarName);
        gEMFX.Set(EMotionFXManager::Create());

        InitSettings finalSettings;
        if (initSettings)
        {
            finalSettings = *initSettings;
        }

        // set the unit type
        gEMFX.Get()->SetUnitType(finalSettings.m_unitType);

        // create and set the objects
        gEMFX.Get()->SetImporter              (Importer::Create());
        gEMFX.Get()->SetActorManager          (ActorManager::Create());
        gEMFX.Get()->SetMotionManager         (MotionManager::Create());
        gEMFX.Get()->SetEventManager          (EventManager::Create());
        gEMFX.Get()->SetSoftSkinManager       (SoftSkinManager::Create());
        gEMFX.Get()->SetAnimGraphManager      (AnimGraphManager::Create());
        gEMFX.Get()->GetAnimGraphManager()->Init();
        gEMFX.Get()->SetRecorder              (Recorder::Create());
        gEMFX.Get()->SetMotionInstancePool    (MotionInstancePool::Create());
        gEMFX.Get()->SetDebugDraw             (aznew DebugDraw());
        gEMFX.Get()->SetPoseDataFactory       (aznew PoseDataFactory());
        gEMFX.Get()->SetGlobalSimulationSpeed (1.0f);

        // set the number of threads
        const AZ::u32 numThreads = AZ::JobContext::GetGlobalContext()->GetJobManager().GetNumWorkerThreads();
        AZ_Assert(numThreads > 0, "The number of threads is expected to be bigger than 0.");
        gEMFX.Get()->SetNumThreads(numThreads);

        // Init motion data factory, which registers the internal motion data types.
        GetMotionManager().GetMotionDataFactory().Init();

        // show details
        gEMFX.Get()->LogInfo();
        return true;
    }


    // shutdown EMotion FX
    void Initializer::Shutdown()
    {
        // delete the global object and reset it to nullptr
        gEMFX.Get()->Destroy();
        gEMFX.Reset();
    }

    //-----------------------------------------------------------------------------


    // constructor
    EMotionFXManager::EMotionFXManager()
    {
        // build the low version string
        AZStd::string lowVersionString;
        BuildLowVersionString(lowVersionString);

        m_versionString = AZStd::string::format("EMotion FX v%d.%s RC4", EMFX_HIGHVERSION, lowVersionString.c_str());
        m_compilationDate        = MCORE_DATE;
        m_highVersion            = EMFX_HIGHVERSION;
        m_lowVersion             = EMFX_LOWVERSION;
        m_importer               = nullptr;
        m_actorManager           = nullptr;
        m_motionManager          = nullptr;
        m_eventManager           = nullptr;
        m_softSkinManager        = nullptr;
        m_recorder               = nullptr;
        m_motionInstancePool     = nullptr;
        m_debugDraw              = nullptr;
        m_poseDataFactory        = nullptr;
        m_unitType               = MCore::Distance::UNITTYPE_METERS;
        m_globalSimulationSpeed  = 1.0f;
        m_isInEditorMode        = false;
        m_isInServerMode        = false;

        // EMotionFX will do optimization in server mode when this is enabled.
        m_enableServerOptimization = true;

        if (MCore::GetMCore().GetIsTrackingMemory())
        {
            RegisterMemoryCategories(MCore::GetMemoryTracker());
        }

        m_renderActorSettings = AZStd::make_unique<AZ::Render::RenderActorSettings>();
    }


    // destructor
    EMotionFXManager::~EMotionFXManager()
    {
        // the motion manager has to get destructed before the anim graph manager as the motion manager kills all motion instances
        // from the motion nodes when destructing the motions itself
        m_motionManager->Destroy();
        m_motionManager = nullptr;

        m_animGraphManager->Destroy();
        m_animGraphManager = nullptr;

        m_importer->Destroy();
        m_importer = nullptr;

        m_actorManager->Destroy();
        m_actorManager = nullptr;

        m_motionInstancePool->Destroy();
        m_motionInstancePool = nullptr;

        m_softSkinManager->Destroy();
        m_softSkinManager = nullptr;

        m_recorder->Destroy();
        m_recorder = nullptr;

        delete m_debugDraw;
        m_debugDraw = nullptr;

        delete m_poseDataFactory;
        m_poseDataFactory = nullptr;

        m_renderActorSettings.reset();

        m_eventManager->Destroy();
        m_eventManager = nullptr;

        // delete the thread datas
        for (uint32 i = 0; i < m_threadDatas.size(); ++i)
        {
            m_threadDatas[i]->Destroy();
        }
        m_threadDatas.clear();
    }


    // create
    EMotionFXManager* EMotionFXManager::Create()
    {
        return aznew EMotionFXManager();
    }


    // update
    void EMotionFXManager::Update(float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "EMotionFXManager::Update");

        m_debugDraw->Clear();
        m_recorder->UpdatePlayMode(timePassedInSeconds);
        m_actorManager->UpdateActorInstances(timePassedInSeconds);
        m_eventManager->OnSimulatePhysics(timePassedInSeconds);
        m_recorder->Update(timePassedInSeconds);

        // sample and apply all anim graphs we recorded
        if (m_recorder->GetIsInPlayMode() && m_recorder->GetRecordSettings().m_recordAnimGraphStates)
        {
            m_recorder->SampleAndApplyAnimGraphs(m_recorder->GetCurrentPlayTime());
        }
    }


    // log information about EMotion FX
    void EMotionFXManager::LogInfo()
    {
        // turn "0.010" into "01" to for example build the string v3.01 later on
        AZStd::string lowVersionString;
        BuildLowVersionString(lowVersionString);

        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("EMotion FX - Information");
        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("Version:          v%d.%s", m_highVersion, lowVersionString.c_str());
        MCore::LogInfo("Version string:   %s", m_versionString.c_str());
        MCore::LogInfo("Compilation date: %s", m_compilationDate.c_str());

    #ifdef MCORE_OPENMP_ENABLED
        MCore::LogInfo("OpenMP enabled:   Yes");
    #else
        MCore::LogInfo("OpenMP enabled:   No");
    #endif

        MCore::LogInfo("-----------------------------------------------");
    }


    // get the version string
    const char* EMotionFXManager::GetVersionString() const
    {
        return m_versionString.c_str();
    }


    // get the compilation date string
    const char* EMotionFXManager::GetCompilationDate() const
    {
        return m_compilationDate.c_str();
    }


    // get the high version
    uint32 EMotionFXManager::GetHighVersion() const
    {
        return m_highVersion;
    }


    // get the low version
    uint32 EMotionFXManager::GetLowVersion() const
    {
        return m_lowVersion;
    }


    // set the importer
    void EMotionFXManager::SetImporter(Importer* importer)
    {
        m_importer = importer;
    }


    // set the actor manager
    void EMotionFXManager::SetActorManager(ActorManager* manager)
    {
        m_actorManager = manager;
    }


    // set the motion manager
    void EMotionFXManager::SetMotionManager(MotionManager* manager)
    {
        m_motionManager = manager;
    }


    // set the event manager
    void EMotionFXManager::SetEventManager(EventManager* manager)
    {
        m_eventManager = manager;
    }


    // set the softskin manager
    void EMotionFXManager::SetSoftSkinManager(SoftSkinManager* manager)
    {
        m_softSkinManager = manager;
    }


    // set the anim graph manager
    void EMotionFXManager::SetAnimGraphManager(AnimGraphManager* manager)
    {
        m_animGraphManager = manager;
    }


    // set the recorder
    void EMotionFXManager::SetRecorder(Recorder* recorder)
    {
        m_recorder = recorder;
    }

    void EMotionFXManager::SetDebugDraw(DebugDraw* draw)
    {
        m_debugDraw = draw;
    }

    // set the motion instance pool
    void EMotionFXManager::SetMotionInstancePool(MotionInstancePool* pool)
    {
        m_motionInstancePool = pool;
        pool->Init();
    }


    // set the path of the media root directory
    void EMotionFXManager::SetMediaRootFolder(const char* path)
    {
        m_mediaRootFolder = path;

        // Make sure the media root folder has an ending slash.
        if (m_mediaRootFolder.empty() == false)
        {
            const char lastChar = AzFramework::StringFunc::LastCharacter(m_mediaRootFolder.c_str());
            if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
            {
                AzFramework::StringFunc::Path::AppendSeparator(m_mediaRootFolder);
            }
        }

        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, m_mediaRootFolder);
    }


    void EMotionFXManager::InitAssetFolderPaths()
    {
        // Initialize the asset source folder path.
        const char* assetSourcePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@");
        if (assetSourcePath)
        {
            m_assetSourceFolder = assetSourcePath;

            // Add an ending slash in case there is none yet.
            // TODO: Remove this and adopt EMotionFX code to work with folder paths without slash at the end like Open 3D Engine does.
            if (m_assetSourceFolder.empty() == false)
            {
                const char lastChar = AzFramework::StringFunc::LastCharacter(m_assetSourceFolder.c_str());
                if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
                {
                    AzFramework::StringFunc::Path::AppendSeparator(m_assetSourceFolder);
                }
            }

            AzFramework::ApplicationRequests::Bus::Broadcast(
                &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, m_assetSourceFolder);
        }
        else
        {
            AZ_Warning("EMotionFX", false, "Failed to set asset source path for alias '@projectroot@'.");
        }


        // Initialize the asset cache folder path.
        const char* assetCachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@products@");
        if (assetCachePath)
        {
            m_assetCacheFolder = assetCachePath;

            // Add an ending slash in case there is none yet.
            // TODO: Remove this and adopt EMotionFX code to work with folder paths without slash at the end like Open 3D Engine does.
            if (m_assetCacheFolder.empty() == false)
            {
                const char lastChar = AzFramework::StringFunc::LastCharacter(m_assetCacheFolder.c_str());
                if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
                {
                    AzFramework::StringFunc::Path::AppendSeparator(m_assetCacheFolder);
                }
            }

            AzFramework::ApplicationRequests::Bus::Broadcast(
                &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, m_assetCacheFolder);
        }
        else
        {
            AZ_Warning("EMotionFX", false, "Failed to set asset cache path for alias '@products@'.");
        }
    }


    void EMotionFXManager::ConstructAbsoluteFilename(const char* relativeFilename, AZStd::string& outAbsoluteFilename)
    {
        outAbsoluteFilename = relativeFilename;
        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, outAbsoluteFilename);
        AzFramework::StringFunc::Replace(outAbsoluteFilename, EMFX_MEDIAROOTFOLDER_STRING, m_mediaRootFolder.c_str(), true);
    }


    AZStd::string EMotionFXManager::ConstructAbsoluteFilename(const char* relativeFilename)
    {
        AZStd::string result;
        ConstructAbsoluteFilename(relativeFilename, result);
        return result;
    }


    void EMotionFXManager::GetFilenameRelativeTo(AZStd::string* inOutFilename, const char* folderPath)
    {
        AZStd::string baseFolderPath = folderPath;
        AZStd::string filename = inOutFilename->c_str();

        // TODO: Add parameter to not lower case the path once it is in and working.
        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, baseFolderPath);
        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        // Remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder.
        AzFramework::StringFunc::Replace(filename, baseFolderPath.c_str(), "", false /* case sensitive */, true /* replace first */);
        *inOutFilename = filename;
    }


    void EMotionFXManager::GetFilenameRelativeToMediaRoot(AZStd::string* inOutFilename) const
    {
        GetFilenameRelativeTo(inOutFilename, GetMediaRootFolder());
    }

    AZStd::string EMotionFXManager::ResolvePath(const char* path)
    {
        AZStd::string result;
        result.resize(AZ::IO::MaxPathLength);
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(path, result.data(), result.size());
        result.resize_no_construct(AZStd::char_traits<char>::length(result.data()));
        return result;
    }


    // build the low version string
    // this turns 900 into 9, and 50 into 05
    void EMotionFXManager::BuildLowVersionString(AZStd::string& outLowVersionString)
    {
        outLowVersionString = AZStd::string::format("%.2f", EMFX_LOWVERSION / 100.0f);
        AzFramework::StringFunc::Strip(outLowVersionString, '0', true /* case sensitive */, true /* beginning */, true /* ending */);
        AzFramework::StringFunc::Strip(outLowVersionString, '.', true /* case sensitive */, true /* beginning */, true /* ending */);

        if (outLowVersionString.empty())
        {
            outLowVersionString = "0";
        }
    }


    // get the global speed factor
    float EMotionFXManager::GetGlobalSimulationSpeed() const
    {
        return m_globalSimulationSpeed;
    }


    // set the global speed factor
    void EMotionFXManager::SetGlobalSimulationSpeed(float speedFactor)
    {
        m_globalSimulationSpeed = MCore::Max<float>(0.0f, speedFactor);
    }


    // set the number of threads
    void EMotionFXManager::SetNumThreads(uint32 numThreads)
    {
        AZ_Assert(numThreads > 0 && numThreads <= 1024, "Number of threads is expected to be between 0 and 1024");
        if (numThreads == 0)
        {
            numThreads = 1;
        }

        if (m_threadDatas.size() == numThreads)
        {
            return;
        }

        // get rid of old data
        for (uint32 i = 0; i < m_threadDatas.size(); ++i)
        {
            m_threadDatas[i]->Destroy();
        }

        m_threadDatas.clear(); // force calling constructors again to reset everything
        m_threadDatas.resize(numThreads);

        for (uint32 i = 0; i < numThreads; ++i)
        {
            m_threadDatas[i] = ThreadData::Create(i);
        }
    }


    // shrink internal pools to minimize memory usage
    void EMotionFXManager::ShrinkPools()
    {
        Allocators::ShrinkPools();
        m_motionInstancePool->Shrink();
    }


    // get the unit type
    MCore::Distance::EUnitType EMotionFXManager::GetUnitType() const
    {
        return m_unitType;
    }


    // set the unit type
    void EMotionFXManager::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        m_unitType = unitType;
    }


    // register the EMotion FX memory categories
    void EMotionFXManager::RegisterMemoryCategories(MCore::MemoryTracker& memTracker)
    {
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_MATERIALS,                       "EMFX_MEMCATEGORY_GEOMETRY_MATERIALS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES,                          "EMFX_MEMCATEGORY_GEOMETRY_MESHES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS,                       "EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES,                "EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS,                   "EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES,                  "EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS,                    "EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS,                  "EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS,                    "EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS,                        "EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS,                      "EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EVENTS,                                   "EMFX_MEMCATEGORY_EVENTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MISC,                             "EMFX_MEMCATEGORY_MOTIONS_MISC");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS,                       "EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER,                    "EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EVENTHANDLERS,                            "EMFX_MEMCATEGORY_EVENTHANDLERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EYEBLINKER,                               "EMFX_MEMCATEGORY_EYEBLINKER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_GROUPS,                           "EMFX_MEMCATEGORY_MOTIONS_GROUPS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL,                       "EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODES,                                    "EMFX_MEMCATEGORY_NODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORS,                                   "EMFX_MEMCATEGORY_ACTORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORINSTANCES,                           "EMFX_MEMCATEGORY_ACTORINSTANCES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODEATTRIBUTES,                           "EMFX_MEMCATEGORY_NODEATTRIBUTES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODESMISC,                                "EMFX_MEMCATEGORY_NODESMISC");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODEMAP,                                  "EMFX_MEMCATEGORY_NODEMAP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_RIGSYSTEM,                                "EMFX_MEMCATEGORY_RIGSYSTEM");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_TRANSFORMDATA,                            "EMFX_MEMCATEGORY_TRANSFORMDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_POSE,                                     "EMFX_MEMCATEGORY_POSE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_TRANSFORM,                                "EMFX_MEMCATEGORY_TRANSFORM");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_SKELETON,                                 "EMFX_MEMCATEGORY_SKELETON");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_CONSTRAINTS,                              "EMFX_MEMCATEGORY_CONSTRAINTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH,                               "EMFX_MEMCATEGORY_ANIMGRAPH");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER,                       "EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE,                      "EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES,                    "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES,                "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES,                 "EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATES,                        "EMFX_MEMCATEGORY_ANIMGRAPH_STATES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS,                   "EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES,               "EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS,                "EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA,              "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS,                       "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS,                   "EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK,                     "EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE,                          "EMFX_MEMCATEGORY_ANIMGRAPH_POSE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS,                    "EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS,                  "EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL,                      "EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODES,                         "EMFX_MEMCATEGORY_ANIMGRAPH_NODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP,                     "EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE,                    "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL,                "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA,                "EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_WAVELETCACHE,                             "EMFX_MEMCATEGORY_WAVELETCACHE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_WAVELETSKELETONMOTION,                    "EMFX_MEMCATEGORY_WAVELETSKELETONMOTION");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IMPORTER,                                 "EMFX_MEMCATEGORY_IMPORTER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IDGENERATOR,                              "EMFX_MEMCATEGORY_IDGENERATOR");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORMANAGER,                             "EMFX_MEMCATEGORY_ACTORMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_UPDATESCHEDULERS,                         "EMFX_MEMCATEGORY_UPDATESCHEDULERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ATTACHMENTS,                              "EMFX_MEMCATEGORY_ATTACHMENTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EMOTIONFXMANAGER,                         "EMFX_MEMCATEGORY_EMOTIONFXMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_FILEPROCESSORS,                           "EMFX_MEMCATEGORY_FILEPROCESSORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EMSTUDIODATA,                             "EMFX_MEMCATEGORY_EMSTUDIODATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_RECORDER,                                 "EMFX_MEMCATEGORY_RECORDER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IK,                                       "EMFX_MEMCATEGORY_IK");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER,                              "EMFX_MEMCATEGORY_MESHBUILDER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO,                 "EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH,                      "EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP,                 "EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER,         "EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER");

        // actor group
        std::vector<uint32> idValues;
        idValues.reserve(100);
        idValues.push_back(EMFX_MEMCATEGORY_NODES);
        idValues.push_back(EMFX_MEMCATEGORY_ACTORS);
        idValues.push_back(EMFX_MEMCATEGORY_NODEATTRIBUTES);
        idValues.push_back(EMFX_MEMCATEGORY_NODESMISC);
        idValues.push_back(EMFX_MEMCATEGORY_ACTORINSTANCES);
        idValues.push_back(EMFX_MEMCATEGORY_TRANSFORMDATA);
        idValues.push_back(EMFX_MEMCATEGORY_POSE);
        idValues.push_back(EMFX_MEMCATEGORY_TRANSFORM);
        idValues.push_back(EMFX_MEMCATEGORY_SKELETON);
        idValues.push_back(EMFX_MEMCATEGORY_CONSTRAINTS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_MATERIALS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
        idValues.push_back(EMFX_MEMCATEGORY_EVENTHANDLERS);
        idValues.push_back(EMFX_MEMCATEGORY_EYEBLINKER);
        idValues.push_back(EMFX_MEMCATEGORY_ATTACHMENTS);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER);
        idValues.push_back(EMFX_MEMCATEGORY_RIGSYSTEM);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_STATES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_NODES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTEPOOL);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTEFACTORY);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTES);
        memTracker.RegisterGroup(EMFX_MEMORYGROUP_ACTORS, "EMFX_MEMORYGROUP_ACTORS", idValues);

        // motion group
        idValues.clear();
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS);
        idValues.push_back(EMFX_MEMCATEGORY_EVENTS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MISC);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_GROUPS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_NODEMAP);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_WAVELETS);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_HUFFMAN);
        memTracker.RegisterGroup(EMFX_MEMORYGROUP_MOTIONS, "EMFX_MEMORYGROUP_MOTIONS", idValues);
    }
} //  namespace EMotionFX
