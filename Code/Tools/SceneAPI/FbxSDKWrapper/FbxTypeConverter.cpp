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

#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        Vector2 FbxTypeConverter::ToVector2(const FbxVector2& vector)
        {
            return Vector2(static_cast<float>(vector[0]),
                static_cast<float>(vector[1]));
        }

        Vector3 FbxTypeConverter::ToVector3(const FbxVector4& vector)
        {
            // Note: FbxVector4[x] is of type FbxDouble and aznumeric_caster does not accept it as cast from type.
            return Vector3(static_cast<float>(vector[0]),
                static_cast<float>(vector[1]),
                static_cast<float>(vector[2]));
        }

        SceneAPI::DataTypes::MatrixType FbxTypeConverter::ToTransform(const FbxAMatrix& matrix)
        {
            SceneAPI::DataTypes::MatrixType transform;
            for (int row = 0; row < 3; ++row)
            {
                FbxVector4 line = matrix.GetColumn(row);
                for (int column = 0; column < 4; ++column)
                {
                    // Note: FbxVector4[x] is of type FbxDouble and aznumeric_caster does not accept it as cast from type.
                    transform.SetElement(row, column, static_cast<float>(line[column]));
                }
            }
            return transform;
        }

        SceneAPI::DataTypes::MatrixType FbxTypeConverter::ToTransform(const FbxMatrix& matrix)
        {
            // This implementation ignores the last row, effectively assuming it to be (0,0,0,1)
            SceneAPI::DataTypes::MatrixType transform;
            for (int row = 0; row < 3; ++row)
            {
                FbxVector4 line = matrix.GetColumn(row);
                for (int column = 0; column < 4; ++column)
                {
                    // Note: FbxVector4[x] is of type FbxDouble and aznumeric_caster does not accept it as cast from type.
                    transform.SetElement(row, column, static_cast<float>(line[column]));
                }
            }
            return transform;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
