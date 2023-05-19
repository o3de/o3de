/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_AZTEST_ATTACH_RESULT_LISTENER 0

#define AZ_TRAIT_UNIT_TEST_ASSET_MANAGER_TEST_DEFAULT_TIMEOUT_SECS 15
#define AZ_TRAIT_UNIT_TEST_ENTITY_ID_GEN_TEST_COUNT 10000
#define AZ_TRAIT_UNIT_TEST_DILLER_TRIGGER_EVENT_COUNT 100000

#define AZ_TRAIT_DISABLE_FAILED_NATIVE_WINDOWS_TESTS    true

#if __ARM_ARCH
#define AZ_TRAIT_DISABLE_FAILED_ARM64_TESTS             true
#endif // __ARM_ARCH
