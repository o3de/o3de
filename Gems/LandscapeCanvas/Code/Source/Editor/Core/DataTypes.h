/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string_view.h>

// GraphModel
#include <GraphModel/Model/DataType.h>

namespace LandscapeCanvas
{
    enum LandscapeCanvasDataTypeEnum
    {
        Bounds = 0,
        Gradient,
        Area,
        String,
        Path,
        Count
    };

    static const AZ::Uuid BoundsTypeId = "{746398F1-0325-4A56-A544-FEF561C24E7C}";
    static const AZ::Uuid GradientTypeId = "{F38CF64A-1EB6-41FA-A2CC-73D19B48E59E}";
    static const AZ::Uuid AreaTypeId = "{FE1878D9-D445-4652-894B-D6348706EEAE}";
    static const AZ::Uuid PathTypeId = "{1100A9EB-7042-45F1-9A71-BA3117AE6D99}";
}
