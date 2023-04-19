/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ImageComparisonConfig.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>


namespace AZ::ScriptAutomation
{
    // Manages the available ImageComparisonToleranceLevels
    class ImageComparisonSettings : public AZ::Data::AssetBus::Handler
    {
    public:
        void Activate();
        void Deactivate();

        //! Return the tolerance level with the given name.
        //! The returned level may be adjusted according to the user's "Level Adjustment" setting in ImGui.
        //! @param name name of the tolerance level to find.
        ImageComparisonToleranceLevel* FindToleranceLevel(const AZStd::string& name);

        //! Returns the list of all available tolerance levels, sorted most- to least-strict.
        const AZStd::vector<ImageComparisonToleranceLevel>& GetAvailableToleranceLevels() const;

        bool IsReady() const {return m_ready;}

    private:

        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::Data::Asset<AZ::RPI::AnyAsset> m_configAsset;
        ImageComparisonConfig m_config;
        bool m_ready = false;
    };
} // namespace AZ::ScriptAutomation
