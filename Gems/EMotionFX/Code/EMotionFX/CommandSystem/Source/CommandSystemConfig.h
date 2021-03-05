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

#ifndef __COMMANDSYSTEM_CONFIG_H
#define __COMMANDSYSTEM_CONFIG_H

#include <MCore/Source/StandardHeaders.h>

// when we use DLL files, setup the COMMANDSYSTEM_API macro
#if defined(COMMANDSYSTEM_DLL_EXPORT)
    #define COMMANDSYSTEM_API           MCORE_EXPORT
    #define DEFINECOMMAND_API           MCORE_EXPORT
#else
    #if defined(COMMANDSYSTEM_DLL)
        #define COMMANDSYSTEM_API       MCORE_IMPORT

        #ifdef DEFINECOMMAND_API
            #undef DEFINECOMMAND_API
        #endif

        #define DEFINECOMMAND_API       MCORE_IMPORT
    #else
        #define COMMANDSYSTEM_API
        #define DEFINECOMMAND_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_COMMANDSYSTEM = 990
};

#endif
