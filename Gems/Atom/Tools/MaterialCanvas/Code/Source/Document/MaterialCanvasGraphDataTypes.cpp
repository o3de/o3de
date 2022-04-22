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
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Int32, azrtti_typeid<int32_t>(), AZStd::any(int32_t{}), "int32_t", "int32_t"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Int64, azrtti_typeid<int64_t>(), AZStd::any(int64_t{}), "int64_t", "int64_t"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Uint32, azrtti_typeid<uint32_t>(), AZStd::any(uint32_t{}), "uint32_t", "uint32_t"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Uint64, azrtti_typeid<uint64_t>(), AZStd::any(uint64_t{}), "uint64_t", "uint64_t"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Float32, azrtti_typeid<float>(), AZStd::any(float{}), "float", "float"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Float64, azrtti_typeid<double>(), AZStd::any(double{}), "double", "double"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Vector2, azrtti_typeid<AZ::Vector2>(), AZStd::any(AZ::Vector2{}), "Vector2", "AZ::Vector2"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Vector3, azrtti_typeid<AZ::Vector3>(), AZStd::any(AZ::Vector3{}), "Vector3", "AZ::Vector3"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Vector4, azrtti_typeid<AZ::Vector4>(), AZStd::any(AZ::Vector4{}), "Vector4", "AZ::Vector4"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::Color, azrtti_typeid<AZ::Color>(), AZStd::any(AZ::Color::CreateOne()), "Color", "AZ::Color"),
            AZStd::make_shared<GraphModel::DataType>(
                MaterialCanvasDataTypeEnum::String, azrtti_typeid<AZStd::string>(), AZStd::any(AZStd::string{}), "String", "AZStd::string")
        };
    }
} // namespace MaterialCanvas
