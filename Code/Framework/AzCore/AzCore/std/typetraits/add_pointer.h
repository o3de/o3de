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
    using std::add_pointer;
    template<class Type>
    using add_pointer_t = std::add_pointer_t<Type>;
}
