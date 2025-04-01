#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshVertexUVData
                : public AZ::SceneAPI::DataTypes::IMeshVertexUVData
            {
            public:
                AZ_RTTI(MeshVertexUVData, "{B435C091-482C-4EB9-B1F4-FA5B480796DA}", AZ::SceneAPI::DataTypes::IMeshVertexUVData);

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API ~MeshVertexUVData() override = default;

                SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;

                SCENE_DATA_API const AZ::Name& GetCustomName() const override;
                SCENE_DATA_API void SetCustomName(const char* name);
                SCENE_DATA_API void SetCustomName(const AZ::Name& name);

                SCENE_DATA_API size_t GetCount() const override;
                SCENE_DATA_API const AZ::Vector2& GetUV(size_t index) const override;

                // Pre-allocate memory
                SCENE_DATA_API void ReserveContainerSpace(size_t size);
                SCENE_DATA_API void AppendUV(const AZ::Vector2& uv);

                SCENE_DATA_API void Clear();

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::Vector2> m_uvs;
                AZ::Name m_customName;
            };
        } // GraphData
    } // SceneData
} // AZ
