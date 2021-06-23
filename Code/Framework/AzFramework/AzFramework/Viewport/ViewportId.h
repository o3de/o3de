/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
