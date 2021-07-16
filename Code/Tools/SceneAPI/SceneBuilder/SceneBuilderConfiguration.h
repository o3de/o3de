#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #define SCENE_BUILDER_API
#else
    #ifdef SCENE_BUILDER_EXPORTS
        #define SCENE_BUILDER_API AZ_DLL_EXPORT
    #else
        #define SCENE_BUILDER_API AZ_DLL_IMPORT
    #endif
#endif // AZ_MONOLITHIC_BUILD
