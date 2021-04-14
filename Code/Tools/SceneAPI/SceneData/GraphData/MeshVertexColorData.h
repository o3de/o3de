#pragma once

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

#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshVertexColorData
                : public AZ::SceneAPI::DataTypes::IMeshVertexColorData
            {
            public:
                AZ_RTTI(MeshVertexColorData, "{17477B86-B163-4574-8FB2-4916BC218B3D}", AZ::SceneAPI::DataTypes::IMeshVertexColorData);

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API ~MeshVertexColorData() override = default;

                SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;

                SCENE_DATA_API const AZ::Name& GetCustomName() const override;
                SCENE_DATA_API void SetCustomName(const char* name);
                SCENE_DATA_API void SetCustomName(const AZ::Name& name);

                SCENE_DATA_API size_t GetCount() const override;
                SCENE_DATA_API const AZ::SceneAPI::DataTypes::Color& GetColor(size_t index) const override;

                // Pre-allocates memory for the color storage container. This can speed up loading as
                //      the container doesn't need to resize between adding colors.
                SCENE_DATA_API void ReserveContainerSpace(size_t size);
                SCENE_DATA_API void AppendColor(const AZ::SceneAPI::DataTypes::Color& color);

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::SceneAPI::DataTypes::Color> m_colors;
                AZ::Name m_customName;
            };
        } // GraphData
    } // SceneData
} // AZ
