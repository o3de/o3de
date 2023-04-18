/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageComparisonSettings.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ::ScriptAutomation
{
    void ImageComparisonSettings::Activate()
    {
        m_configAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>("config/imagecomparisonconfig.azasset", AZ::RPI::AssetUtils::TraceLevel::Assert);
        if (m_configAsset)
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_configAsset.GetId());
            OnAssetReloaded(m_configAsset);
        }
    }

    void ImageComparisonSettings::Deactivate()
    {
        m_configAsset.Release();
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    ImageComparisonToleranceLevel* ImageComparisonSettings::FindToleranceLevel(const AZStd::string& name)
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

        return &m_config.m_toleranceLevels[foundIndex];
    }

    const AZStd::vector<ImageComparisonToleranceLevel>& ImageComparisonSettings::GetAvailableToleranceLevels() const
    {
        return m_config.m_toleranceLevels;
    }

    void ImageComparisonSettings::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_configAsset = asset;
        m_config = *m_configAsset->GetDataAs<ImageComparisonConfig>();
        m_ready = true;
    }
} // namespace AZ::ScriptAutomation
