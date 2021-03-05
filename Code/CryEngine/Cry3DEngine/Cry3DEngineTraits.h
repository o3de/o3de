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
#pragma once

#include "ProjectDefines.h"

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(Cry3DEngineTraits_h)
#else
#if defined(WIN32) || defined(WIN64)
#define AZ_LEGACY_3DENGINE_TRAIT_DEFINE_MM_MULLO_EPI32_EMU 1
#endif
#if defined(WIN32) || defined(WIN64)
#define AZ_LEGACY_3DENGINE_TRAIT_DO_EXTRA_GEOMCACHE_PROCESSING 1 // probably needs a better name
#endif
#define AZ_LEGACY_3DENGINE_TRAIT_HAS_MM_CVTEPI16_EPI32 0
#define AZ_LEGACY_3DENGINE_TRAIT_HAS_MM_MULLO_EPI32 0
#define AZ_LEGACY_3DENGINE_TRAIT_HAS_MM_PACKUS_EPI32 0
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(MAC)
#define AZ_LEGACY_3DENGINE_TRAIT_HAS_SSE 1
#endif
#define AZ_LEGACY_3DENGINE_TRAIT_UNROLL_GEOMETRY_BACKING_LOOPS 1
#if defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_3DENGINE_TRAIT_DISABLE_MMRM_SSE_INSTRUCTIONS 1
#endif
#endif
