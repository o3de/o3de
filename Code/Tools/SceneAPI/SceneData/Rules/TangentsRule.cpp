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
            {
            }

            AZ::SceneAPI::DataTypes::TangentSpace TangentsRule::GetTangentSpace() const
            {
                return m_tangentSpace;
            }

            void TangentsRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<TangentsRule, DataTypes::IRule>()->Version(3)
                    ->Field("tangentSpace", &TangentsRule::m_tangentSpace);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<TangentsRule>("Tangents", "Specify how tangents are imported or generated.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AZ::SceneAPI::SceneData::TangentsRule::m_tangentSpace, "Tangent space", "Specify the tangent space used for normal map baking. Choose 'From Fbx' to extract the tangents and bitangents directly from the Fbx file. When there is no tangents rule or the Fbx has no tangents stored inside it, the 'MikkT' option will be used with orthogonal tangents of unit length, so with the normalize option enabled, using the first UV set.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene, "From Source Scene")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::TangentSpace::MikkT, "MikkT")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ;
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
