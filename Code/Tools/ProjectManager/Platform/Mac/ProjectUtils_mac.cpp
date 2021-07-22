/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<void, QString> FindSupportedCompilerForPlatform()
        {
            // Compiler detection not supported on platform
            return AZ::Success();
        }
        
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
