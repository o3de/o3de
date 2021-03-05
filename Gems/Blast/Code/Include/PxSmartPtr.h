/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
