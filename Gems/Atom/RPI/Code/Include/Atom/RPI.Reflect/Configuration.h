/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #define ATOM_RPI_REFLECT_API
#else
    #if defined(ATOM_RPI_REFLECT_EXPORTS)
        #define ATOM_RPI_REFLECT_API AZ_DLL_EXPORT
    #else
        #define ATOM_RPI_REFLECT_API AZ_DLL_IMPORT
    #endif
#endif
