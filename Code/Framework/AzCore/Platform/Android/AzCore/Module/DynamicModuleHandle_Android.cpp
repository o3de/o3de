/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/osstring.h>
#include <AzCore/Utils/Utils.h>
#include <dlfcn.h>

namespace AZ::Platform
{
    AZ::IO::FixedMaxPath GetModulePath()
    {
        return {};
    }

    void* OpenModule(const AZ::IO::FixedMaxPathString& fileName, bool&)
    {
        // Android 19 does not have RTLD_NOLOAD but it should be OK since only the Editor expects to reopen modules
        return dlopen(fileName.c_str(), RTLD_NOW);
    }

    void ConstructModuleFullFileName(AZ::IO::FixedMaxPath&)
    {
    }

    AZ::IO::FixedMaxPath CreateFrameworkModulePath(const AZ::IO::PathView&)
    {
        return {};
    }
} // namespace AZ::Platform
