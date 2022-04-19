/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <malloc.h>

# define AZ_OS_MALLOC(byteSize, alignment) ::memalign(alignment, byteSize)
# define AZ_OS_FREE(pointer) ::free(pointer)
# define AZ_MALLOC_TRIM(pad) ::malloc_trim(pad)
