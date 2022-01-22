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
        bool SetEnv(const char* envname, const char* envvalue, bool overwrite)
        {
            return setenv(envname, envvalue, overwrite) != -1;
        }

        bool UnsetEnv(const char* envname)
        {
            return unsetenv(envname) != -1;
        }
    }
}
