/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(_RELEASE)
    #define AZ_MEMORY_PROFILE(...) (__VA_ARGS__)
#else
    #define AZ_MEMORY_PROFILE(...)
#endif
