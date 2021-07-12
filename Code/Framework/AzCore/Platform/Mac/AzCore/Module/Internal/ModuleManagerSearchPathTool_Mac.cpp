/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Internal/ModuleManagerSearchPathTool.h>
#include <AzCore/IO/SystemFile.h>

#include <unistd.h>

namespace AZ
{
    namespace Internal
    {
        ModuleManagerSearchPathTool::ModuleManagerSearchPathTool()
        {
            char cwdBuffer[AZ_MAX_PATH_LEN];
            cwdBuffer[0] = '\0';
            getcwd(cwdBuffer, AZ_ARRAY_SIZE(cwdBuffer));
            m_restorePath = cwdBuffer;
        }

        ModuleManagerSearchPathTool::~ModuleManagerSearchPathTool()
        {
            chdir(m_restorePath.c_str());
        }

        void ModuleManagerSearchPathTool::SetModuleSearchPath(const AZ::DynamicModuleDescriptor& moduleDesc)
        {
            AZ::OSString modulePath = GetModuleDirectory(moduleDesc);
            chdir(modulePath.c_str());
        }
    } // namespace Internal
} // namespace AZ

