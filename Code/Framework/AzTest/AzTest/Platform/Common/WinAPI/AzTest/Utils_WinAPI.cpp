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
        bool SetEnv(const char* envname, const char* envvalue, [[maybe_unused]] bool overwrite)
        {
            return _putenv_s(envname, envvalue);
        }

        bool UnsetEnv(const char* envname)
        {
            return SetEnv(envname, "", 1);
        }
    }
}
