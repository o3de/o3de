/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// shared between the tests (ProcessLaunchParseTests.cpp) and the simple exe that they use to test
// themselves (ProcessLaunchMain.cpp).  The targets are unrelated to each other in CMake but still
// need to include this file in both.

namespace ProcessLaunchTestInternal
{
    // the default buffer size for stdout/stdin on some platforms is 4k.  This number
    // should just be higher than a couple multiples of buffer sizes to ensure that it overwhelms
    // any buffer and reveals any deadlock caused by not reading from stdout/stderr when its full.
    constexpr const size_t s_numOutputBytesInPlentyMode = 16 * 1024 ; 
    constexpr const char* s_beginToken = "BEGIN";
    constexpr const char* s_endToken = "END";
    constexpr const char* s_midToken = "x";
}
