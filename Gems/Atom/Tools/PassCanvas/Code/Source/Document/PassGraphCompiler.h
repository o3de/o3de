/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <Document/PassGraphCompilerRequestBus.h>
#include <GraphModel/Model/Node.h>

namespace PassCanvas
{
    //! PassGraphCompiler traverses and transforms a source graph into passes and other applicable assets.
    class PassGraphCompiler : public PassGraphCompilerRequestBus::Handler
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
        bool CompileGraph() const override;
        void QueueCompileGraph() const override;
        bool IsCompileGraphQueued() const override;

    private:
        void CompileGraphStarted() const;
        void CompileGraphFailed() const;
        void CompileGraphCompleted() const;

        const AZ::Crc32 m_toolId = {};
        const AZ::Uuid m_documentId = {};
        mutable bool m_compileGraphQueued = {};
        mutable AZStd::vector<AZStd::string> m_generatedFiles;
    };
} // namespace PassCanvas
