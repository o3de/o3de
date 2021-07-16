/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
