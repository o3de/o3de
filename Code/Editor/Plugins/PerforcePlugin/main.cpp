/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "PerforcePlugin_precompiled.h"

#include "PerforcePlugin.h"

#include "Include/ISourceControl.h"
#include "Include/IEditorClassFactory.h"
#include "PerforceSourceControl.h"

#include <AzCore/PlatformDef.h>

CPerforceSourceControl* g_pPerforceControl = 0;


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return 0;
    }

    ModuleInitISystem(GetIEditor()->GetSystem(), "PerforcePlugin");
    g_pPerforceControl = new CPerforceSourceControl();
    pInitParam->pIEditorInterface->GetClassFactory()->RegisterClass(g_pPerforceControl);
    return new CPerforcePlugin();
}

#ifdef AZ_PLATFORM_WINDOWS
HINSTANCE g_hInstance = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        //DisableThreadLibraryCalls(hInstance);
    }

    return(TRUE);
}
#endif
