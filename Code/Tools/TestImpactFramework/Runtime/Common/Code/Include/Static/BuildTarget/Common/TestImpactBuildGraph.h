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

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/functional.h>

namespace TestImpact
{
    //! Result to return when visiting vertices in the build graph.
    enum class BuildGraphVertexVisitResult : AZ::u8
    {
        Continue, //!< Continue traversing the build graph.
        AbortBranchTraversal, //!< Abort traversal of this particular branch in the build graph.
        AbortGraphTraversal //!< Abort traversal of the build graph.
    };

    template<typename ProductionTarget, typename TestTarget>
    struct BuildGraphVertex;

    //! Visitor callback for when traversing the build graphs.
    //! @param vertex The current vertex to visit in the build graph.
    //! @param distance The distance of this vertex to the vertex of the build target whose build graph is being walked.
    //! @returns The visitor result to determine how the traversal should proceed.
    template<typename ProductionTarget, typename TestTarget>
    using BuildGraphVertexVisitor = AZStd::function<BuildGraphVertexVisitResult(const BuildGraphVertex<ProductionTarget, TestTarget>& vertex, size_t distance)>;
    
    //! Build graph target set for dependencies or dependers.
    template<typename ProductionTarget, typename TestTarget>
    using TargetBuildGraphSet = AZStd::unordered_set<const BuildGraphVertex<ProductionTarget, TestTarget>*>;
    
    //! Build graph for the dependencies and dependers of a given build target.
    template<typename ProductionTarget, typename TestTarget>
    struct TargetBuildGraph
    {
        TargetBuildGraphSet<ProductionTarget, TestTarget> m_build; //!< Build dependencies/dependers.
        TargetBuildGraphSet<ProductionTarget, TestTarget> m_runtime; //!< Runtime dependencies/dependers.
    };
    
    //! Vertex in the build graph containing the build target and its dependencies/dependers.
    template<typename ProductionTarget, typename TestTarget>
    struct BuildGraphVertex
    {
        BuildGraphVertex(BuildTarget<ProductionTarget, TestTarget> buildTarget)
            : m_buildTarget(buildTarget)
        {
        }

        BuildTarget<ProductionTarget, TestTarget> m_buildTarget; //!< The build target for this vertex.
        TargetBuildGraph<ProductionTarget, TestTarget> m_dependencies; //!< The dependencies of this build target.
        TargetBuildGraph<ProductionTarget, TestTarget> m_dependers; //!< The dependers of this build target.
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

        //! Walks the specified target's build dependencies.
        void WalkBuildDependencies(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
            const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const;

        //! Walks the specified target's build dependers.
        void WalkBuildDependers(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
            const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const;

        //! Walks the specified target's runtime dependencies.
        void WalkRuntimeDependencies(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
            const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const;

        //! Walks the specified target's runtime dependers.
        void WalkRuntimeDependers(
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
            const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const;

    private:
        //! Alias for a target dependency or depender graph.
        using TargetBuildGraphPointer = TargetBuildGraph<ProductionTarget, TestTarget> BuildGraphVertex<ProductionTarget, TestTarget>::*;
        
        //! Alias for build or runtime dependency or depender graph.
        using TargetBuildGraphSetPointer = TargetBuildGraphSet<ProductionTarget, TestTarget> TargetBuildGraph<ProductionTarget, TestTarget>::*;

        //! Generic function for walking the specified target's build graph.
        void WalkTargetBuildGraphSet(
            TargetBuildGraphPointer graph,
            TargetBuildGraphSetPointer set,
            const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
            const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const;

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
                if (auto it = m_buildGraphVertices.find(buildTarget);
                    it == m_buildGraphVertices.end())
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

            const auto resolveDependencies =
                [&](const DependencyList& unresolvedDependencies, TargetBuildGraphSet<ProductionTarget, TestTarget>& resolveDependencies)
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
        if (const auto it = m_buildGraphVertices.find(buildTarget);
            it != m_buildGraphVertices.end())
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
        AZ_TestImpact_Eval(
            vertex, 
            BuildTargetException, 
            AZStd::string::format("Couldn't find build target '%s'",
            buildTarget.GetTarget()->GetName().c_str()).c_str());

        return *vertex;
    }

    template<typename ProductionTarget, typename TestTarget>
    void BuildGraph<ProductionTarget, TestTarget>::WalkTargetBuildGraphSet(
        TargetBuildGraphPointer graph,
        TargetBuildGraphSetPointer set,
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
        const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const
    {
        AZStd::queue<const BuildGraphVertex<ProductionTarget, TestTarget>*> vertexQueue;
        AZStd::unordered_set<const BuildGraphVertex<ProductionTarget, TestTarget>*> visitedVertices;
        AZStd::unordered_map<const BuildGraphVertex<ProductionTarget, TestTarget>*, size_t> vertexDistances;

        // Skip visiting the root vertex and start visiting its children instead
        const auto& parentVertex = GetVertexOrThrow(buildTarget);
        visitedVertices.insert(&parentVertex);
        for (const auto* child : parentVertex.*graph.*set)
        {
            vertexQueue.push(child);
            vertexDistances[child] = 1;
        }

        while (!vertexQueue.empty())
        {
            const auto* vertex = vertexQueue.front();
            vertexQueue.pop();

            if (const auto result = visitor(*vertex, vertexDistances[vertex]);
                result == BuildGraphVertexVisitResult::AbortGraphTraversal)
            {
                return;
            }
            else if (result == BuildGraphVertexVisitResult::AbortBranchTraversal)
            {
                continue;
            }

            for (const auto* child : vertex->*graph.*set)
            {
                if (!visitedVertices.contains(child))
                {
                    visitedVertices.insert(child);
                    vertexQueue.push(child);
                    vertexDistances[child] = vertexDistances[vertex] + 1;
                }
            }
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    void BuildGraph<ProductionTarget, TestTarget>::WalkBuildDependencies(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
        const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const
    {
        WalkTargetBuildGraphSet(
            &BuildGraphVertex<ProductionTarget, TestTarget>::m_dependencies,
            &TargetBuildGraph<ProductionTarget, TestTarget>::m_build,
            buildTarget,
            visitor);
    }

    template<typename ProductionTarget, typename TestTarget>
    void BuildGraph<ProductionTarget, TestTarget>::WalkBuildDependers(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
        const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const
    {
        WalkTargetBuildGraphSet(
            &BuildGraphVertex<ProductionTarget, TestTarget>::m_dependers,
            &TargetBuildGraph<ProductionTarget, TestTarget>::m_build,
            buildTarget,
            visitor);
    }

    template<typename ProductionTarget, typename TestTarget>
    void BuildGraph<ProductionTarget, TestTarget>::WalkRuntimeDependencies(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
        const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const
    {
        WalkTargetBuildGraphSet(
            &BuildGraphVertex<ProductionTarget, TestTarget>::m_dependencies,
            &TargetBuildGraph<ProductionTarget, TestTarget>::m_runtime,
            buildTarget,
            visitor);
    }

    template<typename ProductionTarget, typename TestTarget>
    void BuildGraph<ProductionTarget, TestTarget>::WalkRuntimeDependers(
        const BuildTarget<ProductionTarget, TestTarget>& buildTarget,
        const BuildGraphVertexVisitor<ProductionTarget, TestTarget>& visitor) const
    {
        WalkTargetBuildGraphSet(
            &BuildGraphVertex<ProductionTarget, TestTarget>::m_dependers,
            &TargetBuildGraph<ProductionTarget, TestTarget>::m_runtime,
            buildTarget,
            visitor);
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
