/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::ShaderCompiler
{
    using SpaceIndex = uint32_t; // represents register space0, space1, ...

    // We start at an arbitrarily high number to avoid conflicts with spaces occupied by any user declarations
    constexpr const SpaceIndex FirstUnboundedSpace = 1000;
}
