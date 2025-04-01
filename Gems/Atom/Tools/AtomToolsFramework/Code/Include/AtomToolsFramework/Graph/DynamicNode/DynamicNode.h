/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeConfig.h>
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Node.h>

namespace AtomToolsFramework
{
    //! Graph Model node that generates its appearance and slots based on an external data driven configuration. The node looks up the
    //! config via DynamicNodeManagerRequestBus, using a tool ID and a config ID. Serializing these IDs instead of the config object saves
    //! considerable space in the serialized graph.
    class DynamicNode final : public GraphModel::Node
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNode, AZ::SystemAllocator);
        AZ_RTTI(DynamicNode, "{A618F01A-BCD8-4BDD-9832-6AB4DFE75E79}", GraphModel::Node);
        static void Reflect(AZ::ReflectContext* context);

        DynamicNode() = default;
        DynamicNode(GraphModel::GraphPtr ownerGraph, const AZ::Crc32& toolId, const AZ::Uuid& configId);

        const char* GetTitle() const override;
        const char* GetSubTitle() const override;

        // Get the ID of the dynamic node config used to create this node
        const AZ::Uuid& GetConfigId() const;

        // Get the dynamic node config used to create this node. This will be necessary to look up any application or context specific data
        // contained in the config.
        const DynamicNodeConfig& GetConfig() const;

        // Get the name of the title palette for node UI
        AZStd::string GetTitlePaletteName() const;

    protected:
        void RegisterSlots() override;

        AZ::Crc32 m_toolId = {};
        AZ::Uuid m_configId = AZ::Uuid::CreateNull();
        DynamicNodeConfig m_config;
    };
} // namespace AtomToolsFramework
