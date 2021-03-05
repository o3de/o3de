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
