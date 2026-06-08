/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetBuilderApplication.h"
#include "TraceMessageHook.h"
#include "AssetBuilderComponent.h"
#include <AzCore/Settings/CommandLineParser_Platform.h>

// the user is not expected to interact with the AssetBuilderApplication directly,
// so it can be always running in the culture-invariant locale.
#include <locale.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
#include <unistd.h>
#include <thread>
#include <chrono>

namespace
{
    // Child-side parent-death watchdog. Polls getppid(); if the parent
    // process dies (we get reparented to init / systemd-user), exit
    // cleanly so we don't accumulate as an orphan across AP restarts.
    //
    // This replaces ProcessLauncher's m_tetherLifetime path
    // (prctl(PR_SET_PDEATHSIG, SIGTERM)) which is unsafe to use from
    // short-lived forking threads. The kernel binds PDEATHSIG to the
    // THREAD that called fork(), not the parent PROCESS; AssetProcessor's
    // BuilderManager forks builders from short-lived TaskWorker threads,
    // so the prctl approach gets every spawned builder SIGTERM'd within
    // milliseconds. Child-side polling is independent of the launching
    // thread's lifetime.
    //
    // 2s polling = negligible CPU, max orphan-lifetime of 2 seconds.
    void StartParentDeathWatchdog()
    {
        const pid_t parentAtStartup = getppid();
        if (parentAtStartup <= 1)
        {
            // Launched without a meaningful parent (already init's child);
            // no watchdog needed.
            return;
        }
        std::thread([parentAtStartup]()
        {
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                if (::getppid() != parentAtStartup)
                {
                    _exit(0);
                }
            }
        }).detach();
    }
}
#endif

int main(int argc, char** argv)
{
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
    StartParentDeathWatchdog();
#endif

    // globally set the application locale to the culture-invariant locale.
    // This should cause all reading and writing under all threads to use the invariant locale
    // So that the application can be run in any locale and still produce the same output.
    // We would not do this to a front-facing application that needs to actually be localized in a GUI,
    // but since this application runs headlessly and its job is to crunch invariant locale files into
    // other invariant locale files, setting it to the invariant locale means that individual builders
    // don't need to keep track of locale, change it, set it, etc.
    setlocale(LC_ALL, "C"); 

    // Make sure utf-8 commandline parameters can be used on all platforms
    AZ::Settings::Platform::CommandLineConverter converter(argc, argv);

    const AZ::Debug::Trace tracer;
    AssetBuilderApplication app(&argc, &argv);
    AssetBuilder::TraceMessageHook traceMessageHook; // Hook AZ Debug messages and redirect them to stdout
    traceMessageHook.EnableTraceContext(true);
    AZ::Debug::Trace::HandleExceptions(true);

    AZ::ComponentApplication::StartupParameters startupParams;
    startupParams.m_loadDynamicModules = false;

    app.Start(AzFramework::Application::Descriptor(), startupParams);
    traceMessageHook.EnableDebugMode(app.IsInDebugMode());

    bool result = false;

    BuilderBus::BroadcastResult(result, &BuilderBus::Events::Run);

    traceMessageHook.EnableTraceContext(false);
    app.Stop();
    
    return result ? 0 : 1;
}
