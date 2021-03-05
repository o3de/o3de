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
        GameApplication(int* argc, char*** argvS);
        ~GameApplication();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
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

