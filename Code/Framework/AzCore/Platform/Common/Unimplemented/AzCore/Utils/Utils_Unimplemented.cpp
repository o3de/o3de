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
    AZ::IO::FixedMaxPathString GetHomeDirectory(AZ::SettingsRegistryInterface*)
    {
        return {};
    }

    GetEnvOutcome GetEnv(AZStd::span<char>, const char*)
    {
        return AZ::Failure(GetEnvErrorResult{ GetEnvErrorCode::NotImplemented });
    }

    bool IsEnvSet(const char*)
    {
        return false;
    }

    bool SetEnv([[maybe_unused]] const char* envname, [[maybe_unused]] const char* envvalue, [[maybe_unused]] bool overwrite)
    {
        return false;
    }

    bool UnsetEnv([[maybe_unused]] const char* envname)
    {
        return false;
    }
} // namespace AZ::Utils
