/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    using std::add_lvalue_reference;
    template< class T >
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    using std::add_rvalue_reference;
    template< class T >
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;
}
