/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Document/MaterialCanvasGraphDataTypes.h>

namespace MaterialCanvas
{
    GraphModel::DataTypeList CreateAllDataTypes()
    {
        return {
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Int32, int32_t{}, "int32"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Uint32, uint32_t{}, "uint32"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Float32, float{}, "float"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Vector2, AZ::Vector2::CreateZero(), "Vector2"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Vector3, AZ::Vector3::CreateZero(), "Vector3"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Vector4, AZ::Vector4::CreateZero(), "Vector4"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::Color, AZ::Color::CreateOne(), "Color"),
            AZStd::make_shared<GraphModel::DataType>(MaterialCanvasDataTypeEnum::String, AZStd::string{}, "String"),
        };
    }
} // namespace MaterialCanvas
