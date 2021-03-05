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