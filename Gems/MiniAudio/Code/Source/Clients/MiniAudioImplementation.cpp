/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

// warning C4701: potentially uninitialized local variable 'mid' used in vorbis
// warning C4244: '=': conversion from 'int' to 'int8', possible loss of data
// warning C4456: declaration of 'z' hides previous local declaration
// warning C4457: declaration of 'n' hides function parameter
// warning C4245: '=': conversion from 'int' to 'uint32', signed /unsigned mismatch
AZ_PUSH_DISABLE_WARNING(4701 4244 4456 4457 4245, "-Wuninitialized")

extern "C" {

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c> // Enables Vorbis decoding.
}

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

extern "C" {
// The stb_vorbis implementation must come after the implementation of miniaudio.
#undef STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
}

AZ_POP_DISABLE_WARNING
