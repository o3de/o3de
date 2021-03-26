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
#include "AssetBuilderApplication.h"
#include "TraceMessageHook.h"
#include "AssetBuilderComponent.h"

int main(int argc, char** argv)
{
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
