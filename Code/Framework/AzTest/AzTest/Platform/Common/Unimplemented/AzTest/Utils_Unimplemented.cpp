/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/Utils.h>

namespace AZ
{
    namespace Test
    {
        bool SetEnv([[maybe_unused]] const char* envname, [[maybe_unused]] const char* envvalue, [[maybe_unused]] bool overwrite)
        {
            return false;
        }

        bool UnsetEnv([[maybe_unused]] const char* envname)
        {
            return false;
        }
    } // namespace Test
} // namespace AZ
