/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Editor Crashpad Upload Handler - Mac

#include <AzCore/Debug/Trace.h>
#include <handler/handler_main.h>
#include <Uploader/CrashUploader.h>

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    Lumberyard::InstallCrashUploader(argc, argv);

    LOG(ERROR) << "Initializing posix crash uploader";
    return crashpad::HandlerMain(argc, argv, Lumberyard::CrashUploader::GetCrashUploader()->GetUserStreamSources());
}
