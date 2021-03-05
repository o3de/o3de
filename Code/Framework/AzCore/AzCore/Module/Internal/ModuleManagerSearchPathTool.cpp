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

#include "ModuleManagerSearchPathTool.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Module/ModuleManagerBus.h>

namespace AZ
{
    namespace Internal
    {
        AZ::OSString ModuleManagerSearchPathTool::GetModuleDirectory(const AZ::DynamicModuleDescriptor& moduleDesc)
        {
            // For each module that is loaded, attempt to set the module's folder as a path for dependent module resolution
            AZ::OSString modulePath = moduleDesc.m_dynamicLibraryPath;
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, modulePath);
            auto lastPathSep = modulePath.find_last_of(AZ_TRAIT_OS_PATH_SEPARATOR);
            if (lastPathSep != modulePath.npos)
            {
                modulePath = modulePath.substr(0, lastPathSep);
            }
            return modulePath;
        }
    } // namespace Internal
} // namespace AZ