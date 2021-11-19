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
        return AZ::Utils::GetExecutableDirectory();
    }

    void* OpenModule(const AZ::OSString& fileName, bool& alreadyOpen)
    {
        void* handle = dlopen(fileName.c_str(), RTLD_NOLOAD);
        alreadyOpen = (handle != nullptr);
        if (!alreadyOpen)
        {
            handle = dlopen(fileName.c_str(), RTLD_NOW);
        }
        return handle;
    }

    void ConstructModuleFullFileName(AZ::IO::FixedMaxPath&)
    {
    }
} // namespace AZ::Platform
