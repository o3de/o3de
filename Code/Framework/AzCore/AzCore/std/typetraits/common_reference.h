/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/common_type.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>

namespace AZStd
{
    using std::basic_common_reference;
    using std::common_reference;
    using std::common_reference_t;
    using std::common_reference_with;
}
