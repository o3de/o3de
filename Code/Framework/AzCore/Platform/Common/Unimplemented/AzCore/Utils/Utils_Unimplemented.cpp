/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

namespace AZ::Utils
{
    AZ::IO::FixedMaxPathString GetHomeDirectory()
    {
        return {};
    }

    bool SetEnv(const char* envname, const char* envvalue, bool overwrite)
    {
        AZ_UNUSED(envname);
        AZ_UNUSED(envvalue);
        AZ_UNUSED(overwrite);

        return false;
    }

    bool UnSetEnv(const char* envname)
    {
        AZ_UNUSED(envname);

        return false;
    }
} // namespace AZ::Utils
