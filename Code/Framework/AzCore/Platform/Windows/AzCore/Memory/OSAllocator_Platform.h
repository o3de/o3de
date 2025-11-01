/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(AZCORE_OS_ALLOCATOR_USE_MIMALLOC)
#include <../Common/Default/AzCore/Memory/OSAllocator_mimalloc.h>
#else
#include <../Common/WinAPI/AzCore/Memory/OSAllocator_WinAPI.h>
#endif
