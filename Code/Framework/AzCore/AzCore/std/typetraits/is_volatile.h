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
    using std::is_volatile;

    template<class T>
    constexpr bool is_volatile_v = std::is_volatile<T>::value;
}
