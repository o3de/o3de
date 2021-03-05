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

// Set to 1 to enable internal EBus asserts if debugging an EBus bug
#define EBUS_DEBUG 0

#if EBUS_DEBUG
#include <AzCore/Debug/Trace.h>
#define EBUS_ASSERT(expr, msg) AZ_Assert(expr, msg)
#else
#define EBUS_ASSERT(...)
#endif
