/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PassCanvasApplication.h>

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    AzQtComponents::AzQtApplication::InitializeDpiScaling();

    PassCanvas::PassCanvasApplication app(&argc, &argv);
    if (app.LaunchLocalServer())
    {
        app.Start({}, {});
        app.RunMainLoop();
        app.Stop();
    }

    return 0;
}
