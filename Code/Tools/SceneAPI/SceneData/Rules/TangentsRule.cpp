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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            TangentsRule::TangentsRule()
                : DataTypes::IRule()
                , m_tangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::MikkT)
                , m_bitangentMethod(AZ::SceneAPI::DataTypes::BitangentMethod::Orthogonal)
                , m_uvSetIndex(0)
                , m_normalize(true)
            {
            }


            AZ::SceneAPI::DataTypes::TangentSpace TangentsRule::GetTangentSpace() const
            {
                return m_tangentSpace;
            }


            AZ::SceneAPI::DataTypes::BitangentMethod TangentsRule::GetBitangentMethod() const
            {
                return m_bitangentMethod;
            }


            AZ::u64 TangentsRule::GetUVSetIndex() const
            {
                return m_uvSetIndex;
            }


            bool TangentsRule::GetNormalizeVectors() const
            {
                return m_normalize;
            }


            // Find UV data.
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* TangentsRule::FindUVData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet)
            {
                const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

                AZ::u64 uvSetIndex = 0;
                auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
                for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
                {
                    AZ::SceneAPI::DataTypes::IMeshVertexUVData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
                    if (data)
                    {
                        if (uvSetIndex == uvSet)
                        {
                            return data;
                        }
                        uvSetIndex++;
                    }
                }

                return nullptr;
            }


            // Find tangent data.
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* TangentsRule::FindTangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace)
            {
                const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

                auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
                for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
                {
                    AZ::SceneAPI::DataTypes::IMeshVertexTangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexTangentData*>(child->second.get());
                    if (data)
                    {
                        if (setIndex == data->GetTangentSetIndex() && tangentSpace == data->GetTangentSpace())
                        {
                            return data;
                        }
                    }
                }

                return nullptr;
            }


            // Find bitangent data.
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* TangentsRule::FindBitangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace)
            {
                const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

                auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
                for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
                {
                    AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*>(child->second.get());
                    if (data)
                    {
                        if (setIndex == data->GetBitangentSetIndex() && tangentSpace == data->GetTangentSpace())
                        {
                            return data;
                        }
                    }
                }

                return nullptr;
            }

            AZ::Crc32 TangentsRule::GetNormalizeVisibility() const
            {
                return (m_tangentSpace == AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX) ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
            }

            AZ::Crc32 TangentsRule::GetOrthogonalVisibility() const
            {
                return (m_tangentSpace == AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX) ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
            }

            void TangentsRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<TangentsRule, DataTypes::IRule>()->Version(1)
                    ->Field("tangentSpace", &TangentsRule::m_tangentSpace)
                    ->Field("bitangentMethod", &TangentsRule::m_bitangentMethod)
                    ->Field("normalize", &TangentsRule::m_normalize)
                    ->Field("uvSetIndex", &TangentsRule::m_uvSetIndex);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<TangentsRule>("Tangents", "Specify how tangents are imported or generated.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AZ::SceneAPI::SceneData::TangentsRule::m_tangentSpace, "Tangent space", "Specify the tangent space used for normal map baking. Choose 'From Fbx' to extract the tangents and bitangents directly from the Fbx file. When there is no tangents rule or the Fbx has no tangents stored inside it, the 'MikkT' option will be used with orthogonal tangents of unit length, so with the normalize option enabled, using the first UV set.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx, "From Fbx")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::MikkT, "MikkT")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX, "EMotion FX")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AZ::SceneAPI::SceneData::TangentsRule::m_bitangentMethod, "Bitangents", "Set to 'use from tangent space' to use the bitangents generated by the algorithm used or inside the fbx file. This can result in non-orthogonal tangents. Set to 'orthogonal' to skip storing the bitangents and let the engine calculate the bitangents in a way it will be perpendicular to both the normal and tangent.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::BitangentMethod::UseFromTangentSpace, "Use from tangent space")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::BitangentMethod::Orthogonal, "Orthogonal")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &TangentsRule::GetOrthogonalVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TangentsRule::m_uvSetIndex, "Uv set", "The UV set index to generate the tangents from. A value of 0 means the first uv set, while 1 means the second uv set.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 1)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TangentsRule::m_normalize, "Normalize", "Normalize the tangents and bitangents? When disabled the vectors might no be unit length, which can be useful for relief mapping.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &TangentsRule::GetNormalizeVisibility)
                    ;
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
