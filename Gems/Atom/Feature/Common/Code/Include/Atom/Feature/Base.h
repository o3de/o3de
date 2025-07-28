/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(AZ_MONOLITHIC_BUILD)
    #define ATOM_FEATURE_COMMON_API
    #define ATOM_FEATURE_COMMON_API_EXPORT
#else
    #if defined(ATOM_FEATURE_COMMON_EXPORTS)
        #define ATOM_FEATURE_COMMON_API        AZ_DLL_EXPORT
        #define ATOM_FEATURE_COMMON_API_EXPORT AZ_DLL_EXPORT
    #else
        #define ATOM_FEATURE_COMMON_API        AZ_DLL_IMPORT
        #define ATOM_FEATURE_COMMON_API_EXPORT
    #endif
#endif
