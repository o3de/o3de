/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>

namespace AZ
{
    class Name;

    namespace Render
    {
        //! Check if the given pass name exist and is enabled
        //! @return optional boolean with no value if failed to find the pass. If found, return true if enabled and false if not.
        AZStd::optional<bool> IsPassEnabled(const Name& name);

        //! Toggle temporal anti aliasing pass from the default viewport
        //! @return boolean true on success, false if failed to get the pass
        bool EnableTAA(bool enable);

    } // namespace Render
} // namespace AZ
