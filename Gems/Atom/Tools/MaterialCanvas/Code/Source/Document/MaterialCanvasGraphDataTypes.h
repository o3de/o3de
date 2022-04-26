/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphModel/Model/DataType.h>

namespace MaterialCanvas
{
    enum MaterialCanvasDataTypeEnum
    {
        Invalid = GraphModel::DataType::ENUM_INVALID,
        Int32,
        Uint32,
        Float32,
        Vector2,
        Vector3,
        Vector4,
        Color,
        String,
        Count
    };

    GraphModel::DataTypeList CreateAllDataTypes();
} // namespace MaterialCanvas
