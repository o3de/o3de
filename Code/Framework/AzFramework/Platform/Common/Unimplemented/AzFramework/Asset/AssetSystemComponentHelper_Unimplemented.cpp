/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string_view.h>

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {}
    bool LaunchAssetProcessor(AZStd::string_view, AZStd::string_view, AZStd::string_view)
    {
        return false;
    }
}
