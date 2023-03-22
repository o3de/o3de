/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <imgui/imgui.h>

#include <ImageComparisonOptions.h>
#include <Utils.h>

namespace ScriptAutomation
{
    void ImageComparisonOptions::Activate()
    {
        m_configAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>("config/ImageComparisonConfig.azasset", AZ::RPI::AssetUtils::TraceLevel::Assert);
        if (m_configAsset)
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_configAsset.GetId());
            OnAssetReloaded(m_configAsset);
        }
    }

    void ImageComparisonOptions::Deactivate()
    {
        m_configAsset.Release();
        ResetImGuiSettings();
        AZ::Data::AssetBus::Handler::BusDisconnect();
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
        if (m_selectedOverrideSetting == OverrideSetting_ScriptControlled)
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

    bool ImageComparisonOptions::IsScriptControlled() const
    {
        return m_selectedOverrideSetting == OverrideSetting_ScriptControlled;
    }

    bool ImageComparisonOptions::IsLevelAdjusted() const
    {
        return m_toleranceAdjustment != 0;
    }

    void ImageComparisonOptions::DrawImGuiSettings()
    {
        ImGui::Text("Tolerance");
        ImGui::Indent();

        if (ImGui::Combo("Level",
            &m_selectedOverrideSetting,
            m_overrideSettings.data(),
            aznumeric_cast<int>(m_overrideSettings.size())))
        {
            if (m_selectedOverrideSetting == OverrideSetting_ScriptControlled)
            {
                m_currentToleranceLevel = nullptr;
            }
            else
            {
                m_currentToleranceLevel = &m_config.m_toleranceLevels[m_selectedOverrideSetting - 1];
            }
        }

        if (IsScriptControlled())
        {
            ImGui::InputInt("Level Adjustment", &m_toleranceAdjustment);
        }

        ImGui::Unindent();
    }

    void ImageComparisonOptions::ResetImGuiSettings()
    {
        m_currentToleranceLevel = nullptr;
        m_selectedOverrideSetting = OverrideSetting_ScriptControlled;
        m_toleranceAdjustment = 0;
    }

    void ImageComparisonOptions::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_configAsset = asset;
        m_config = *m_configAsset->GetDataAs<ImageComparisonConfig>();

        m_overrideSettings.clear();
        m_overrideSettings.push_back("[Script-controlled]");
        for (size_t i = 0; i < m_config.m_toleranceLevels.size(); ++i)
        {
            AZ_Assert(i == 0 || m_config.m_toleranceLevels[i].m_threshold > m_config.m_toleranceLevels[i - 1].m_threshold, "Threshold values are not sequential");

            m_overrideSettings.push_back(m_config.m_toleranceLevels[i].m_name.c_str());
        }
    }

} // namespace ScriptAutomation
