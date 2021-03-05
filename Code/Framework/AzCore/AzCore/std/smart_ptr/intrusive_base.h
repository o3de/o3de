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

#include "intrusive_refcount.h"
#include <AzCore/std/parallel/atomic.h>

namespace AZStd
{
    /**
     * An intrusive reference counting base class that is compliant with AZStd::intrusive_ptr. This
     * version is thread-safe through use of an implicit atomic reference count. Explicit friending of
     * intrusive_ptr is used; the user must use intrusive_ptr to take a reference on the object.
     */
    using intrusive_base = intrusive_refcount<atomic_uint>;

} // namespace AZStd