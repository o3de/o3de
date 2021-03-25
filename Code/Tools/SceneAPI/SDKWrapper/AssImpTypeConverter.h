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

#include <assimp/scene.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {

        namespace DataTypes
        {
            struct Color;
        } // namespace DataTypes
    } // namespace SceneAPI

    namespace AssImpSDKWrapper
    {
        class AssImpTypeConverter
        {
        public:
            static SceneAPI::DataTypes::MatrixType ToTransform(const aiMatrix4x4& matrix);
            static SceneAPI::DataTypes::MatrixType ToTransform(const AZ::Matrix4x4& matrix);
            static SceneAPI::DataTypes::Color ToColor(const aiColor4D& color);
            static AZ::Vector3 ToVector3(const aiVector3D& vector3);
        };
    } // namespace AssImpSDKWrapper
} // namespace AZ
