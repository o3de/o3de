/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributeGPUModel.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/regex.h>

namespace AzFramework
{
    DeviceAttributeGPUModel::DeviceAttributeGPUModel(const AZStd::vector<AZStd::string_view>& value)
    {
        AZStd::transform(
            value.begin(),
            value.end(),
            std::back_inserter(m_value),
            [](const auto& value)
            {
                return value;
            });
    }

    AZStd::string_view DeviceAttributeGPUModel::GetName() const
    {
        return m_name;
    }

    AZStd::string_view DeviceAttributeGPUModel::GetDescription() const
    {
        return m_description;
    }

    AZStd::any DeviceAttributeGPUModel::GetValue() const
    {
        return AZStd::any(m_value);
    }

    bool DeviceAttributeGPUModel::Evaluate(AZStd::string_view rule) const
    {
        // Evaluate against all of the GPUs
        auto regex = AZStd::regex(rule.data(), rule.size());
        for (const auto& gpu : m_value)
        {
            if (AZStd::regex_match(gpu, regex))
            {
                return true;
            }
        }
        return false;
    }
} // namespace AzFramework

