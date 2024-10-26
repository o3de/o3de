/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>
#include <AzFramework/Application/Application.h>

#pragma once

namespace AzGameFramework
{
    class GameApplication
        : public AzFramework::Application
    {
    public:
        AZ_CLASS_ALLOCATOR(GameApplication, AZ::SystemAllocator);

        GameApplication();
        GameApplication(int argc, char** argvS);
        // Allows passing in a JSON Merge Patch string that can bootstrap
        // the settings registry with an initial set of settings
        explicit GameApplication(AZ::ComponentApplicationSettings componentAppSettings);
        GameApplication(int argc, char** argvS, AZ::ComponentApplicationSettings componentAppSettings);
        ~GameApplication();

        //////////////////////////////////////////////////////////////////////////
        // AZ::ComponentApplication
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationRequests::Bus
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        //! Set the headless state of the game application. This is determined at compile time
        void SetHeadless(bool headless);

        //! Set the flag indicating if console-only mode is supported. This is determined based on the platform that supports it.
        void SetConsoleModeSupported(bool supported);

    protected:

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void StartCommon(AZ::Entity* systemEntity) override;

        // Instead of iterating over the individual settings folders to composite the Settings Registry, load a pre-generated
        // game.*.setreg instead. In non-release builds this will still load the dev user settings and the command line settings.
        void MergeSettingsToRegistry(AZ::SettingsRegistryInterface& registry) override;
        //////////////////////////////////////////////////////////////////////////

        bool m_headless{ false };

        bool m_consoleModeSupported{ true };

        bool m_consoleMode{ false };
    };
} // namespace AzGameFramework

