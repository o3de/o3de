/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// This file exists to allow any preprocessor directives to be applied
// before including miniaudio.h, if necessary.  Note that the header is very large and contains the entirety
// of miniaudio, so care should be taken not to actually use this in header files that themselves are included by
// other header files, if at all possible.

#pragma once

extern "C" {

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>    // Enables Vorbis decoding.

#include <miniaudio.h>

}
