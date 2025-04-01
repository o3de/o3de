/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <Document/PassGraphCompiler.h>

namespace PassCanvas
{
    void PassGraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PassGraphCompiler, AtomToolsFramework::GraphCompiler>()
                ->Version(0)
                ;
        }
    }

    PassGraphCompiler::PassGraphCompiler(const AZ::Crc32& toolId)
        : AtomToolsFramework::GraphCompiler(toolId)
    {
    }

    PassGraphCompiler::~PassGraphCompiler()
    {
    }

    AZStd::string PassGraphCompiler::GetGraphPath() const
    {
        if (const auto& graphPath = AtomToolsFramework::GraphCompiler::GetGraphPath(); graphPath.ends_with(".passgraph"))
        {
            return graphPath;
        }

        return AZStd::string::format("%s/Assets/Passes/Generated/untitled.passgraph", AZ::Utils::GetProjectPath().c_str());
    }

    bool PassGraphCompiler::CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath)
    {
        if (!AtomToolsFramework::GraphCompiler::CompileGraph(graph, graphName, graphPath))
        {
            return false;
        }

        SetState(State::Complete);
        return true;
    }
} // namespace PassCanvas
