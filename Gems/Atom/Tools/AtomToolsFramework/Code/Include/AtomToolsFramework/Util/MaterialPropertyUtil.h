/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/any.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

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

    //! Traverse up the instance data node hierarchy to find the containing dynamic property object 
    const AtomToolsFramework::DynamicProperty* FindDynamicPropertyForInstanceDataNode(const AzToolsFramework::InstanceDataNode* pNode);
} // namespace AtomToolsFramework
