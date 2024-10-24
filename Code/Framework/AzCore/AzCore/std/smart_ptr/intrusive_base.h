/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    extern template class AZCORE_API_EXTERN intrusive_refcount<atomic_uint>;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
} // namespace AZStd
