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

#ifndef WOODPECKER_APPLICATION_H
#define WOODPECKER_APPLICATION_H

#include <AzCore/base.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Component/ComponentApplication.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkApplication.h>

namespace Woodpecker
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
        AZStd::string ResolveFilePath(AZ::u32 /*providerId*/);
        //////////////////////////////////////////////////////////////////////////
    };
}

#endif
