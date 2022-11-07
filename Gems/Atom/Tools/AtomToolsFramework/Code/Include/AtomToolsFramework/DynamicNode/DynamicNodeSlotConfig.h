/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <GraphModel/Model/DataType.h>

namespace AtomToolsFramework
{
    // Contains tables of strings representing application or context specific settings for each node
    using DynamicNodeSettingsMap = AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>>;

    //! Contains all of the settings for an individual input or output slot on a DynamicNode
    struct DynamicNodeSlotConfig final
    {
        AZ_CLASS_ALLOCATOR(DynamicNodeSlotConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNodeSlotConfig, "{F2C95A99-41FD-4077-B9A7-B0BF8F76C2CE}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeSlotConfig(
            const AZStd::string& name,
            const AZStd::string& displayName,
            const AZStd::string& description,
            const AZStd::any& defaultValue,
            const AZStd::string& supportedDataTypeRegex,
            const DynamicNodeSettingsMap& settings);
        DynamicNodeSlotConfig() = default;
        ~DynamicNodeSlotConfig() = default;

        //! The function validates that the default data type corresponds to one of the registered data types matching the supported
        //! data type regular expression. If it is empty or does not match one of the supported registered data types then the value is
        //! automatically set to the first register data type, if it cannot be inferred from the last assigned default value.
        //!
        //! Once the default data type has been resolved, the currently assigned default value will be reset if it does not match the new
        //! data type.
        //!
        //! @returns AZ::Edit::PropertyRefreshLevels::EntireTree if the data type of the default value changed, otherwise
        //! AZ::Edit::PropertyRefreshLevels::AttributesAndValues
        AZ::Crc32 ValidateDataTypes();

        //! @returns the custom default value stored in the configuration if it matches one of the supported data types. Otherwise, the
        //! default value from the default data type or the first registered data type will be returned.
        AZStd::any GetDefaultValue() const;

        //! @returns the display name of the data type returned from GetDefaultDataType.
        AZStd::string GetDefaultDataTypeName() const;

        //! @returns the first register data type matching the default data type name.
        GraphModel::DataTypePtr GetDefaultDataType() const;

        //! @returns a vector of names of all the data types returned from GetSupportedDataTypes, primarily used to feed options into the
        //! property editor for selection.
        AZStd::vector<AZStd::string> GetSupportedDataTypeNames() const;

        //! @returns a list of all registered graph model data types with names matching the supported data type regular expression.
        GraphModel::DataTypeList GetSupportedDataTypes() const;

        //! @returns the name of this object that will be displayed in the reflected property editor.
        AZStd::string GetDisplayNameForEditor() const;

        //! Unique name or ID of a slot
        AZStd::string m_name = "untitled";
        //! Name displayed next to a slot in the node UI
        AZStd::string m_displayName = "untitled";
        //! Longer description display for tooltips and other UI
        AZStd::string m_description;
        //! The default value associated with a slot
        AZStd::any m_defaultValue;
        //! Regular expression for identifying the names of data types this slot can hold and connect to
        AZStd::string m_supportedDataTypeRegex;
        //! Name of the default data type from the set of supported data types if no value is assigned
        AZStd::string m_defaultDataType;
        //! Container of generic or application specific settings for a slot
        DynamicNodeSettingsMap m_settings;
        //! Specifies whether or not the slot will appear on the node UI
        bool m_visibleOnNode = true;
        //! Specifies whether or not the slot value will be editable on the node UI
        bool m_editableOnNode = true;
        //! Hint on whether or not the slot name can be substituted or mangled in applicable systems
        bool m_allowNameSubstitution = true;

    private:
        static const AZ::Edit::ElementData* GetDynamicEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
    };
} // namespace AtomToolsFramework
