/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManagerRequestBus.h>
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
        AZ_CLASS_ALLOCATOR(DynamicNodeManager, AZ::SystemAllocator);
        AZ_RTTI(DynamicNodeManager, "{D5330BF2-945F-4C8B-A5CF-68145EE6CBED}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeManager() = default;
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
        GraphModel::NodePtr CreateNodeById(GraphModel::GraphPtr graph, const AZ::Uuid& configId) override;
        GraphModel::NodePtr CreateNodeByName(GraphModel::GraphPtr graph, const AZStd::string& name) override;
        void RegisterEditDataForSetting(const AZStd::string& settingName, const AZ::Edit::ElementData& editData) override;
        AZStd::vector<AZStd::string> GetRegisteredEditDataSettingNames() const override;
        const AZ::Edit::ElementData* GetEditDataForSetting(const AZStd::string& settingName) const override;

    private:
        AZ_DISABLE_COPY_MOVE(DynamicNodeManager);

        bool ValidateSlotConfig(const AZ::Uuid& configId, const DynamicNodeSlotConfig& slotConfig) const;
        bool ValidateSlotConfigVec(const AZ::Uuid& configId, const AZStd::vector<DynamicNodeSlotConfig>& slotConfigVec) const;
        bool IsNodeConfigLoggingEnabled() const;

        const AZ::Crc32 m_toolId = {};
        GraphModel::DataTypeList m_registeredDataTypes;
        AZStd::unordered_map<AZ::Uuid, DynamicNodeConfig> m_nodeConfigMap;
        AZStd::unordered_map<AZStd::string, AZ::Edit::ElementData> m_editDataForSettingName;
    };
} // namespace AtomToolsFramework
