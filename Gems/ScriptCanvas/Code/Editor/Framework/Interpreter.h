/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/Executor.h>

// will need a source handle version, with builder overrides
// use the data system, use a different class for that, actually
// the Interpreter

// \todo consider making a template version with return values, similar to execution out
// or perhaps safety checked versions with an array / table of any. something parsable
// or consider just having users make ebuses that the graphs will handle
// and wrapping the whole thing in a single class
// interpreter + ebus, and calling it EZ SC Hook or something like that

// the EZ Code driven thing, when it uses the click button, opens up a graph
// and drops in the main function WItH the typed arguments and return values stubbed out
// and makes those the required function of the graph!
// this code include using an ebus, for easiy switching to C++ extension

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    class Interpreter final
    {
    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute();

        bool IsExecutable() const;

        void ResetRuntimeUserData();

        void SetRuntimeUserData(AZStd::any&& runtimeUserData);

        void SetScript(SourceHandle source);

        void Stop();

    private:
        // this might have to negotiate between the AP and system files
        //, let the data system due that, and communicate back to this


        AZStd::any m_runtimeUserData;
        ScriptCanvas::RuntimeDataOverrides m_runtimeDataOverrides;
        Configuration m_configuration;
        // all calls to the executor must be safety checked!
        ScriptCanvas::Executor m_executor;

        void OnAssetAvailable();
    };
}
