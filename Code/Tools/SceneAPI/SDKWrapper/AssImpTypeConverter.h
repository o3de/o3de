/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
