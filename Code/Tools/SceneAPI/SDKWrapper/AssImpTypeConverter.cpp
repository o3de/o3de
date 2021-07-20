/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        SceneAPI::DataTypes::MatrixType AssImpTypeConverter::ToTransform(const aiMatrix4x4& matrix)
        {
            SceneAPI::DataTypes::MatrixType transform;
            transform.SetElement(0, 0, static_cast<float>(matrix.a1));
            transform.SetElement(0, 1, static_cast<float>(matrix.a2));
            transform.SetElement(0, 2, static_cast<float>(matrix.a3));
            transform.SetElement(0, 3, static_cast<float>(matrix.a4));

            transform.SetElement(1, 0, static_cast<float>(matrix.b1));
            transform.SetElement(1, 1, static_cast<float>(matrix.b2));
            transform.SetElement(1, 2, static_cast<float>(matrix.b3));
            transform.SetElement(1, 3, static_cast<float>(matrix.b4));

            transform.SetElement(2, 0, static_cast<float>(matrix.c1));
            transform.SetElement(2, 1, static_cast<float>(matrix.c2));
            transform.SetElement(2, 2, static_cast<float>(matrix.c3));
            transform.SetElement(2, 3, static_cast<float>(matrix.c4));
            
            return transform;
        }

        SceneAPI::DataTypes::MatrixType AssImpTypeConverter::ToTransform(const AZ::Matrix4x4& matrix)
        {
            SceneAPI::DataTypes::MatrixType transform;
            for (int row = 0; row < 3; ++row)
            {
                for (int column = 0; column < 4; ++column)
                {
                    transform.SetElement(row, column, matrix.GetElement(row, column));
                }
            }

            return transform;
        }

        SceneAPI::DataTypes::Color AssImpTypeConverter::ToColor(const aiColor4D& color)
        {
            return AZ::SceneAPI::DataTypes::Color(color.r, color.g, color.b, color.a);
        }

        AZ::Vector3 AssImpTypeConverter::ToVector3(const aiVector3D& vector3)
        {
            return AZ::Vector3(vector3.x, vector3.y, vector3.z);
        }
    } //AssImpSDKWrapper
} // namespace AZ
