/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzQtComponents/AzQtComponentsAPI.h>

namespace AzQtComponents
{
    namespace Utilities
    {
        enum DpiAwareness {
            Unset,
            Unaware,
            SystemDpiAware,
            PerScreenDpiAware
        };

        AZ_QT_COMPONENTS_API void HandleDpiAwareness(DpiAwareness dpiAwareness);

    } // namespace Utilities
} // namespace AZ
