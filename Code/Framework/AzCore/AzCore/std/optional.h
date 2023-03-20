/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <optional>
#include <AzCore/std/utils.h>

namespace AZStd
{

    using std::nullopt_t;
    using std::nullopt;
    using std::optional;
    using std::make_optional;


    // 23.6.10 Hash support
    template<class T>
    struct hash;

    template<class T>
    struct hash<optional<T>>
    {
        constexpr size_t operator()(const optional<T>& opt) const
        {
            return static_cast<bool>(opt) ? hash<T>{}(*opt) : 0;
        }
    };
}
