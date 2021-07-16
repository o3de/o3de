/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>

namespace WhiteBox
{
    //! Type alias for tightly packed two-component float vector in device friendly format.
    using PackedFloat2 = AZStd::array<float, 2>;
} // namespace WhiteBox
