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
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkinMeshAdvancedRule, SystemAllocator);

            SkinMeshAdvancedRule::SkinMeshAdvancedRule()
                : m_use32bitVertices(false)
                , m_useCustomNormals(true)
            {
                AZ::SceneAPI::Events::AssetImportRequestBus::Broadcast(&AZ::SceneAPI::Events::AssetImportRequestBus::Events::AreCustomNormalsUsed, m_useCustomNormals);
            }

            void SkinMeshAdvancedRule::SetUse32bitVertices(bool value)
            {
                m_use32bitVertices = value;
            }

            bool SkinMeshAdvancedRule::Use32bitVertices() const
            {
                return m_use32bitVertices;
            }

            bool SkinMeshAdvancedRule::MergeMeshes() const
            {
                return true;
            }

            void SkinMeshAdvancedRule::SetUseCustomNormals(bool value)
            {
                m_useCustomNormals = value;
            }

            bool SkinMeshAdvancedRule::UseCustomNormals() const
            {
                return m_useCustomNormals;
            }

            void SkinMeshAdvancedRule::SetVertexColorStreamName(const AZStd::string& name)
            {
                m_vertexColorStreamName = name;
            }

            void SkinMeshAdvancedRule::SetVertexColorStreamName(AZStd::string&& name)
            {
                m_vertexColorStreamName = AZStd::move(name);
            }

            const AZStd::string& SkinMeshAdvancedRule::GetVertexColorStreamName() const
            {
                return m_vertexColorStreamName;
            }

            bool SkinMeshAdvancedRule::IsVertexColorStreamDisabled() const
            {
                return m_vertexColorStreamName == DataTypes::s_advancedDisabledString;
            }

            void SkinMeshAdvancedRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkinMeshAdvancedRule, DataTypes::IMeshAdvancedRule>()->Version(6)
                    ->Field("use32bitVertices", &SkinMeshAdvancedRule::m_use32bitVertices)
                    ->Field("useCustomNormals", &SkinMeshAdvancedRule::m_useCustomNormals)
                    ->Field("vertexColorStreamName", &SkinMeshAdvancedRule::m_vertexColorStreamName);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkinMeshAdvancedRule>("Skin (Advanced)", "Configure advanced properties for this skin group.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::RadioButton, &SkinMeshAdvancedRule::m_use32bitVertices, "Vertex Precision",
                            "Selecting 32-bits of precision increases the accuracy of the position of each vertex which can be useful when the skin is located far from its pivot.\n\n"
                            "Please note that not all platforms support 32-bit vertices. For more details please see documentation."
                        )
                            ->Attribute(AZ::Edit::Attributes::FalseText, "16-bit")
                            ->Attribute(AZ::Edit::Attributes::TrueText, "32-bit")
                        ->DataElement(Edit::UIHandlers::Default, &SkinMeshAdvancedRule::m_useCustomNormals, "Use Custom Normals", "Use custom normals from DCC data or average them.")
                        ->DataElement("NodeListSelection", &SkinMeshAdvancedRule::m_vertexColorStreamName, "Vertex Color Stream",
                            "Select a vertex color stream to enable Vertex Coloring or 'Disable' to turn Vertex Coloring off.\n\n"
                            "Vertex Coloring works in conjunction with materials. If a material was previously generated,\n"
                            "changing vertex coloring will require the material to be reset or the material editor to be used\n"
                            "to enable 'Vertex Coloring'.")
                            ->Attribute("ClassTypeIdFilter", DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                            ->Attribute("DisabledOption", DataTypes::s_advancedDisabledString)
                            ->Attribute("UseShortNames", true);                            
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
