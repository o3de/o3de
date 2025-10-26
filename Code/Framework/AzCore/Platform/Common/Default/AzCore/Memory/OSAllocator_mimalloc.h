/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <mimalloc.h>

#define AZ_OS_MALLOC(byteSize, alignment) mi_memalign(alignment, byteSize)
#define AZ_OS_FREE(pointer) mi_free(pointer)
#define AZ_OS_REALLOC(pointer, byteSize, alignment) mi_realloc_aligned(pointer, byteSize, alignment)
#define AZ_OS_MSIZE(pointer, alignment) mi_usable_size(pointer)
#define AZ_MALLOC_TRIM(pad) AZ_UNUSED(pad)
