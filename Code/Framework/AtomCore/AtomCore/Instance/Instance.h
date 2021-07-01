/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace AZ
{
    namespace Data
    {
        /**
         * This is an alias of intrusive_ptr designed for any class which inherits from
         * InstanceData. You're not required to use Instance<> over AZStd::intrusive_ptr<>,
         * but it provides symmetry with Asset<>.
         */
        template <typename T>
        using Instance = AZStd::intrusive_ptr<T>;
    }
}
