/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_AZTEST_ATTACH_RESULT_LISTENER 0

#define AZ_TRAIT_UNIT_TEST_ASSET_MANAGER_TEST_DEFAULT_TIMEOUT_SECS 5
#define AZ_TRAIT_UNIT_TEST_ENTITY_ID_GEN_TEST_COUNT 10000
#define AZ_TRAIT_UNIT_TEST_DILLER_TRIGGER_EVENT_COUNT 100000

#define AZ_TRAIT_DISABLE_ASSET_JOB_PARALLEL_TESTS true
#define AZ_TRAIT_DISABLE_ASSET_MANAGER_FLOOD_TEST true
#define AZ_TRAIT_DISABLE_ASSETCONTAINERDISABLETEST true

// Golden perline gradiant values for random seed 7878 for this platform
#define AZ_TRAIT_UNIT_TEST_PERLINE_GRADIANT_GOLDEN_VALUES_7878  0.5000f, 0.5456f, 0.5138f, 0.4801f, \
                                                                0.4174f, 0.4942f, 0.5493f, 0.5431f, \
                                                                0.4984f, 0.5204f, 0.5526f, 0.5840f, \
                                                                0.5251f, 0.5029f, 0.6153f, 0.5802f,
