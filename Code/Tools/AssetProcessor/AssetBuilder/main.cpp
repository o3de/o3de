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

#include <string>

// int main(int argc, char** argv)
int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    std::string testargs[] = { "D:\\o3de\\build\\unity\\bin\\profile\\AssetBuilder.exe", "-debug",
                               "D:\\o3de\\Gems\\AtomLyIntegration\\CommonFeatures\\Assets\\Objects\\Hermanubis\\Hermanubis_High.fbx" };

    char* testargv[] = { testargs[0].data(), testargs[1].data(), testargs[2].data() };
    char** testargv1 = testargv;
    int testargc = 3;

    AssetBuilderApplication app(&testargc, &testargv1);


    // AssetBuilderApplication app(&argc, &argv);
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
