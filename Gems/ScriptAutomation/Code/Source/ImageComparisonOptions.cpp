/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <imgui/imgui.h>

#include <ImageComparisonOptions.h>
#include <Utils.h>

namespace ScriptAutomation
{
    void ImageComparisonOptions::Activate()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        static const AZStd::string setregPath = "/O3DE/ScriptAutomation/ImageComparisonConfig";

        if (settingsRegistry)
        {
            settingsRegistry->GetObject(&m_config, azrtti_typeid(m_config), setregPath.c_str());
        }
    }

    void ImageComparisonOptions::Deactivate()
    {
        ResetImGuiSettings();
    }

    ImageComparisonToleranceLevel* ImageComparisonOptions::FindToleranceLevel(const AZStd::string& name, bool allowLevelAdjustment)
    {
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

        if (allowLevelAdjustment)
        {
            int adjustedIndex = aznumeric_cast<int>(foundIndex);
            adjustedIndex += m_toleranceAdjustment;
            adjustedIndex = AZ::GetClamp(adjustedIndex, 0, aznumeric_cast<int>(m_config.m_toleranceLevels.size()) - 1);
            foundIndex = aznumeric_cast<size_t>(adjustedIndex);
        }

        return &m_config.m_toleranceLevels[foundIndex];
    }

    const AZStd::vector<ImageComparisonToleranceLevel>& ImageComparisonOptions::GetAvailableToleranceLevels() const
    {
        return m_config.m_toleranceLevels;
    }

    void ImageComparisonOptions::SelectToleranceLevel(const AZStd::string& name, bool allowLevelAdjustment)
    {
        ImageComparisonToleranceLevel* level = FindToleranceLevel(name, allowLevelAdjustment);

        if (level)
        {
            m_currentToleranceLevel = level;
        }
        else
        {
            AZ_Error("ScriptAutomation", false, "ImageComparisonToleranceLevel '%s' not found.", name.c_str());
        }
    }

    void ImageComparisonOptions::SelectToleranceLevel(ImageComparisonToleranceLevel* level)
    {
        if (nullptr == level)
        {
            m_currentToleranceLevel = level;
            return;
        }
        else
        {
            SelectToleranceLevel(level->m_name);
            AZ_Assert(GetCurrentToleranceLevel() == level, "Wrong ImageComparisonToleranceLevel pointer used");
        }
    }

    ImageComparisonToleranceLevel* ImageComparisonOptions::GetCurrentToleranceLevel()
    {
        return m_currentToleranceLevel;
    }

    bool ImageComparisonOptions::IsLevelAdjusted() const
    {
        return m_toleranceAdjustment != 0;
    }

    void ImageComparisonOptions::DrawImGuiSettings()
    {
        ImGui::Text("Tolerance");
        ImGui::Indent();

        ImGui::InputInt("Level Adjustment", &m_toleranceAdjustment);

        ImGui::Unindent();
    }

    void ImageComparisonOptions::ResetImGuiSettings()
    {
        m_currentToleranceLevel = nullptr;
        m_toleranceAdjustment = 0;
    }
} // namespace ScriptAutomation
