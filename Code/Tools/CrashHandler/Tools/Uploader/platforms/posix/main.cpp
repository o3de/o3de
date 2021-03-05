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

// LY Editor Crashpad Upload Handler - Mac

#include <handler/handler_main.h>
#include <Uploader/CrashUploader.h>

int main(int argc, char** argv)
{
    Lumberyard::InstallCrashUploader(argc, argv);

    LOG(ERROR) << "Initializing posix crash uploader";
    return crashpad::HandlerMain(argc, argv, Lumberyard::CrashUploader::GetCrashUploader()->GetUserStreamSources());
}
