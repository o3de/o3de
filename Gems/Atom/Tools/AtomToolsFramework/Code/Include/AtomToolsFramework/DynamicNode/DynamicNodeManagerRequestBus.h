/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/string.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphModel/Model/DataType.h>
#include <GraphModel/Model/Node.h>

namespace AtomToolsFramework
{
    //! Interface for dynamic node manager interactions
    class DynamicNodeManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        virtual ~DynamicNodeManagerRequests() = default;

        //! Register data types needed by the dynamic node manager and graph contexts
        //! @param dataTypes Container of data type pointers
        virtual void RegisterDataTypes(const GraphModel::DataTypeList& dataTypes) = 0;

        //! Get a container of all data types registered with the dynamic node manager
        virtual GraphModel::DataTypeList GetRegisteredDataTypes() = 0;

        //! Loads and registers all of the DynamicNodeConfig files matching given extensions
        //! @param extension Extension to enumerate DynamicNodeConfig files
        virtual void LoadConfigFiles(const AZStd::string& extension) = 0;

        //! Register a DynamicNodeConfig with the manager.
        //! @param config DynamicNodeConfig to be added.
        virtual bool RegisterConfig(const DynamicNodeConfig& config) = 0;

        //! Get a DynamicNodeConfig with a specified ID.
        //! @param configId Unique identifier used to register a DynamicNodeConfig
        //! @returns DynamicNodeConfig matching the ID or a default.
        virtual DynamicNodeConfig GetConfigById(const AZ::Uuid& configId) const = 0;

        //! Remove all registered DynamicNodeConfig.
        virtual void Clear() = 0;

        //! Generate the node palette tree from registered DynamicNodeConfig
        virtual GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteTree() const = 0;

        //! Create a dynamic node from the configuration matching the specified id.
        virtual GraphModel::NodePtr CreateNodeById(GraphModel::GraphPtr graph, const AZ::Uuid& configId) = 0;

        //! Create a dynamic node from the configuration matching the specified name.
        virtual GraphModel::NodePtr CreateNodeByName(GraphModel::GraphPtr graph, const AZStd::string& name) = 0;

        //! Register dynamic edit data for dynamic node settings so that the edit context handler and attribute can be overridden for a
        //! particular settings group.
        virtual void RegisterEditDataForSetting(const AZStd::string& settingName, const AZ::Edit::ElementData& editData) = 0;

        //! Get the pointer value of the dynamic edit data registered for a dynamic node configuration setting. Edit data pointer must
        //! remain valid for as long as configurations can be edited.
        virtual const AZ::Edit::ElementData* GetEditDataForSetting(const AZStd::string& settingName) const = 0;
    };

    using DynamicNodeManagerRequestBus = AZ::EBus<DynamicNodeManagerRequests>;
} // namespace AtomToolsFramework
