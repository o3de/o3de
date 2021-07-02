/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Module/Internal/ModuleManagerSearchPathTool.h>

namespace AZ
{
    namespace Internal
    {
        ModuleManagerSearchPathTool::ModuleManagerSearchPathTool()
        {
        }

        ModuleManagerSearchPathTool::~ModuleManagerSearchPathTool()
        {
            ::SetDllDirectoryA(NULL);
        }

        void ModuleManagerSearchPathTool::SetModuleSearchPath(const AZ::DynamicModuleDescriptor& moduleDesc)
        {
            AZ::OSString modulePath = GetModuleDirectory(moduleDesc);
            ::SetDllDirectoryA(modulePath.c_str());
        }
    } // namespace Internal
} // namespace AZ
