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

int main(int argc, char** argv)
{
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
