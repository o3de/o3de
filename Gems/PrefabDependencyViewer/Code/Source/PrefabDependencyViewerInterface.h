/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <DirectedGraph.h>

 namespace PrefabDependencyViewer
{
    class PrefabDependencyViewerInterface
    {
    public:
        AZ_RTTI(PrefabDependencyViewerInterface, "{9F550542-7184-42BE-B33D-CB9BDF88A758}");

        virtual void DisplayTree(const Utils::DirectedGraph& graph) = 0;
    };
} // namespace AzToolsFramework

