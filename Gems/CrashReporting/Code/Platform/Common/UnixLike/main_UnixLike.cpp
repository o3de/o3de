/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Game Gem Crashpad Upload Handler - Posix

#include <AzCore/Debug/Trace.h>
#include <handler/handler_main.h>
#include <Uploader/CrashUploader.h>

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    LOG(ERROR) << "Initializing non-windows crash uploader logging";
    return crashpad::HandlerMain(argc, argv, Lumberyard::CrashUploader::GetCrashUploader()->GetUserStreamSources());
}
