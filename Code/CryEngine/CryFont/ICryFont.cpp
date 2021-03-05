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

// Description : Create the font interface.


#include "CryFont_precompiled.h"

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "CryFont.h"
#if defined(USE_NULLFONT)
#include "NullFont.h"
#endif

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Font
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            break;
        }
        }
    }
};
static CSystemEventListner_Font g_system_event_listener_font;

///////////////////////////////////////////////
extern "C" ICryFont * CreateCryFontInterface(ISystem * pSystem)
{
    ModuleInitISystem(pSystem, "CryFont");

    if (gEnv->IsDedicated())
    {
#if defined(USE_NULLFONT)
        return new CCryNullFont();
#else
        // The NULL font implementation must be present for all platforms
        // supporting running as a pure dedicated server.
        pSystem->GetILog()->LogError("Missing NULL font implementation for dedicated server");
        return NULL;
#endif
    }
    else
    {
#if defined(USE_NULLFONT) && defined(USE_NULLFONT_ALWAYS)
        return new CCryNullFont();
#else
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_font);
        return new CCryFont(pSystem);
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryFont
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryFont, "EngineModule_CryFont", 0x6758643f43214957, 0x9b920d898d31f434)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryFont";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;
        env.pCryFont = CreateCryFontInterface(pSystem);
        return env.pCryFont != 0;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryFont)

CEngineModule_CryFont::CEngineModule_CryFont()
{
};

CEngineModule_CryFont::~CEngineModule_CryFont()
{
};
