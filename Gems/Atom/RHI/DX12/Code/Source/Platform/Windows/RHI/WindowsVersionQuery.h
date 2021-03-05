/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
