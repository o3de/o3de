/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptEvents
{
    struct FundamentalTypes final
    {
        AZ_TYPE_INFO(FundamentalTypes, "{7BEB1932-2CE2-4786-8320-E71B4E35FCFF}");
        using TypeData = AZStd::unordered_map<AZ::Uuid, AZStd::string>;
        TypeData m_typeData;

        FundamentalTypes()
        {
            m_typeData = {
                    { azrtti_typeid<bool>(), "Boolean" },
                    { azrtti_typeid<short>(), "Number" },
                    { azrtti_typeid<AZ::s64>(), "Number" },
                    { azrtti_typeid<long>(), "Number" },
                    { azrtti_typeid<unsigned char>(), "Number" },
                    { azrtti_typeid<unsigned short>(), "Number" },
                    { azrtti_typeid<unsigned int>(), "Number" },
                    { azrtti_typeid<int>(), "Number" },
                    { azrtti_typeid<AZ::u64>(), "Number" },
                    { azrtti_typeid<unsigned long>(), "Number" },
                    { azrtti_typeid<float>(), "Number" },
                    { azrtti_typeid<double>(), "Number" },
                    { azrtti_typeid<AZStd::string>(), "String" },
                    { azrtti_typeid<AZStd::string_view>(), "String" },
                    { azrtti_typeid<const char*>(), "String" }
            };
        }

        const char* FindFundamentalTypeName(const AZ::Uuid& typeId) const
        {
            auto seek = m_typeData.find(typeId);
            const char* result = (seek != m_typeData.end()) ? seek->second.c_str() : "";

            return result;
        }

        bool IsFundamentalType(const AZ::Uuid& uuid) const
        {
            bool result = m_typeData.find(uuid) != m_typeData.end();

            return result;
        }

    };
}
