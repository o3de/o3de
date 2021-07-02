/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            struct WindowsVersion
            {
                WORD m_majorVersion = 0;
                WORD m_minorVersion = 0;
                WORD m_buildVersion = 0;
            };

            //! Returns true if the function was able to successfully read the host machine's Windows version information. False otherwise.
            bool GetWindowsVersion(WindowsVersion* WindowsVersion);
        }
    }
}
