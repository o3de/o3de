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
#include <AzCore/std/string/string.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace AtomToolsFramework
{
    //! Interface dynamic node manager, loading and registering dynamic node configurations and creating notes from them
    class DynamicNodeManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        virtual ~DynamicNodeManagerRequests() = default;

        //! Loads and registers all of the dynamic node configuration files matching given extensions
        //! @param extensions Set of extensions used to enumerate and identify dynamic mode configuration files
        virtual void LoadConfigFiles(const AZStd::unordered_set<AZStd::string>& extensions) = 0;

        //! Register a node configuration with the manager.
        //! @param configId Path or other unique identifier used to register a configuration
        //! @param config Dynamic node configuration to be added.
        virtual void RegisterConfig(const AZStd::string& configId, const DynamicNodeConfig& config) = 0;

        //! Get a node configuration with a specified ID.
        //! @param configId Path or other unique identifier used to register a configuration
        //! @returns Dynamic node configuration matching the ID or a default.
        virtual DynamicNodeConfig GetConfig(const AZStd::string& configId) const = 0;

        //! Remove all registered node configurations.
        virtual void Clear() = 0;

        //! Generate the node palette hierarchy from the dynamic node configurations
        virtual GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteTree() const = 0;
    };

    using DynamicNodeManagerRequestBus = AZ::EBus<DynamicNodeManagerRequests>;
} // namespace AtomToolsFramework
