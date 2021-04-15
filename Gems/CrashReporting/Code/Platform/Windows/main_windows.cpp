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

// LY Game Gem crash uploader extension - Windows

#include <AzCore/PlatformIncl.h>

#include <handler/handler_main.h>
#include <tools/tool_support.h>
#include <Uploader/CrashUploader.h>

namespace
{
    int HandlerMain(int argc, char* argv[])
    {
        O3de::InstallCrashUploader(argc, argv);

        LOG(ERROR) << "Initializing windows game crash uploader logging";
        int resultCode = crashpad::HandlerMain(argc, argv, O3de::CrashUploader::GetCrashUploader()->GetUserStreamSources());

        return resultCode;
    }

}
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, [[maybe_unused]] wchar_t* lpCmdLine, int)
{
    return crashpad::ToolSupport::Wmain(__argc, __wargv, HandlerMain);
}

