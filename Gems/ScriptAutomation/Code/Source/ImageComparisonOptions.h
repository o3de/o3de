/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <ImageComparisonConfig.h>

namespace ScriptAutomation
{
    // Manages the available ImageComparisonToleranceLevels and override options
    class ImageComparisonOptions : public AZ::Data::AssetBus::Handler
    {
    public:
        void Activate();
        void Deactivate();

        //! Return the tolerance level with the given name.
        //! The returned level may be adjusted according to the user's "Level Adjustment" setting in ImGui.
        //! @param name name of the tolerance level to find.
        //! @param allowLevelAdjustment may be set to false to avoid applying the user's "Level Adjustment" setting.
        ImageComparisonToleranceLevel* FindToleranceLevel(const AZStd::string& name, bool allowLevelAdjustment = true);

        //! Returns the list of all available tolerance levels, sorted most- to least-strict.
        const AZStd::vector<ImageComparisonToleranceLevel>& GetAvailableToleranceLevels() const;

        //! Sets the active tolerance level.
        //! The selected level may be adjusted according to the user's "Level Adjustment" setting in ImGui.
        //! @param name name of the tolerance level to select.
        //! @param allowLevelAdjustment may be set to false to avoid applying the user's "Level Adjustment" setting.
        void SelectToleranceLevel(const AZStd::string& name, bool allowLevelAdjustment = true);

        //! Sets the active tolerance level.
        //! @param level must be one of the tolerance levels already available.
        void SelectToleranceLevel(ImageComparisonToleranceLevel* level);

        //! Returns the active tolerance level.
        ImageComparisonToleranceLevel* GetCurrentToleranceLevel();

        //! Returns whether the user has configured the script to control tolerance
        //! level selection, otherwise they have selected a specific override level.
        bool IsScriptControlled() const;

        //! Returns true if the user has applied a level up/down adjustment in ImGui.
        bool IsLevelAdjusted() const;

        void DrawImGuiSettings();
        void ResetImGuiSettings();

    private:
        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::Data::Asset<AZ::RPI::AnyAsset> m_configAsset;
        ImageComparisonConfig m_config;
        ImageComparisonToleranceLevel* m_currentToleranceLevel = nullptr;
        static constexpr int OverrideSetting_ScriptControlled = 0;
        AZStd::vector<const char*> m_overrideSettings;
        int m_selectedOverrideSetting = 0;
        int m_toleranceAdjustment = 0;
    };

} // namespace ScriptAutomation
