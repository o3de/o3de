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

#include "Maestro_precompiled.h"
#include "CryMovie.h"
#include "Movie.h"
#include <CrtDebugStats.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#undef GetClassName

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Movie
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            CLightAnimWrapper::ReconstructCache();
            break;
        }
        }
    }
};

static CSystemEventListner_Movie g_system_event_listener_movie;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryMovie
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryMovie, "EngineModule_CryMovie", 0xdce26beebdc6400f, 0xa0e9b42839f2dd5b)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryMovie";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_movie);

        env.pMovieSystem = aznew CMovieSystem(pSystem);
        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryMovie)

CEngineModule_CryMovie::CEngineModule_CryMovie()
{
};

CEngineModule_CryMovie::~CEngineModule_CryMovie()
{
};
