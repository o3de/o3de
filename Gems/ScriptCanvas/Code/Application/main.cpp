/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasApplication.h"

#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Application/AzQtApplication.h>

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
    AzQtComponents::AzQtApplication::InitializeDpiScaling();

    ScriptCanvas::ScriptCanvasApplication app(&argc, &argv);
    if (app.LaunchLocalServer())
    {
        app.Start({}, {});
        app.RunMainLoop();
        app.Stop();
    }

    return 0;
}
