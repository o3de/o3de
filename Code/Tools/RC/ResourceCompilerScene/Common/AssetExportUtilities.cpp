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

#include <RC/ResourceCompilerScene/Common/AssetExportUtilities.h>

namespace AZ
{
    namespace RC
    {
        Matrix34 AssetExportUtilities::ConvertToCryMatrix34(const SceneAPI::DataTypes::MatrixType& transform)
        {
            Matrix34 matrix;

            matrix.m00 = transform.GetColumn(0).GetX();
            matrix.m10 = transform.GetColumn(0).GetY();
            matrix.m20 = transform.GetColumn(0).GetZ();

            matrix.m01 = transform.GetColumn(1).GetX();
            matrix.m11 = transform.GetColumn(1).GetY();
            matrix.m21 = transform.GetColumn(1).GetZ();

            matrix.m02 = transform.GetColumn(2).GetX();
            matrix.m12 = transform.GetColumn(2).GetY();
            matrix.m22 = transform.GetColumn(2).GetZ();

            matrix.m03 = transform.GetColumn(3).GetX();
            matrix.m13 = transform.GetColumn(3).GetY();
            matrix.m23 = transform.GetColumn(3).GetZ();

            return matrix;
        }

        float AssetExportUtilities::CryQuatDotProd(const CryQuat& q, const CryQuat& p)
        {
            return q.w*p.w + q.v*p.v;
        }
    }
}
