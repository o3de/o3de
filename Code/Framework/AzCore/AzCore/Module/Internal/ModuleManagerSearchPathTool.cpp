/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ModuleManagerSearchPathTool.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Module/ModuleManagerBus.h>

namespace AZ::Internal
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
} // namespace AZ::Internal
