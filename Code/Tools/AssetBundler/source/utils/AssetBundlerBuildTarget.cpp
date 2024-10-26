/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/string/string_view.h>

namespace AssetBundler
{
    //! This file is to be added only to the AssetBundler build target
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (O3DE_CMAKE_TARGET)
#error "O3DE_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ O3DE_CMAKE_TARGET };
    }
}
