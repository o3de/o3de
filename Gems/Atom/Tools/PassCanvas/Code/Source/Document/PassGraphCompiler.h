/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/GraphCompiler.h>

namespace PassCanvas
{
    //! PassGraphCompiler traverses a pass graph, searching for and splicing shader code snippets, variable values and definitions,
    //! and other information into complete, functional pass types, passes, and shaders. Currently, the resulting files will be
    //! generated an output into the same folder location has the source graph.
    class PassGraphCompiler : public AtomToolsFramework::GraphCompiler
    {
    public:
        AZ_RTTI(PassGraphCompiler, "{4D9407B1-195A-404A-B97A-E2BA22207C87}", AtomToolsFramework::GraphCompiler);
        AZ_CLASS_ALLOCATOR(PassGraphCompiler, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(PassGraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        PassGraphCompiler() = default;
        PassGraphCompiler(const AZ::Crc32& toolId);
        virtual ~PassGraphCompiler();

        // AtomToolsFramework::GraphCompiler overrides...
        AZStd::string GetGraphPath() const;
        bool CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath) override;
    };
} // namespace PassCanvas
