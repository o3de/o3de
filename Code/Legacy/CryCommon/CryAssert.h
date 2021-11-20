/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Assert dialog box

#pragma once

#include <AzCore/base.h>

// Using AZ_Assert for all assert kinds (assert =, CRY_ASSERT, AZ_Assert).
// see Trace::Assert for implementation
#undef assert
#define assert(condition) AZ_Assert(condition, "%s", #condition)

#define CRY_ASSERT(condition) AZ_Assert(condition, "%s", #condition)
#define CRY_ASSERT_MESSAGE(condition, message) AZ_Assert(condition, message)
#define CRY_ASSERT_TRACE(condition, parenthese_message) AZ_Assert(condition, parenthese_message)

