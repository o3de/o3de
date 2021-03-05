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

// Description : Defines the DLL entry point, implements access to other modules


#include "Cry3DEngine_precompiled.h"

#include "MatMan.h"

#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "I3DEngine_info.h"

//////////////////////////////////////////////////////////////////////

struct CSystemEventListner_3DEngine
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
            if (Cry3DEngineBase::Get3DEngine())
            {
                Cry3DEngineBase::Get3DEngine()->ClearPrecacheInfo();
            }
            break;
        case ESYSTEM_EVENT_RANDOM_SEED:
            cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
            break;
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            if (Cry3DEngineBase::Get3DEngine())
            {
                Cry3DEngineBase::Get3DEngine()->ClearDebugFPSInfo(true);
            }
            break;
        }
        case ESYSTEM_EVENT_LEVEL_LOAD_END:
        {
            if (Cry3DEngineBase::Get3DEngine())
            {
                Cry3DEngineBase::Get3DEngine()->ClearDebugFPSInfo();
            }
            if (Cry3DEngineBase::GetObjManager())
            {
                Cry3DEngineBase::GetObjManager()->FreeNotUsedCGFs();
            }
            Cry3DEngineBase::m_bLevelLoadingInProgress = false;
            break;
        }
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        {
            Cry3DEngineBase::m_bLevelLoadingInProgress = true;
            break;
        }
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        {
            Cry3DEngineBase::m_bLevelLoadingInProgress = true;
            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_START:
        {
            Cry3DEngineBase::GetMatMan()->DoLoadSurfaceTypesInInit(false);
            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_END:
        {
            if (Cry3DEngineBase::Get3DEngine()->GetObjectTree())
            {
                delete Cry3DEngineBase::Get3DEngine()->GetObjectTree();
                Cry3DEngineBase::Get3DEngine()->SetObjectTree(nullptr);
            }

            if (IObjManager* pObjManager = Cry3DEngineBase::GetObjManager())
            {
                pObjManager->UnloadObjects(true);
            }


            if (Cry3DEngineBase::GetMatMan())
            {
                Cry3DEngineBase::GetMatMan()->ShutDown();
                Cry3DEngineBase::GetMatMan()->DoLoadSurfaceTypesInInit(true);
            }

            break;
        }
        }
    }
};
static CSystemEventListner_3DEngine g_system_event_listener_engine;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CEngineModule_Cry3DEngine
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_Cry3DEngine, "EngineModule_Cry3DEngine", 0x2d38f12a521d43cf, 0xba18fd1fa7ea5020)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "Cry3DEngine";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        ModuleInitISystem(pSystem, "Cry3DEngine");
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_engine);

        C3DEngine* p3DEngine = CryAlignedNew<C3DEngine>(pSystem);
        env.p3DEngine = p3DEngine;
        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_Cry3DEngine)

CEngineModule_Cry3DEngine::CEngineModule_Cry3DEngine()
{
};

CEngineModule_Cry3DEngine::~CEngineModule_Cry3DEngine()
{
};

#if !defined(AZ_MONOLITHIC_BUILD)
    #include <CrtDebugStats.h>
#endif

#include "TypeInfo_impl.h"

// 3DEngine types
#include "SkyLightNishita_info.h"

STRUCT_INFO_BEGIN(SImageSubInfo)
VAR_INFO(nDummy)
VAR_INFO(nDim)
VAR_INFO(fTilingIn)
VAR_INFO(fTiling)
VAR_INFO(fSpecularAmount)
VAR_INFO(nSortOrder)
STRUCT_INFO_END(SImageSubInfo)

STRUCT_INFO_BEGIN(SImageInfo)
VAR_INFO(baseInfo)
VAR_INFO(detailInfo)
VAR_INFO(szDetMatName)
VAR_INFO(arrTextureId)
VAR_INFO(nPhysSurfaceType)
VAR_INFO(szBaseTexName)
VAR_INFO(fUseRemeshing)
VAR_INFO(layerFilterColor)
VAR_INFO(nLayerId)
VAR_INFO(fBr)
STRUCT_INFO_END(SImageInfo)
