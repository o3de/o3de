/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI.Reflect/Base.h>      // For AZ::RHI::ResultCode

namespace XR
{
    template <typename T>
    using Ptr = AZStd::intrusive_ptr<T>;

    template<typename T>
    using ConstPtr = AZStd::intrusive_ptr<const T>;

    using StringList = AZStd::vector<AZStd::string>;
    using RawStringList = AZStd::vector<const char*>;

    enum class  Side : uint32_t
    {
        Left = 0,
        Right,
        Count
    };

#define RETURN_RESULTCODE_IF_UNSUCCESSFUL(result) \
    if ((result) != AZ::RHI::ResultCode::Success) {\
        return (result);\
    }
}

