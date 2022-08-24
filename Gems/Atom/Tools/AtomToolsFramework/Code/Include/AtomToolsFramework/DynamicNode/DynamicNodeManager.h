/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AtomToolsFramework
{
    //! Manages all of the DynamicNodeConfig for a tool, providing functions for loading, registering, retrieving DynamicNodeConfig, as well
    //! as generating a node palette tree to create DynamicNode from DynamicNodeConfig.
    class DynamicNodeManager : public DynamicNodeManagerRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNodeManager, AZ::SystemAllocator, 0);
        AZ_RTTI(Graph, "{0DE0A2FA-3296-4E11-AA7F-831FAFA4126F}");
        AZ_DISABLE_COPY_MOVE(DynamicNodeManager);

        DynamicNodeManager(const AZ::Crc32& toolId);
        virtual ~DynamicNodeManager();

        //! DynamicNodeManagerRequestBus::Handler overrides...
        void RegisterDataTypes(const GraphModel::DataTypeList& dataTypes) override;
        GraphModel::DataTypeList GetRegisteredDataTypes() override;
        void LoadConfigFiles(const AZStd::string& extension) override;
        bool RegisterConfig(const DynamicNodeConfig& config) override;
        DynamicNodeConfig GetConfigById(const AZ::Uuid& configId) const override;
        void Clear() override;
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteTree() const override;

    private:
        bool ValidateSlotConfig(const AZ::Uuid& configId, const DynamicNodeSlotConfig& slotConfig) const;
        bool ValidateSlotConfigVec(const AZ::Uuid& configId, const AZStd::vector<DynamicNodeSlotConfig>& slotConfigVec) const;

        const AZ::Crc32 m_toolId = {};
        GraphModel::DataTypeList m_registeredDataTypes;
        AZStd::unordered_map<AZ::Uuid, DynamicNodeConfig> m_nodeConfigMap;
    };
} // namespace AtomToolsFramework
