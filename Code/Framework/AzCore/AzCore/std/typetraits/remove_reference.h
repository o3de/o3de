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
    using std::remove_reference;
    template< class T >
    using remove_reference_t = typename remove_reference<T>::type;
}
