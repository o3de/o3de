/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <climits>

#ifndef PATH_MAX
#pragma message "PATH_MAX not defined in <climits>. Setting it to 4096."
#define PATH_MAX 4096
#endif

#define ASSETPROCESSOR_TRAIT_CASE_SENSITIVE_FILESYSTEM true
#define ASSETPROCESSOR_TRAIT_ASSET_BUILDER_LOST_CONNECTION_RETRIES 5
#define ASSETPROCESSOR_TRAIT_MAX_PATH_LEN PATH_MAX
