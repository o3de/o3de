/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Launcher.h>
#include <../Common/UnixLike/Launcher_UnixLike.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Math/Vector2.h>

#include <execinfo.h>
#include <libgen.h>
#include <netdb.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>

namespace 
{
    void SignalHandler(int sig, siginfo_t* info, void* secret)
    {
        FILE* ftrace = fopen("backtrace.log", "w");
        if (!ftrace)
        {
            ftrace = stderr;
        }

        AZ::Debug::StackFrame frames[25];
        unsigned int frameCount = AZ_ARRAY_SIZE(frames);
        frameCount = AZ::Debug::StackRecorder::Record(frames, frameCount);
        AZ::Debug::SymbolStorage::StackLine lines[25];
        AZ::Debug::SymbolStorage::DecodeFrames(frames, frameCount, lines);

        for (unsigned int frame = 0; frame < frameCount; ++frame)
        {
            fprintf(ftrace, "%s", lines[frame]);
        }

        if (ftrace != stderr)
        {
            fclose(ftrace);
        }

        abort();
    }

    void InitStackTracer()
    {
        struct sigaction sa;
        sa.sa_sigaction = SignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigaction(SIGSEGV, &sa, 0);
        sigaction(SIGBUS, &sa, 0);
        sigaction(SIGILL, &sa, 0);
        prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
    }
}

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    bool waitForDebugger = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-wait"))
        {
            waitForDebugger = true;
            break;
        }
    }
    if (waitForDebugger)
    {
        while(!AZ::Debug::Trace::Instance().IsDebuggerPresent())
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
        }
    }

    InitStackTracer();

    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;

    mainInfo.m_updateResourceLimits = IncreaseResourceLimits;

    bool ret = mainInfo.CopyCommandLine(argc, argv);

    // run the Lumberyard application
    ReturnCode status = ret ? 
        Run(mainInfo) : 
        ReturnCode::ErrCommandLine;

    return static_cast<int>(status);
}

void CVar_OnViewportPosition([[maybe_unused]] const AZ::Vector2& value) {}
