/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace physx
{

    template <typename T>
    struct PxPtrReleaser
    {
        void operator()(T* ptr)
        {
            if (ptr)
            {
                ptr->release();
            }
        }
    };

    template <typename T>
    using unique_ptr = AZStd::unique_ptr<T, PxPtrReleaser<T>>;
}
