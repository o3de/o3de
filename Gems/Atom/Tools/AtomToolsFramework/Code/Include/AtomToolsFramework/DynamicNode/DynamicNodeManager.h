/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace AtomToolsFramework
{
    //! Manages a collection of dynamic node configuration objects.
    //! Provides functions for loading, registering, clearing, and sorting configurations as well as generating node palette items.
    class DynamicNodeManager
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNodeManager, AZ::SystemAllocator, 0);
        AZ_RTTI(Graph, "{0DE0A2FA-3296-4E11-AA7F-831FAFA4126F}");

        DynamicNodeManager() = default;
        virtual ~DynamicNodeManager() = default;

        //! Loads and registers all of the dynamic node configuration files matching a given extension
        //! @param extensions Set of extensions used to enumerate and identify dynamic mode configuration files
        void LoadMatchingConfigFiles(const AZStd::unordered_set<AZStd::string>& extensions);

        //! Register a node configuration with the manager.
        //! @param config Dynamic node configuration to be added.
        void Register(const DynamicNodeConfig& config);

        //! Remove all registered node configurations.
        void Clear();

        //! Sort all of the node configurations by category and title
        void Sort();

        //! Generate the node palette hierarchy from the dynamic node configurations
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteRootTreeItem(const AZ::Crc32& toolId) const;

    private:
        AZStd::vector<DynamicNodeConfig> m_dynamicNodeConfigs;
    };

} // namespace AtomToolsFramework
