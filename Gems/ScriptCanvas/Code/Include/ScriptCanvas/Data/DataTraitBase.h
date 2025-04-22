/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/any.h>
#include <ScriptCanvas/Data/DataType.h>

namespace ScriptCanvas
{
    namespace Data
    {
        template<typename t_Type>
        struct TraitsBase
        {
            using ThisType = TraitsBase<t_Type>;
            using Type = AZStd::decay_t<t_Type>;
            static const bool s_isAutoBoxed = false;
            static const bool s_isKey = false;
            static const bool s_isNative = false;
            static const eType s_type = eType::Invalid;

            static AZ::Uuid GetAZType(const Data::Type& = {})
            {
                return azrtti_typeid<t_Type>();
            }
            // Force a compiler error if Traits class doesn't have explicit specialization
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) = delete;
            static AZStd::string GetName(const Data::Type& = {}) = delete;
            static Type GetDefault(const Data::Type& = {}) = delete;
            static bool IsDefault(const AZStd::any&, const Data::Type& = {}) = delete;
        };

        template<typename t_Type>
        struct Traits : public TraitsBase<t_Type>
        {
        };

        // A compile time map of eType back to underlying AZ type and traits
        template<eType>
        struct eTraits
        {
        };
    } // namespace Data
} // namespace ScriptCanvas
