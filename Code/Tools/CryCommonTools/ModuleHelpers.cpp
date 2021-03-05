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

#include <platform.h>
#include "ModuleHelpers.h"

HMODULE ModuleHelpers::GetCurrentModule(CurrentModuleSpecifier moduleSpecifier)
{
    switch (moduleSpecifier)
    {
    case CurrentModuleSpecifier_Executable:
        return GetModuleHandle(0);

    case CurrentModuleSpecifier_Library:
        MEMORY_BASIC_INFORMATION mbi;
        static int dummy;
        VirtualQuery(&dummy, &mbi, sizeof(mbi));
        HMODULE instance = reinterpret_cast<HMODULE>(mbi.AllocationBase);
        return instance;
    }

    return 0;
}

std::basic_string<TCHAR> ModuleHelpers::GetCurrentModulePath(CurrentModuleSpecifier moduleSpecifier)
{
    // Here's a trick that will get you the handle of the module
    // you're running in without any a-priori knowledge:
    // http://www.dotnet247.com/247reference/msgs/13/65259.aspx
    HMODULE instance = GetCurrentModule(moduleSpecifier);
    TCHAR moduleNameBuffer[MAX_PATH];
    GetModuleFileName(instance, moduleNameBuffer, sizeof(moduleNameBuffer) / sizeof(moduleNameBuffer[0]));
    return moduleNameBuffer;
}
