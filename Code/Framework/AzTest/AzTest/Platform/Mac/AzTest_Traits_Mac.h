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
#define AZ_TRAIT_DISABLE_FAILED_DLL_TESTS true
#define AZ_TRAIT_DISABLE_FAILED_MODULE_TESTS true
#define AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS true

// Golden perline gradiant values for random seed 7878 for this platform
#define AZ_TRAIT_UNIT_TEST_PERLINE_GRADIANT_GOLDEN_VALUES_7878  0.5000f, 0.5276f, 0.5341f, 0.4801f, \
                                                                0.5220f, 0.5162f, 0.4828f, 0.5431f, \
                                                                0.4799f, 0.4486f, 0.5054f, 0.4129f, \
                                                                0.6023f, 0.5029f, 0.4529f, 0.4428f,
