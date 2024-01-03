/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4701, "-Wuninitialized") //  warning C4701: potentially uninitialized local variable 'mid' used in vorbis

extern "C" {

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>    // Enables Vorbis decoding.

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// The stb_vorbis implementation must come after the implementation of miniaudio.
#undef STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

}

AZ_POP_DISABLE_WARNING
