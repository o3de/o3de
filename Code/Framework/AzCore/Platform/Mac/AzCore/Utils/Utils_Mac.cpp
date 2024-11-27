/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ::Utils
{
    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath()
    {
        return AZStd::nullopt;
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultDevWriteStoragePath()
    {
        return AZStd::nullopt;
    }
}
