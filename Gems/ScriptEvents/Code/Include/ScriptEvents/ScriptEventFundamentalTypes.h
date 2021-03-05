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
