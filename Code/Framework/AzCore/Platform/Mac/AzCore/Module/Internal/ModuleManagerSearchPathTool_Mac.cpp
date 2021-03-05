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

