/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>

#if AZ_DEBUG_BUILD
    #define AZ_MATH_TEST_START_TRACE_SUPPRESSION     AZ_TEST_START_TRACE_SUPPRESSION
    #define AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(x)   AZ_TEST_STOP_TRACE_SUPPRESSION(x)
#else
    #define AZ_MATH_TEST_START_TRACE_SUPPRESSION
    #define AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(x)
#endif
