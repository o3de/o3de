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

    void* OpenModule(const AZ::IO::FixedMaxPathString& fileName, bool&, bool globalSymbols)
    {
        // Android 19 does not have RTLD_NOLOAD but it should be OK since only the Editor expects to reopen modules
        int openFlags = globalSymbols ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;
        return dlopen(fileName.c_str(), openFlags);
    }

    void ConstructModuleFullFileName(AZ::IO::FixedMaxPath&)
    {
    }

    AZ::IO::FixedMaxPath CreateFrameworkModulePath(const AZ::IO::PathView&)
    {
        return {};
    }
} // namespace AZ::Platform
