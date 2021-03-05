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

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS BoneData
                : public AZ::SceneAPI::DataTypes::IBoneData
            {
            public:
                AZ_RTTI(BoneData, "{EDFB7CDB-DA39-41F1-800D-1E10421849E5}", AZ::SceneAPI::DataTypes::IBoneData);

                SCENE_DATA_API void SetWorldTransform(const SceneAPI::DataTypes::MatrixType& transform);
                SCENE_DATA_API const SceneAPI::DataTypes::MatrixType& GetWorldTransform() const override;

                static void Reflect(ReflectContext* context);

            protected:
                SceneAPI::DataTypes::MatrixType m_worldTransform;
            };
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ
