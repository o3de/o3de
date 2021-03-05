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

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Sandbox Plugin Instance Creation
//
/////////////////////////////////////////////////////////////////////////////

#include "EditorAssetImporter_precompiled.h"
#include "AssetImporterPlugin.h"


#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <AzCore/Memory/SystemAllocator.h>

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    IEditor* editor = pInitParam->pIEditorInterface;
    SetIEditor(editor);
    ISystem* system = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(system, "QtAssetImporter");
    return new AssetImporterPlugin(editor);
}


#if !defined(AZ_MONOLITHIC_BUILD)

#if defined(AZ_PLATFORM_WINDOWS)
HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }

    return TRUE;
}
#endif

#endif // !defined(AZ_MONOLITHIC_BUILD)
