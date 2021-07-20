/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_EMSTUDIOCONFIG_H
#define __EMSTUDIO_EMSTUDIOCONFIG_H

#include <QString>

// when we use DLL files, setup the EMFX_API macro
#if defined(EMSTUDIO_DLL_EXPORT)
    #define EMSTUDIO_API MCORE_EXPORT
#else
    #if defined(EMSTUDIO_DLL)
        #define EMSTUDIO_API MCORE_IMPORT
    #else
        #define EMSTUDIO_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_EMSTUDIOSDK                     = 991,
    MEMCATEGORY_EMSTUDIOSDK_CUSTOMWIDGETS       = 950,
    MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE    = 952,
};

#define SHOW_REALTIMEINTERFACE_PERFORMANCEINFO

#endif
