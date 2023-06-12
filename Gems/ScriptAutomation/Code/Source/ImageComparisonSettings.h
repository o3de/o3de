/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ImageComparisonConfig.h>


namespace AZ::ScriptAutomation
{
    // Manages the available ImageComparisonToleranceLevels
    class ImageComparisonSettings
    {
    public:
        //! Return the tolerance level with the given name.
        //! The returned level may be adjusted according to the user's "Level Adjustment" setting in ImGui.
        //! @param name name of the tolerance level to find.
        ImageComparisonToleranceLevel* FindToleranceLevel(const AZStd::string& name);

        //! Returns the list of all available tolerance levels, sorted most- to least-strict.
        const AZStd::vector<ImageComparisonToleranceLevel>& GetAvailableToleranceLevels() const;


    private:
        bool IsReady() const {return m_ready;}
        void GetToleranceLevelsFromSettingsRegistry();

        ImageComparisonConfig m_config;
        bool m_ready = false;
    };
} // namespace AZ::ScriptAutomation
