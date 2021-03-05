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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IMeshVertexUVData;
            class IMeshVertexTangentData;
            class IMeshVertexBitangentData;
        }

        namespace SceneData
        {
            class SCENE_DATA_CLASS TangentsRule
                : public DataTypes::IRule
            {
            public:
                AZ_RTTI(TangentsRule, "{4BD1CE13-D2EB-4CCF-AB21-4877EF69DE7D}", DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(TangentsRule, AZ::SystemAllocator, 0)

                SCENE_DATA_API TangentsRule();
                SCENE_DATA_API ~TangentsRule() override = default;

                SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentSpace GetTangentSpace() const;
                SCENE_DATA_API AZ::SceneAPI::DataTypes::BitangentMethod GetBitangentMethod() const;
                SCENE_DATA_API AZ::u64 GetUVSetIndex() const;
                SCENE_DATA_API bool GetNormalizeVectors() const;

                SCENE_DATA_API static AZ::SceneAPI::DataTypes::IMeshVertexUVData* FindUVData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet);
                SCENE_DATA_API static AZ::SceneAPI::DataTypes::IMeshVertexTangentData* FindTangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace);
                SCENE_DATA_API static AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* FindBitangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace);

                static void Reflect(ReflectContext* context);

            protected:
                AZ::Crc32 GetNormalizeVisibility() const;
                AZ::Crc32 GetOrthogonalVisibility() const;

                AZ::SceneAPI::DataTypes::TangentSpace       m_tangentSpace;     /**< Specifies how to handle tangents. Either generate them, or import them. */
                AZ::SceneAPI::DataTypes::BitangentMethod    m_bitangentMethod;  /**< Grab the bitangents from the generator/source or use an orthogonal basis by always calculating them? */
                AZ::u64                                     m_uvSetIndex;       /**< Generate the tangents from this UV set. */
                bool                                        m_normalize;        /**< Normalize the tangent and bitangents? */
            };
        } // SceneData
    } // SceneAPI
} // AZ
