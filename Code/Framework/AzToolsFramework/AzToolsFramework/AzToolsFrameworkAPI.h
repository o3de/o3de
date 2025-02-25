/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h> ///< Platform/compiler specific defines

#if defined(AZ_MONOLITHIC_BUILD)
    #define AZTF_API
    #define AZTF_EBUS_API
#else
    #if defined(AZTF_EXPORTS)
        #define AZTF_API        AZ_DLL_EXPORT
    #else
        #define AZTF_API        AZ_DLL_IMPORT
    #endif
    #if defined(AZTF_EBUS_EXPORTS)
        #define AZTF_EBUS_API   AZ_DLL_EXPORT
    #else
        #define AZTF_EBUS_API   AZ_DLL_IMPORT
    #endif
#endif
