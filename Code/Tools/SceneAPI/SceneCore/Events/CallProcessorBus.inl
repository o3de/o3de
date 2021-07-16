/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/utils.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            template<typename Context, typename... Args>
            ProcessingResult Process(Args&&... args)
            {
                Context context(std::forward<Args>(args)...);
                return Process(context);
            }
        } // Events
    } // SceneAPI
} // AZ
