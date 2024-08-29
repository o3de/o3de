/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define ASSETPROCESSOR_TRAIT_CASE_SENSITIVE_FILESYSTEM false
#define ASSETPROCESSOR_TRAIT_ASSET_BUILDER_LOST_CONNECTION_RETRIES 1

// Even though AP can handle files with path length greater than window's legacy path length limit, we have some 3rdparty sdk's
// which do not handle this case, therefore we will make AP fail any jobs whose either source file or output file name exceeds the windows
// legacy path length limit
#define ASSETPROCESSOR_TRAIT_MAX_PATH_LEN 260
