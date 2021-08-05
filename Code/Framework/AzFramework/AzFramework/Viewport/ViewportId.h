/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <limits>

namespace AzFramework
{
    using ViewportId = int;
    static constexpr ViewportId InvalidViewportId = std::numeric_limits<ViewportId>::max();
} //namespace AzFramework
