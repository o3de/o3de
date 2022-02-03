/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZStd
{
    // rvalue
    // rvalue move
    template<class T>
    constexpr AZStd::remove_reference_t<T>&& move(T&& t)
    {
        return static_cast<AZStd::remove_reference_t<T>&&>(t);
    }
}
