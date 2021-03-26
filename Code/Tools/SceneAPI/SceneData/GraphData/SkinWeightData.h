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
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SkinWeightData
                : public SceneAPI::DataTypes::ISkinWeightData
            {
            public:
                AZ_RTTI(SkinWeightData, "{2175A399-8EAA-4BFF-9720-C5FED739717E}", SceneAPI::DataTypes::ISkinWeightData);

                SCENE_DATA_API ~SkinWeightData() override = default;

                SCENE_DATA_API size_t GetVertexCount() const override;
                SCENE_DATA_API size_t GetLinkCount(size_t vertexIndex) const override;
                SCENE_DATA_API const Link& GetLink(size_t vertexIndex, size_t linkIndex) const override;
                SCENE_DATA_API Link& GetLink(size_t vertexIndex, size_t linkIndex);
                SCENE_DATA_API size_t GetBoneCount() const override;
                SCENE_DATA_API const AZStd::string& GetBoneName(int boneId) const override;

                SCENE_DATA_API void ResizeContainerSpace(size_t size);
                SCENE_DATA_API void AppendLink(size_t vertexIndex, const SceneAPI::DataTypes::ISkinWeightData::Link& link);
                SCENE_DATA_API void AddAndSortLink(size_t vertexIndex, const SceneAPI::DataTypes::ISkinWeightData::Link& link);

                SCENE_DATA_API int GetBoneId(const AZStd::string& boneName);

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZStd::vector<SceneAPI::DataTypes::ISkinWeightData::Link>> m_vertexLinks;
                AZStd::unordered_map<AZStd::string, int> m_boneNameIdMap;
                AZStd::unordered_map<int, AZStd::string> m_boneIdNameMap;
            };
        } // GraphData
    } // SceneData
} // AZ
