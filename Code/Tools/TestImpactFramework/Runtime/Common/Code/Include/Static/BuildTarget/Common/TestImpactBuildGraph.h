/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTargetException.h>
#include <BuildTarget/Common/TestImpactBuildTargetList.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    template<typename ProductionTarget, typename TestTarget>
    struct BuildGraphVertex;
    
    //! List of vertices for dependencies or dependers.
    template<typename ProductionTarget, typename TestTarget>
    using BuildTargetDependencyList = AZStd::unordered_set<const BuildGraphVertex<ProductionTarget, TestTarget>*>;
    
    //! Build and runtime dependencies or dependers for a given vertex.
    template<typename ProductionTarget, typename TestTarget>
    struct BuildTargetDependencies
    {
        BuildTargetDependencyList<ProductionTarget, TestTarget> m_build; //!<
        BuildTargetDependencyList<ProductionTarget, TestTarget> m_runtime; //!<
    };
    
    //! Vertex in the build graph
    template<typename ProductionTarget, typename TestTarget>
    struct BuildGraphVertex
    {
        BuildGraphVertex(BuildTarget<ProductionTarget, TestTarget> buildTarget)
            : m_buildTarget(buildTarget)
        {
        }

        BuildTarget<ProductionTarget, TestTarget> m_buildTarget; //!<
        BuildTargetDependencies<ProductionTarget, TestTarget> m_dependencies; //!<
        BuildTargetDependencies<ProductionTarget, TestTarget> m_dependers; //!<
    };
    
    //! Build graph of all build targets in the repository, including their depenency and depender graphs.
    template<typename ProductionTarget, typename TestTarget>
    class BuildGraph
    {
    public:
        BuildGraph(const BuildTargetList<ProductionTarget, TestTarget>& buildTargetList);
    
        //! Returns the vertex for the specified build target, else returns `nullptr`.
        const BuildGraphVertex<ProductionTarget, TestTarget>* GetVertex(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget) const;

        //! Returns the vertex for the specified build target, else throws `BuildTargetException`.
        const BuildGraphVertex<ProductionTarget, TestTarget>& GetVertexOrThrow(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget) const;
            
    private:
        //! Map of all graph vertices, identified by build target.
        AZStd::unordered_map<BuildTarget<ProductionTarget, TestTarget>, BuildGraphVertex<ProductionTarget, TestTarget>>
            m_buildGraphVertices;
    };
    
    template<typename ProductionTarget, typename TestTarget>
    BuildGraph<ProductionTarget, TestTarget>::BuildGraph(
        const BuildTargetList<ProductionTarget, TestTarget>& buildTargetList)
    {
        // Build dependency graphs
        for (const auto& buildTarget : buildTargetList.GetBuildTargets())
        {
            const auto addOrRetrieveVertex = [this](const BuildTarget<ProductionTarget, TestTarget>& buildTarget)
            {
                auto it = m_buildGraphVertices.find(buildTarget);
                if (it == m_buildGraphVertices.end())
                {
                    return &m_buildGraphVertices
                                .emplace(
                                    AZStd::piecewise_construct, AZStd::forward_as_tuple(buildTarget), AZStd::forward_as_tuple(buildTarget))
                                .first->second;
                }
                else
                {
                    return &it->second;
                }
            };

            const auto resolveDependencies = [&](const DependencyList& unresolvedDependencies,
                                                 BuildTargetDependencyList<ProductionTarget, TestTarget>& resolveDependencies)
            {
                for (const auto& buildDependency : unresolvedDependencies)
                {
                    const auto buildDependencyTarget = buildTargetList.GetBuildTarget(buildDependency);
                    if (!buildDependencyTarget.has_value())
                    {
                        AZ_Warning(
                            "BuildTargetDependencyGraph",
                            false,
                            "Couldn't find build dependency '%s' for build target '%s'",
                            buildDependency.c_str(),
                            buildTarget.GetTarget()->GetName().c_str());
                        continue;
                    }

                    const auto* buildDependencyVertex = addOrRetrieveVertex(buildDependencyTarget.value());
                    resolveDependencies.insert(buildDependencyVertex);
                }
            };

            auto& vertex = *addOrRetrieveVertex(buildTarget);
            resolveDependencies(buildTarget.GetTarget()->GetDependencies().m_build, vertex.m_dependencies.m_build);
            resolveDependencies(buildTarget.GetTarget()->GetDependencies().m_runtime, vertex.m_dependencies.m_runtime);
        }

        // Build depender graphs
        for (auto& [buildTarget, vertex] : m_buildGraphVertices)
        {
            for (auto* dependency : vertex.m_dependencies.m_build)
            {
                auto& dependencyVertex = m_buildGraphVertices.find(dependency->m_buildTarget)->second;
                dependencyVertex.m_dependers.m_build.insert(&vertex);
            }

            for (auto* dependency : vertex.m_dependencies.m_runtime)
            {
                auto& dependencyVertex = m_buildGraphVertices.find(dependency->m_buildTarget)->second;
                dependencyVertex.m_dependers.m_runtime.insert(&vertex);
            }
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    const BuildGraphVertex<ProductionTarget, TestTarget>* BuildGraph<ProductionTarget, TestTarget>::GetVertex(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget) const
    {
        if (const auto it = m_buildGraphVertices.find(buildTarget) != m_buildGraphVertices.end())
        {
            return &it->second;
        }

        return nullptr;
    }

    template<typename ProductionTarget, typename TestTarget>
    const BuildGraphVertex<ProductionTarget, TestTarget>& BuildGraph<ProductionTarget, TestTarget>::
        GetVertexOrThrow(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget) const
    {
        const auto vertex = GetVertex(buildTarget);
        TestImpact_Eval(vertex, BuildTargetException, AZStd::string::format().c_str("Couldn't find build target '%s'", buildTarget.GetTarget()->GetName().c_str()));
        return vertex;
    }
} // namespace TestImpact

namespace AZStd
{
    //! Hash function for BuildTarget types for use in maps and sets.
    template<typename ProductionTarget, typename TestTarget>
    struct hash<TestImpact::BuildGraphVertex<ProductionTarget, TestTarget>>
    {
        size_t operator()(const TestImpact::BuildGraphVertex<ProductionTarget, TestTarget>& vertex) const noexcept
        {
            return reinterpret_cast<size_t>(vertex.m_buildTarget.GetTarget());
        }
    };
} // namespace AZStd
