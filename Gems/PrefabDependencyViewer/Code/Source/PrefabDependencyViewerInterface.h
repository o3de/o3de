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

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <Utils.h>

 namespace PrefabDependencyViewer
{
    class PrefabDependencyViewerInterface
    {
    public:
        AZ_RTTI(PrefabDependencyViewerInterface, "{9F550542-7184-42BE-B33D-CB9BDF88A758}");

        virtual void DisplayTree(const Utils::DirectedGraph& graph) = 0;
    };
} // namespace AzToolsFramework

