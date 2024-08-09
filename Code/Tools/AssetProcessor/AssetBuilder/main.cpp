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

// the user is not expected to interact with the AssetBuilderApplication directly,
// so it can be always running in the culture-invariant locale.
#include <locale.h>

int main(int argc, char** argv)
{
    // globally set the application locale to the culture-invariant locale.
    // This should cause all reading and writing under all threads to use the invariant locale
    // So that the application can be run in any locale and still produce the same output.
    // We would not do this to a front-facing application that needs to actually be localized in a GUI,
    // but since this application runs headlessly and its job is to crunch invariant locale files into
    // other invariant locale files, setting it to the invariant locale means that individual builders
    // don't need to keep track of locale, change it, set it, etc.
    setlocale(LC_ALL, "C"); 

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
