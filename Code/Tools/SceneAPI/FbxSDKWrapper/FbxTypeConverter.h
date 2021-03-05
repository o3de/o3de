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

#include <fbxsdk.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxTypeConverter
        {
        public:
            static Vector2 ToVector2(const FbxVector2& vector);
            static Vector3 ToVector3(const FbxVector4& vector);
            static SceneAPI::DataTypes::MatrixType ToTransform(const FbxAMatrix& matrix);
            static SceneAPI::DataTypes::MatrixType ToTransform(const FbxMatrix& matrix);
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
