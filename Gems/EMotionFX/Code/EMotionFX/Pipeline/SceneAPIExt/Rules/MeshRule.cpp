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
#include <SceneAPIExt/Rules/MeshRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MeshRule, AZ::SystemAllocator, 0)

            const char* MeshRule::m_disabledVertexColorsName = "No vertex colors";

            MeshRule::MeshRule()
            {
            }

            bool MeshRule::IsVertexColorsDisabled() const
            {
                return (m_vertexColorStreamName == m_disabledVertexColorsName);
            }

            IMeshRule::VertexColorMode MeshRule::GetVertexColorMode() const
            {
                return m_vertexColorMode;
            }

            const AZStd::string& MeshRule::GetVertexColorStreamName() const
            {
                return m_vertexColorStreamName;
            }

            void MeshRule::DisableVertexColors()
            {
                m_vertexColorStreamName = m_disabledVertexColorsName;
            }

            bool MeshRule::VersionConverter([[maybe_unused]] AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
            {
                if (rootElementNode.GetVersion() < 4)
                {
                    rootElementNode.RemoveElementByName(AZ_CRC("optimizeTriangleList", 0xfc208cc5));
                }
                return true;
            }

            void MeshRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IMeshRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

                serializeContext->Class<MeshRule, IMeshRule>()
                    ->Version(4, VersionConverter)
                    ->Field("vertexColorStreamName", &MeshRule::m_vertexColorStreamName)
                    ->Field("vertexColorMode", &MeshRule::m_vertexColorMode);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MeshRule>("Mesh", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement("NodeListSelection", &MeshRule::m_vertexColorStreamName, "Vertex color stream",
                            "Select the vertex color stream that will be used to color the rendered meshes.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                            ->Attribute("DisabledOption", m_disabledVertexColorsName)
                            ->Attribute("DefaultToDisabled", true)
                            ->Attribute("UseShortNames", true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshRule::m_vertexColorMode, "Vertex color mode", "What precision should we export vertex colors in?")
                        ->EnumAttribute(VertexColorMode::Precision_32, "32 bit (8 bits per channel)")
                        ->EnumAttribute(VertexColorMode::Precision_128, "128 bit (32 bits per channel)");
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
