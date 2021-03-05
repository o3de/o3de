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

// Description : Provides implementation for CryMemoryManager globally defined functions.
//               This file included only by platform_impl.cpp, do not include it directly in code!


#pragma once

#ifdef AZ_MONOLITHIC_BUILD
    #include <ISystem.h> // <> required for Interfuscator
#endif // AZ_MONOLITHIC_BUILD

#include "CryLibrary.h"

#include <AzCore/Module/Environment.h>

#define DLL_ENTRY_GETMEMMANAGER "CryGetIMemoryManagerInterface"

// Resolve IMemoryManager by looking in this DLL, then loading and rummaging through
// CrySystem. Cache the result per DLL, because this is not quick.
IMemoryManager* CryGetIMemoryManager()
{
    static AZ::EnvironmentVariable<IMemoryManager*> memMan = nullptr;
    if (!memMan)
    {
        memMan = AZ::Environment::FindVariable<IMemoryManager*>("CryIMemoryManagerInterface");
        AZ_Assert(memMan, "Unable to find CryIMemoryManagerInterface via AZ::Environment");
    }
    return *memMan;
}
