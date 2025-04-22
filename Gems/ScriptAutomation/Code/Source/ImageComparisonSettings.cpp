/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageComparisonSettings.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::ScriptAutomation
{
    void ImageComparisonSettings::GetToleranceLevelsFromSettingsRegistry()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            const char* imageComparisonSettingsPath = "/O3DE/ScriptAutomation/ImageComparisonSettings";

            m_ready = settingsRegistry->GetObject(m_config, imageComparisonSettingsPath);
        }
    }

    ImageComparisonToleranceLevel* ImageComparisonSettings::FindToleranceLevel(const AZStd::string& name)
    {
        if (!IsReady())
        {
            GetToleranceLevelsFromSettingsRegistry();
        }
        AZ_Assert(IsReady(), "Failed to get image comparison tolerance levels from the settings registry");
        size_t foundIndex = m_config.m_toleranceLevels.size();

        for (size_t i = 0; i < m_config.m_toleranceLevels.size(); ++i)
        {
            if (m_config.m_toleranceLevels[i].m_name == name)
            {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex == m_config.m_toleranceLevels.size())
        {
            return nullptr;
        }

        return &m_config.m_toleranceLevels[foundIndex];
    }
} // namespace AZ::ScriptAutomation
