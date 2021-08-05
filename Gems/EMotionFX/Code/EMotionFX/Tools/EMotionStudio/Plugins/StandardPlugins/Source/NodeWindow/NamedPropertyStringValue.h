/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
