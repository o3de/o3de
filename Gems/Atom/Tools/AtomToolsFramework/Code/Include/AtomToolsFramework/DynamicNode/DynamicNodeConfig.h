/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>
#include <AzCore/Serialization/EditContext.h>

namespace AtomToolsFramework
{
    //! Structure used to data drive appearance and other settings for dynamic graph model nodes.
    struct DynamicNodeConfig final
    {
        AZ_CLASS_ALLOCATOR(DynamicNodeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNodeConfig, "{D43A2D1A-B67F-4144-99AF-72EA606CA026}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeConfig(
            const AZStd::string& category,
            const AZStd::string& title,
            const AZStd::string& subTitle,
            const DynamicNodeSettingsMap& settings,
            const AZStd::vector<DynamicNodeSlotConfig>& inputSlots,
            const AZStd::vector<DynamicNodeSlotConfig>& outputSlots,
            const AZStd::vector<DynamicNodeSlotConfig>& propertySlots);
        DynamicNodeConfig() = default;
        ~DynamicNodeConfig() = default;

        //! Save all the configuration settings to a JSON file at the specified path
        //! @param path Absolute or aliased path where the configuration will be saved
        //! @returns True if the operation succeeded, otherwise false
        bool Save(const AZStd::string& path) const;

        //! Load all of the configuration settings from JSON file at the specified path
        //! @param path Absolute or aliased path from where the configuration will be loaded
        //! @returns True if the operation succeeded, otherwise false
        bool Load(const AZStd::string& path);

        void ValidateSlots();

        AZStd::vector<AZStd::string> GetSlotNames() const;

        //! Globally unique identifier for referencing this node config inside of DynamicNodeManager and graphs
        AZ::Uuid m_id = AZ::Uuid::CreateRandom();
        //! The category will be used by the DynamicNodeManager to sort and group node palette tree items
        AZStd::string m_category;
        //! Title will be displayed at the top of every DynamicNode in the graph view 
        AZStd::string m_title = "untitled";
        //! Subtitle will be displayed below the main title of every DynamicNode 
        AZStd::string m_subTitle;
        //! Name of the node title bar UI palette style sheet entry
        AZStd::string m_titlePaletteName;
        //! Vector of delimited strings, each representing a group of slot names that should share the same type
        AZStd::vector<AZStd::string> m_slotDataTypeGroups;
        //! Settings is a container of key value string pairs that can be used for any custom or application specific data
        DynamicNodeSettingsMap m_settings;
        //! Property slots is a container of DynamicNodeSlotConfig for property widgets that appear directly on the node
        AZStd::vector<DynamicNodeSlotConfig> m_propertySlots;
        //! Input slots is a container of DynamicNodeSlotConfig for all inputs into a node 
        AZStd::vector<DynamicNodeSlotConfig> m_inputSlots;
        //! Output slots is a container of DynamicNodeSlotConfig for all outputs from a node 
        AZStd::vector<DynamicNodeSlotConfig> m_outputSlots;

    private:
        static const AZ::Edit::ElementData* GetDynamicEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
    };
} // namespace AtomToolsFramework
