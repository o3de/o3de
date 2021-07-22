/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                , m_normalsSource(NormalsSource::FromSourceScene)
                , m_shouldRenormalizeNormals(false)
            {
            }

            AZ::SceneAPI::DataTypes::TangentSpace TangentsRule::GetTangentSpace() const
            {
                return m_tangentSpace;
            }

            SCENE_DATA_API NormalsSource TangentsRule::GetNormalsSource() const
            {
                return m_normalsSource;
            }

            SCENE_DATA_API bool TangentsRule::GetShouldRenormalizeNormals() const
            {
                return m_shouldRenormalizeNormals;
            }

            SCENE_DATA_API const TangentsRule& TangentsRule::GetDefault()
            {
                static const TangentsRule def;
                return def;
            }

            void TangentsRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<TangentsRule, DataTypes::IRule>()->Version(4)
                    ->Field("tangentSpace", &TangentsRule::m_tangentSpace)
                    ->Field("normalsSource", &TangentsRule::m_normalsSource)
                    ->Field("shouldRenormalizeNormals", &TangentsRule::m_shouldRenormalizeNormals)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<TangentsRule>("Normals and Tangents", "Specify how normals and tangents are imported or generated.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AZ::SceneAPI::SceneData::TangentsRule::m_tangentSpace, "Tangent space", "Specify the tangent space used for normal map baking. Choose 'From Fbx' to extract the tangents and bitangents directly from the Fbx file. When there is no tangents rule or the Fbx has no tangents stored inside it, the 'MikkT' option will be used with orthogonal tangents of unit length, so with the normalize option enabled, using the first UV set.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene, "From Source Scene")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::MikkT, "MikkT")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AZ::SceneAPI::SceneData::TangentsRule::m_normalsSource,
                            "Normals",
                            "Specify what normals are used for this mesh. "
                            "'From Source Scene' - use precisely those normals which are stored in the source FBX. "
                            "'Flat Shaded' - generate flat shaded vertex normals based on face normals (only works for non-indexed meshes). "
                            "'Smooth Shaded' - generate smooth shaded vertex normals based on face normals by interpolating between normals of adjacent faces. "
                        )
                        ->EnumAttribute(NormalsSource::FromSourceScene, "From Source Scene")
                        ->EnumAttribute(NormalsSource::Flat, "Flat Shaded")
                        ->EnumAttribute(NormalsSource::Smoothed, "Smooth Shaded")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                        ->DataElement(0, &AZ::SceneAPI::SceneData::TangentsRule::m_shouldRenormalizeNormals,
                            "Re-normalize normals",
                            "Esoteric option, use rarely. If your normals are not actually normalized, which should never actually occur, enable this to normalize them."
                        )
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ;
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
