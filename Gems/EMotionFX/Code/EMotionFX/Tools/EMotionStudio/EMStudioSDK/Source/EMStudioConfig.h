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
