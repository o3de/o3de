/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/StandaloneToolsApplication.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace LUAEditor
{
    class Application
        : public StandaloneTools::BaseApplication
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
