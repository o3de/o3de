/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/MathUtils.h>

namespace AzToolsFramework
{   
    // The type used internally by Qt for widget values    
    using QtWidgetValueType = int;

    // All LY widget types and values will be clamped so as to not exceed the range of QtWidgetValueType
    template<typename SourceType>
    using QtWidgetLimits = AZ::ClampedIntegralLimits<SourceType, QtWidgetValueType>;
} // AzToolsFramework
