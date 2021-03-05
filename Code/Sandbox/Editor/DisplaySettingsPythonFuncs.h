#pragma once
/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    //! A legacy component to reflect scriptable commands for the Editor
    class DisplaySettingsPythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DisplaySettingsPythonFuncsHandler, "{517AC40C-4A1F-4E02-ABA2-5A927582ECB4}")

            static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

    //! Class to store the Display Settings
    class DisplaySettingsState
    {
    public:
        AZ_TYPE_INFO(DisplaySettingsState, "{EBEDA5EC-29D3-4F23-ABCC-C7C4EE48FA36}")

        //! Disable collision with terrain.
        bool m_noCollision;
        //! Do not draw labels.
        bool m_noLabels;
        //! Simulation is enabled.
        bool m_simulate;
        //! Enable displaying of animation tracks in views.
        bool m_hideTracks;
        //! Enable displaying of links between objects.
        bool m_hideLinks;
        //! Disable displaying of all object helpers.
        bool m_hideHelpers;
        //! Enable displaying of dimension figures.
        bool m_showDimensionFigures;

        //! Equality test operator
        bool operator==(const DisplaySettingsState& rhs) const;

        //! String representation of the current state
        AZStd::string ToString() const;
    };

    //! API to retrieve and set the Display Settings
    class DisplaySettingsRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get the current display settings state
        virtual DisplaySettingsState GetSettingsState() const = 0;
        //! Set the display settings state
        virtual void SetSettingsState(const DisplaySettingsState& settingsState) = 0;

        ~DisplaySettingsRequests() = default;
    };
    using DisplaySettingsBus = AZ::EBus<DisplaySettingsRequests>;

    //! Component to modify Display Settings 
    class DisplaySettingsComponent final
        : public AZ::Component
        , public DisplaySettingsBus::Handler
    {
    public:
        AZ_COMPONENT(DisplaySettingsComponent, "{A7CDBF22-3904-46C6-85D2-073CD902DD7F}")

        DisplaySettingsComponent() = default;
        ~DisplaySettingsComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // Component...
        void Activate() override;
        void Deactivate() override;

        // DisplaySettingsBus...
        DisplaySettingsState GetSettingsState() const override;
        void SetSettingsState(const DisplaySettingsState& settingsState) override;

        int ConvertToFlags(const DisplaySettingsState& settingsState) const;
        DisplaySettingsState ConvertToSettings(int settings) const;
    };
} // namespace AzToolsFramework
