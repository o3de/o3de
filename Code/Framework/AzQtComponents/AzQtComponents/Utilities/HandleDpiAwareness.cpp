/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/HandleDpiAwareness.h>

namespace AzQtComponents
{
    namespace Utilities
    {
        namespace Platform
        {
            void HandleDpiAwareness(DpiAwareness dpiAwareness);
        }
        
        void HandleDpiAwareness(DpiAwareness dpiAwareness)
        {
            Platform::HandleDpiAwareness(dpiAwareness);
        }

    } // namespace Utilities
} // namespace AZ
