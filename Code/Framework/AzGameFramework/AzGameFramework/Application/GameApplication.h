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
        AZ_CLASS_ALLOCATOR(GameApplication, AZ::SystemAllocator, 0);

        GameApplication();
        GameApplication(int argc, char** argvS);
        ~GameApplication();

        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationRequests::Bus
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

    protected:

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void StartCommon(AZ::Entity* systemEntity) override;

        // Instead of iterating over the individual settings folders to composite the Settings Registry, load a pre-generated
        // game.*.setreg instead. In non-release builds this will still load the dev user settings and the command line settings.
        void MergeSettingsToRegistry(AZ::SettingsRegistryInterface& registry) override;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzGameFramework

