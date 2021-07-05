/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// Landsca Canvas
#include <Editor/Core/DataTypes.h>

namespace LandscapeCanvas
{
    void LandscapeCanvasDataType::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<LandscapeCanvasDataType, GraphModel::DataType>()
                ->Version(0)
                ;
        }
    }

    LandscapeCanvasDataType::LandscapeCanvasDataType(
        Enum typeEnum,
        AZ::Uuid typeUuid,
        AZStd::any defaultValue,
        AZStd::string_view typeDisplayName,
        AZStd::string_view cppTypeName)
        : DataType(typeEnum, typeUuid, defaultValue, typeDisplayName, cppTypeName)
    {
    }
}
