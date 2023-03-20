/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_OS_MALLOC(byteSize, alignment) _aligned_malloc(byteSize, alignment)
#define AZ_OS_FREE(pointer) _aligned_free(pointer)
#define AZ_OS_REALLOC(pointer, byteSize, alignment) _aligned_realloc(pointer, byteSize, alignment)
#define AZ_OS_MSIZE(pointer, alignment) _aligned_msize(pointer, alignment, 0)
#define AZ_MALLOC_TRIM(pad) AZ_UNUSED(pad)
