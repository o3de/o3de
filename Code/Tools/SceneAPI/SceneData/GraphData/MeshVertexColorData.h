#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
