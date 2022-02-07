/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Component/ComponentApplication.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkApplication.h>

namespace StandaloneTools
{
    class BaseApplication
        : public LegacyFramework::Application
        , AZ::UserSettingsFileLocatorBus::Handler
    {
    public:

        BaseApplication(int &argc, char **argv);
        ~BaseApplication() override;

    protected:

        void RegisterCoreComponents() override;
        void CreateSystemComponents() override;
        void CreateApplicationComponents() override;
        void OnApplicationEntityActivated() override;

        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
    private:
        AZStd::string GetStoragePath() const;
        
        bool LaunchDiscoveryService();

        // AZ::UserSettingsFileLocatorBus::Handler
        AZStd::string ResolveFilePath(AZ::u32 /*providerId*/) override;
        //////////////////////////////////////////////////////////////////////////
    };
}
