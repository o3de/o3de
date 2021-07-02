/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

/**
* This file is to be included from UnixLike SystemFile implementations only. It should NOT be included by the user.
*/

namespace AZ
{
    namespace IO
    {
        namespace Internal
        {
            bool FormatAndPeelOffWildCardExtension(const char* sourcePath, char* filePath, size_t filePathSize, char* extensionPath, size_t extensionSize, bool keepWildcard = false);
        }
    }
}
