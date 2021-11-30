/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/any.h>

namespace AzToolsFramework
{
    class InstanceDataNode;
}

namespace AtomToolsFramework
{
    //! Convert an editor property stored in a AZStd::any into a material property value
    AZ::RPI::MaterialPropertyValue ConvertToRuntimeType(const AZStd::any& value);

    //! Convert a material property value into a AZStd::any that can be used as a dynamic property
    AZStd::any ConvertToEditableType(const AZ::RPI::MaterialPropertyValue& value);

    //! Convert from a material system property type enumeration into the corresponding dynamic property type 
    AtomToolsFramework::DynamicPropertyType ConvertToEditableType(const AZ::RPI::MaterialPropertyDataType dataType);

    //! Convert and assign material type source data property definition fields to editor dynamic property configuration 
    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialTypeSourceData::PropertyDefinition& propertyDefinition);

    //! Convert and assign material property meta data fields to editor dynamic property configuration 
    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData);

    //! Convert and assign editor dynamic property configuration fields to material property meta data
    void ConvertToPropertyMetaData(AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData, const AtomToolsFramework::DynamicPropertyConfig& propertyConfig);

    //! Compare equality of data types and values of editor property stored in AZStd::any
    bool ArePropertyValuesEqual(const AZStd::any& valueA, const AZStd::any& valueB);

    //! Convert the property value into the format that will be stored in the source data
    //! This is primarily needed to support conversions of special types like enums and images
    //! @param exportPath absolute path of the file being saved
    //! @param propertyDefinition describes type information and other details about propertyValue
    //! @param propertyValue the value being converted before saving
    bool ConvertToExportFormat(
        const AZStd::string& exportPath,
        [[maybe_unused]] const AZ::Name& propertyId,
        const AZ::RPI::MaterialTypeSourceData::PropertyDefinition& propertyDefinition,
        AZ::RPI::MaterialPropertyValue& propertyValue);

    //! Generate a file path from the exported file to the external reference.
    //! This function returns a relative path from the export file to the reference file.
    //! If the relative path is too different or distant from the export path then we return the asset folder relative path.
    //! @param exportPath absolute path of the file being saved
    //! @param referencePath absolute path of a file that will be treated as an external reference
    //! @param maxPathDepth the maximum relative depth or number of parent or child folders between the export path and the reference path
    AZStd::string GetExteralReferencePath(
        const AZStd::string& exportPath, const AZStd::string& referencePath, const uint32_t maxPathDepth = 2);

    //! Traverse up the instance data node hierarchy to find the containing dynamic property object 
    const AtomToolsFramework::DynamicProperty* FindDynamicPropertyForInstanceDataNode(const AzToolsFramework::InstanceDataNode* pNode);
} // namespace AtomToolsFramework
