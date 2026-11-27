/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/base.h>

namespace AnchorPresets
{
    static const int PresetIndexCount = 16;

    int AnchorToPresetIndex(const AZ::Vector4& v);
    const AZ::Vector4& PresetIndexToAnchor(int i);
} // namespace AnchorPresets
