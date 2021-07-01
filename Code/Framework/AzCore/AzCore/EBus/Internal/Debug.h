/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
