/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : to get some defines available in every CryEngine project


#pragma once


#include "BaseTypes.h"
#include <AzCore/PlatformDef.h>

#if defined(_RELEASE) && !defined(RELEASE)
    #define RELEASE
#endif

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define PROJECTDEFINES_H_SECTION_STATS_AGENT 1
#define PROJECTDEFINES_H_SECTION_TRAITS 2
#define PROJECTDEFINES_H_SECTION_VTX_IDX 3
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PROJECTDEFINES_H_SECTION_STATS_AGENT
    #include AZ_RESTRICTED_FILE(ProjectDefines_h)
#elif defined(WIN32) || defined(WIN64)
#if !defined(_RELEASE) || defined(PERFORMANCE_BUILD)
#define ENABLE_STATS_AGENT
#endif
#endif

// The following definitions are used by Sandbox and RC to determine which platform support is needed
#define TOOLS_SUPPORT_POWERVR
#define TOOLS_SUPPORT_ETC2COMP

// Type used for vertex indices
// WARNING: If you change this typedef, you need to update AssetProcessorPlatformConfig.ini to convert cgf and abc files to the proper index format.
#if defined(RESOURCE_COMPILER)
typedef uint32 vtx_idx;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(MOBILE)
typedef uint16 vtx_idx;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PROJECTDEFINES_H_SECTION_VTX_IDX
    #include AZ_RESTRICTED_FILE(ProjectDefines_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
// Uncomment one of the two following typedefs:
typedef uint32 vtx_idx;
//typedef uint16 vtx_idx;
#endif


// 0=off, 1=on
#define TERRAIN_USE_CIE_COLORSPACE 0

// When non-zero, const cvar accesses (by name) are logged in release-mode on consoles.
// This can be used to find non-optimal usage scenario's, where the constant should be used directly instead.
// Since read accesses tend to be used in flow-control logic, constants allow for better optimization by the compiler.
#define LOG_CONST_CVAR_ACCESS 0

#if defined(WIN32) || defined(WIN64) || LOG_CONST_CVAR_ACCESS
#define RELEASE_LOGGING
#endif

#if defined(_RELEASE) && !defined(RELEASE_LOGGING)
#define EXCLUDE_NORMAL_LOG
#endif

// Add the "REMOTE_ASSET_PROCESSOR" define except in release
// this makes it so that asset processor functions.  Without this, all assets must be present and on local media
// with this, the asset processor can be used to remotely process assets.
#if !defined(_RELEASE)
#   define REMOTE_ASSET_PROCESSOR
#endif

#if (!defined(_RELEASE) || defined(PERFORMANCE_BUILD))
    #define USE_HTTP_WEBSOCKETS 0
#endif


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PROJECTDEFINES_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(ProjectDefines_h)
#else
#define PROJECTDEFINES_H_TRAIT_DISABLE_MONOLITHIC_PROFILING_MARKERS 1
#if !defined(LINUX) && !defined(APPLE)
#define PROJECTDEFINES_H_TRAIT_ENABLE_SOFTCODE_SYSTEM 1
#endif
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
#define PROJECTDEFINES_H_TRAIT_USE_GPU_PARTICLES 1
#endif
#define PROJECTDEFINES_H_TRAIT_USE_MESH_TESSELLATION 1
#if defined(WIN32)
#define PROJECTDEFINES_H_TRAIT_USE_SVO_GI 1
#endif
#if defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_CRYCOMMON_TRAIT_USE_PTHREADS 1
#define AZ_LEGACY_CRYCOMMON_TRAIT_USE_UNIX_PATHS 1
#endif
#endif

#define USE_GLOBAL_BUCKET_ALLOCATOR

#ifdef IS_PROSDK
#   define USING_TAGES_SECURITY                 // Wrapper for TGVM security
# if defined(LINUX) || defined(APPLE)
#   error LINUX and Mac does not support evaluation version
# endif
#endif

#ifdef USING_TAGES_SECURITY
#   define TAGES_EXPORT __declspec(dllexport)
#else
#   define TAGES_EXPORT
#endif // USING_TAGES_SECURITY
// test -------------------------------------

#define _DATAPROBE



//This feature allows automatic crash submission to JIRA, but does not work outside of O3DE
//Note: This #define will be commented out during code export
#define ENABLE_CRASH_HANDLER

#if !defined(PHYSICS_STACK_SIZE)
# define PHYSICS_STACK_SIZE (128U << 10)
#endif

#if (!defined(_RELEASE) || defined(PERFORMANCE_BUILD)) && !defined(RESOURCE_COMPILER)
#ifndef ENABLE_PROFILING_CODE
    #define ENABLE_PROFILING_CODE
#endif
#if !(defined(SANDBOX_EXPORTS) || defined(PLUGIN_EXPORTS) || (defined(AZ_MONOLITHIC_BUILD) && PROJECTDEFINES_H_TRAIT_DISABLE_MONOLITHIC_PROFILING_MARKERS))
    #define ENABLE_PROFILING_MARKERS
#endif

//lightweight profilers, disable for submissions, disables displayinfo inside 3dengine as well
#ifndef ENABLE_LW_PROFILERS
    #define ENABLE_LW_PROFILERS
#endif
#endif

#if defined(ENABLE_PROFILING_CODE)
#define ENABLE_ART_RT_TIME_ESTIMATE
#endif

#if defined(ENABLE_PROFILING_CODE) && !defined(_RELEASE)
    #define FMOD_STREAMING_DEBUGGING 1
#endif

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(AZ_PLATFORM_LINUX)
#define FLARES_SUPPORT_EDITING
#endif

// Reflect texture slot information - only used in the editor
#if defined(WIN32) || defined(WIN64) || defined(AZ_PLATFORM_MAC)
#define SHADER_REFLECT_TEXTURE_SLOTS 1
#else
#define SHADER_REFLECT_TEXTURE_SLOTS 0
#endif

// these enable and disable certain net features to give compatibility between PCs and consoles / profile and performance builds
#define PC_CONSOLE_NET_COMPATIBLE 0
#define PROFILE_PERFORMANCE_NET_COMPATIBLE 0

#if (!defined(_RELEASE) || defined(PERFORMANCE_BUILD)) && !PROFILE_PERFORMANCE_NET_COMPATIBLE
#define USE_LAGOMETER (1)
#else
#define USE_LAGOMETER (0)
#endif

// enable this in order to support old style material names in old data ("engine/material.mtl" or "mygame/material.mtl" as opposed to just "material.mtl")
// previously, material names could have the game folder in it, but this is not necessary anymore and would not work with things like gems
// note that if you use any older projects such as GameSDK this should remain enabled
#define SUPPORT_LEGACY_MATERIAL_NAMES

// Enable additional structures and code for sprite motion blur. Currently non-functional and disabled
// #define PARTICLE_MOTION_BLUR

// a special ticker thread to run during load and unload of levels
#define USE_NETWORK_STALL_TICKER_THREAD

#if !defined(MOBILE)
//---------------------------------------------------------------------
// Enable Tessellation Features
// (displacement mapping, subdivision, water tessellation)
//---------------------------------------------------------------------
// Modules   : 3DEngine, Renderer
// Depends on: DX11

// Global tessellation feature flag
    #define TESSELLATION
    #ifdef TESSELLATION
// Specific features flags
        #define WATER_TESSELLATION
        #define PARTICLES_TESSELLATION

        #if PROJECTDEFINES_H_TRAIT_USE_MESH_TESSELLATION
// Mesh tessellation (displacement, smoothing, subd)
            #define MESH_TESSELLATION
// Mesh tessellation also in motion blur passes
            #define MOTIONBLUR_TESSELLATION
        #endif

// Dependencies
        #ifdef MESH_TESSELLATION
            #define MESH_TESSELLATION_ENGINE
        #endif
            #ifndef NULL_RENDERER
            #ifdef WATER_TESSELLATION
                #define WATER_TESSELLATION_RENDERER
            #endif
            #ifdef PARTICLES_TESSELLATION
                #define PARTICLES_TESSELLATION_RENDERER
            #endif
            #ifdef MESH_TESSELLATION_ENGINE
                #define MESH_TESSELLATION_RENDERER
            #endif

            #if defined(WATER_TESSELLATION_RENDERER) || defined(PARTICLES_TESSELLATION_RENDERER) || defined(MESH_TESSELLATION_RENDERER)
// Common tessellation flag enabling tessellation stages in renderer
                #define TESSELLATION_RENDERER
            #endif
        #endif // !NULL_RENDERER
    #endif // TESSELLATION
#endif // !defined(MOBILE)

//------------------------------------------------------
// SVO GI
//------------------------------------------------------
// Modules   : Renderer, Engine
// Platform  : DX11
#if !defined(RENDERNODES_LEAN_AND_MEAN)
    #if PROJECTDEFINES_H_TRAIT_USE_SVO_GI
        #define FEATURE_SVO_GI
    #endif
#endif

#if defined(ENABLE_PROFILING_CODE)
#   define USE_DISK_PROFILER
#   define ENABLE_LOADING_PROFILER  // requires AZ_PROFILE_TELEMETRY to also be defined
#endif

#if PROJECTDEFINES_H_TRAIT_USE_GPU_PARTICLES && !defined(NULL_RENDERER)
    #define GPU_PARTICLES 1
#else
    #define GPU_PARTICLES 0
#endif

// The maximum number of joints in an animation
#define MAX_JOINT_AMOUNT 1024
