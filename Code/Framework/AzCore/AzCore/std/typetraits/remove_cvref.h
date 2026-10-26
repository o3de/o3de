/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/remove_reference.h>

namespace AZStd
{
    using std::remove_cvref;
    using std::remove_cvref_t;
}
