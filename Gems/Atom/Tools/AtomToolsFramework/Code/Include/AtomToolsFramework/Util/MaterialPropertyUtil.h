/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/std/any.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

namespace AtomToolsFramework
{
    AZ::RPI::MaterialPropertyValue ConvertToRuntimeType(const AZStd::any& value);
    AZStd::any ConvertToEditableType(const AZ::RPI::MaterialPropertyValue& value);
    AtomToolsFramework::DynamicPropertyType ConvertToEditableType(const AZ::RPI::MaterialPropertyDataType dataType);
    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialTypeSourceData::PropertyDefinition& propertyDefinition);
    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData);
    void ConvertToPropertyMetaData(AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData, const AtomToolsFramework::DynamicPropertyConfig& propertyConfig);
} // namespace AtomToolsFramework
