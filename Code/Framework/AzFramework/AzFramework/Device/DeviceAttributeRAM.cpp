/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributeRAM.h>
#include <AzCore/std/string/regex.h>

namespace AzFramework
{
    // Platform specific RAM value retrieval can be found in the relevant platform's code folder
    // e.g. Code/Framework/AzFramework/Platform/Windows/AzFramework/Device/DeviceAttributesCommon_Win.cpp

    AZStd::string_view DeviceAttributeRAM::GetName() const
    {
        return m_name;
    }

    AZStd::string_view DeviceAttributeRAM::GetDescription() const
    {
        return m_description;
    }

    AZStd::any DeviceAttributeRAM::GetValue() const
    {
        return AZStd::any(m_valueInGiB);
    }

    bool DeviceAttributeRAM::Evaluate(AZStd::string_view rule) const
    {
        auto regex = AZStd::regex(rule.data(), rule.size());
        return AZStd::regex_match(m_value, regex);
    }
} // namespace AzFramework

