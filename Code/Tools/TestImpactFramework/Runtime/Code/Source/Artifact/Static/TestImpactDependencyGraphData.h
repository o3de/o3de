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

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Raw representation of the dependency graph for a given build target.
    struct DependencyGraphData
    {
        AZStd::string m_root; //!< The build target this dependency graph is for.
        AZStd::vector<AZStd::string> m_vertices; //!< The depender/depending built targets in this graph.
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> m_edges; //!< The dependency connectivity of the build targets in this graph.
    };
} // namespace TestImpact
