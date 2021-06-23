/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    using std::is_pointer;
    template<typename T>
    inline constexpr bool is_pointer_v = std::is_pointer_v<T>;
}
