/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasNodeRegistry.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    static AZ::EnvironmentVariable<NodeRegistry> g_nodeRegistry;

    NodeRegistry* NodeRegistry::GetInstance()
    {
        // Look up variable in AZ::Environment first
        // This is need if the Environment variable was already created in a different module memory space
        if (!g_nodeRegistry)
        {
            g_nodeRegistry = AZ::Environment::FindVariable<NodeRegistry>(s_nodeRegistryName);
        }

        // Create the environment variable in this memory space if it has not been found in an attached environment
        if (!g_nodeRegistry)
        {
            g_nodeRegistry = AZ::Environment::CreateVariable<NodeRegistry>(s_nodeRegistryName);
        }

        return &(g_nodeRegistry.Get());
    }

    void NodeRegistry::ResetInstance()
    {
        g_nodeRegistry.Reset();
    }
} // namespace ScriptCanvas
