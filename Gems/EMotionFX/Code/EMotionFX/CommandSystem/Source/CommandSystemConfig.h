/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
