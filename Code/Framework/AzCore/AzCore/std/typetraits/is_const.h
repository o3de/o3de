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
    using std::is_const;
    template<class T>
    constexpr bool is_const_v = std::is_const<T>::value;

}
