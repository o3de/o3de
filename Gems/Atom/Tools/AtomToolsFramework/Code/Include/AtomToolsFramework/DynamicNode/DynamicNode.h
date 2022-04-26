/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Node.h>

namespace AtomToolsFramework
{
    //! Data driven graph model node
    class DynamicNode final : public GraphModel::Node
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNode, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNode, "{A618F01A-BCD8-4BDD-9832-6AB4DFE75E79}", GraphModel::Node);
        static void Reflect(AZ::ReflectContext* context);

        DynamicNode() = default;
        DynamicNode(GraphModel::GraphPtr ownerGraph, const DynamicNodeConfig& config);

        const char* GetTitle() const override;
        const char* GetSubTitle() const override;

        using Node::PostLoadSetup;
        void PostLoadSetup(GraphModel::GraphPtr ownerGraph, GraphModel::NodeId id) override;

    protected:
        void RegisterSlots() override;

        DynamicNodeConfig m_config;
    };
} // namespace AtomToolsFramework
