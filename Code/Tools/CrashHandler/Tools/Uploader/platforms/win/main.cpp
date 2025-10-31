/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Editor Crashpad Upload Handler Extension

#include <AzCore/Debug/Trace.h>
#include <AzCore/PlatformIncl.h>
#include <handler/handler_main.h>
#include <tools/tool_support.h>
#include <Uploader/CrashUploader.h>

namespace
{
    int HandlerMain(int argc, char* argv[])
    {
        O3de::InstallCrashUploader(argc, argv);

        LOG(ERROR) << "Initializing windows crash uploader";
        int resultCode = crashpad::HandlerMain(argc, argv, O3de::CrashUploader::GetCrashUploader()->GetUserStreamSources());
        
        return resultCode;
    }

}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, [[maybe_unused]] wchar_t* lpCmdLine, int)
{
    const AZ::Debug::Trace tracer;
    return crashpad::ToolSupport::Wmain(__argc, __wargv, HandlerMain);
}
