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
#ifndef WOODPECKER_PROFILER_APPLICATION_H
#define WOODPECKER_PROFILER_APPLICATION_H

#include <Woodpecker/WoodpeckerApplication.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace LUAEditor
{
    class Application
        : public Woodpecker::BaseApplication
        , protected AzToolsFramework::SourceControlNotificationBus::Handler
    {
    public:
        Application(int &argc, char **argv);
        ~Application() override;

    protected:
        void RegisterCoreComponents() override;
        void CreateApplicationComponents() override;

        void ConnectivityStateChanged(const AzToolsFramework::SourceControlState /*connected*/) override;
    };
}

#endif
