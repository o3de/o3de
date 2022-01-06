/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ShaderManagementConsoleApplication.h>

int main(int argc, char** argv)
{
    AzQtComponents::AzQtApplication::InitializeDpiScaling();

    ShaderManagementConsole::ShaderManagementConsoleApplication app(&argc, &argv);
    if (app.LaunchLocalServer())
    {
        app.Start(AZ::ComponentApplication::Descriptor{});
        app.exec();
        app.Stop();
    }

    return 0;
}
