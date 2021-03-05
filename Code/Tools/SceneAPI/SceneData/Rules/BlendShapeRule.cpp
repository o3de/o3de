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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(BlendShapeRule, AZ::SystemAllocator, 0)

            SceneNodeSelectionList& BlendShapeRule::GetNodeSelectionList()
            {
                return m_blendShapes;
            }

            DataTypes::ISceneNodeSelectionList& BlendShapeRule::GetSceneNodeSelectionList()
            {
                return m_blendShapes;
            }

            const DataTypes::ISceneNodeSelectionList& BlendShapeRule::GetSceneNodeSelectionList() const
            {
                return m_blendShapes;
            }

            void BlendShapeRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<BlendShapeRule, DataTypes::IBlendShapeRule>()->Version(1)
                    ->Field("blendShapes", &BlendShapeRule::m_blendShapes);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<BlendShapeRule>("Blend shapes", "Select mesh targets to configure blend shapes at a later time using Lumberyard.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &BlendShapeRule::m_blendShapes, "Select blend shapes", 
                            "Select 1 or more meshes to include in the skin group for later use with the blend shape system.")
                            ->Attribute("FilterName", "blend shapes")
                            ->Attribute("FilterType", DataTypes::IBlendShapeData::TYPEINFO_Uuid())
                            ->Attribute("NarrowSelection", true);

                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
