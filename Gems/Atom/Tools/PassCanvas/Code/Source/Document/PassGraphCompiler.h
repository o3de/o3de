/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <Document/PassGraphCompilerRequestBus.h>
#include <GraphModel/Model/Node.h>

namespace PassCanvas
{
    //! PassGraphCompiler traverses a pass graph, searching for and splicing shader code snippets, variable values and definitions,
    //! and other information into complete, functional pass types, passes, and shaders. Currently, the resulting files will be
    //! generated an output into the same folder location has the source graph.
    class PassGraphCompiler
        : public PassGraphCompilerRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_RTTI(PassGraphCompiler, "{462AD6DF-874C-4017-9372-370B86A0A24B}");
        AZ_CLASS_ALLOCATOR(PassGraphCompiler, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(PassGraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        PassGraphCompiler() = default;
        PassGraphCompiler(const AZ::Crc32& toolId, const AZ::Uuid& documentId);
        virtual ~PassGraphCompiler();

        // PassGraphCompilerRequestBus::Handler overrides...
        const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const override;
        bool CompileGraph() override;
        void QueueCompileGraph() override;
        bool IsCompileGraphQueued() const override;

    private:
        void CompileGraphStarted();
        void CompileGraphFailed();
        void CompileGraphCompleted();

        // Get the graph export path based on the Graph document path or default export path.
        AZStd::string GetGraphPath() const;

        // AZ::SystemTickBus::Handler overrides...
        void OnSystemTick() override;

        const AZ::Crc32 m_toolId = {};
        const AZ::Uuid m_documentId = {};
        bool m_compileGraph = {};
        bool m_compileAssets = {};
        int m_compileAssetIndex = {};
        AZStd::string m_lastAssetStatusMessage;
        AZStd::vector<AZStd::string> m_generatedFiles;
    };
} // namespace PassCanvas
