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

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace EMStudio
{
    class NamedPropertyStringValue final
    {
    public:
        AZ_RTTI(NamedPropertyStringValue, "{38550727-AF3A-49E6-AF63-99679F48F91B}")
        AZ_CLASS_ALLOCATOR_DECL

        NamedPropertyStringValue() {}
        NamedPropertyStringValue(const AZStd::string& name, const AZStd::string& value);
        ~NamedPropertyStringValue() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        const AZStd::string& GetName() const { return m_name; }

        AZStd::string m_name;
        AZStd::string m_value;
    };
} // namespace EMStudio
