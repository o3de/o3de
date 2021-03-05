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

// AZ
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string_view.h>

// GraphModel
#include <GraphModel/Model/DataType.h>

namespace LandscapeCanvas
{
    enum LandscapeCanvasDataTypeEnum
    {
        InvalidEntity = 0,  // Need a special case data type for the AZ::EntityId::InvalidEntityId to handle the default value since we are re-using the AZ::EntityId type in several node data types
        Bounds,
        Gradient,
        Area,
        String,
        Count
    };

    static const AZ::Uuid InvalidEntityTypeId = "{4CD676F4-2C6E-43A4-8477-623640477D32}";
    static const AZ::Uuid BoundsTypeId = "{746398F1-0325-4A56-A544-FEF561C24E7C}";
    static const AZ::Uuid GradientTypeId = "{F38CF64A-1EB6-41FA-A2CC-73D19B48E59E}";
    static const AZ::Uuid AreaTypeId = "{FE1878D9-D445-4652-894B-D6348706EEAE}";

    class LandscapeCanvasDataType : public GraphModel::DataType
    {
    public:
        AZ_CLASS_ALLOCATOR(LandscapeCanvasDataType, AZ::SystemAllocator, 0);
        AZ_RTTI(LandscapeCanvasDataType, "{BD06082C-5CCA-4FB6-B881-6927B82A2142}", GraphModel::DataType);

        static void Reflect(AZ::ReflectContext* reflection);

        LandscapeCanvasDataType() = default;
        LandscapeCanvasDataType(Enum typeEnum, AZ::Uuid typeUuid, AZStd::any defaultValue, AZStd::string_view typeDisplayName, AZStd::string_view cppTypeName);
    };    
}
