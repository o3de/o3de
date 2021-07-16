/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// types like AZ::u64 require an usigned long long, but inttypes.h has it as unsigned long
#undef PRIX64
#undef PRIx64
#undef PRId64
#undef PRIu64
#define PRIX64 "llX"
#define PRIx64 "llx"
#define PRId64 "lld"
#define PRIu64 "llu"
