/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

#include <Pipeline/SceneAPIExt/ClothRule.h>

namespace NvCloth
{
    namespace Pipeline
    {
        // It's necessary for the rule to specify the system allocator, otherwise
        // the editor crashes when deleting the cloth modifier from Scene Settings.
        AZ_CLASS_ALLOCATOR_IMPL(ClothRule, AZ::SystemAllocator)

        const char* const ClothRule::DefaultChooseNodeName = "Choose a node";
        const char* const ClothRule::DefaultInverseMassesString = "Default: 1.0";
        const char* const ClothRule::DefaultMotionConstraintsString = "Default: 1.0";
        const char* const ClothRule::DefaultBackstopString = "None";

        const AZStd::string& ClothRule::GetMeshNodeName() const
        {
            return m_meshNodeName;
        }

        AZStd::vector<AZ::Color> ClothRule::ExtractClothData(
            const AZ::SceneAPI::Containers::SceneGraph& graph,
            const size_t numVertices) const
        {
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex meshNodeIndex = [this, &graph]()
            {
                const auto originalMeshIndex = graph.Find(GetMeshNodeName());
                return AZ::SceneAPI::Utilities::SceneGraphSelector::RemapToOptimizedMesh(
                    graph, originalMeshIndex);
            }();

            if (!meshNodeIndex.IsValid())
            {
                return {};
            }

            const float defaultInverseMass = 1.0f;
            const float defaultMotionConstraint = 1.0f;
            const float defaultBackstopOffset = 0.5f; // 0.5 means offset 0 once the range is converted from [0,1] -> [-1,1]
            const float defaultBackstopRadius = 0.0f;

            using GetterFunction = AZStd::function<float(size_t index)>;

            GetterFunction getInverseMass = [defaultInverseMass]([[maybe_unused]] size_t index) { return defaultInverseMass; };
            GetterFunction getMotionConstraint = [defaultMotionConstraint]([[maybe_unused]] size_t index) { return defaultMotionConstraint; };
            GetterFunction getBackstopOffset = [defaultBackstopOffset]([[maybe_unused]] size_t index) { return defaultBackstopOffset; };
            GetterFunction getBackstopRadius = [defaultBackstopRadius]([[maybe_unused]] size_t index) { return defaultBackstopRadius; };

            auto getColorChannelSafe = [](
                const AZ::SceneAPI::DataTypes::IMeshVertexColorData* data,
                size_t index,
                AZ::SceneAPI::DataTypes::ColorChannel channel)
            {
                const float colorChannel = data->GetColor(index).GetChannel(channel);
                return AZ::GetClamp(colorChannel, 0.0f, 1.0f);
            };

            if (!IsInverseMassesStreamDisabled())
            {
                if (auto data = FindVertexColorData(graph, meshNodeIndex, m_inverseMassesStreamName, numVertices))
                {
                    getInverseMass =
                        [&getColorChannelSafe, data, channel = m_inverseMassesChannel](size_t index)
                        {
                            return getColorChannelSafe(data.get(), index, channel);
                        };
                }
            }

            if (!IsMotionConstraintsStreamDisabled())
            {
                if (auto data = FindVertexColorData(graph, meshNodeIndex, m_motionConstraintsStreamName, numVertices))
                {
                    getMotionConstraint =
                        [&getColorChannelSafe, data, channel = m_motionConstraintsChannel](size_t index)
                        {
                            return getColorChannelSafe(data.get(), index, channel);
                        };
                }
            }

            if (!IsBackstopStreamDisabled())
            {
                if (auto data = FindVertexColorData(graph, meshNodeIndex, m_backstopStreamName, numVertices))
                {
                    getBackstopOffset =
                        [&getColorChannelSafe, data, channel = m_backstopOffsetChannel](size_t index)
                        {
                            return getColorChannelSafe(data.get(), index, channel);
                        };
                    getBackstopRadius =
                        [&getColorChannelSafe, data, channel = m_backstopRadiusChannel](size_t index)
                        {
                            return getColorChannelSafe(data.get(), index, channel);
                        };
                }
            }

            AZStd::vector<AZ::Color> clothData;
            clothData.resize_no_construct(numVertices);

            // Compile all the data to the vertex color stream of the mesh.
            for (size_t i = 0; i < numVertices; ++i)
            {
                clothData[i].Set(
                    getInverseMass(i), // Store inverse masses in red channel
                    getMotionConstraint(i), // Store motion constraints in green channel
                    getBackstopOffset(i), // Store backstop offsets in blue channel
                    getBackstopRadius(i)); // Store backstop radius in alpha channel
            }

            return clothData;
        }

        const AZStd::string& ClothRule::GetInverseMassesStreamName() const
        {
            return m_inverseMassesStreamName;
        }

        const AZStd::string& ClothRule::GetMotionConstraintsStreamName() const
        {
            return m_motionConstraintsStreamName;
        }

        const AZStd::string& ClothRule::GetBackstopStreamName() const
        {
            return m_backstopStreamName;
        }

        void ClothRule::SetMeshNodeName(const AZStd::string& name)
        {
            m_meshNodeName = name;
        }

        void ClothRule::SetInverseMassesStreamName(const AZStd::string& name)
        {
            m_inverseMassesStreamName = name;
        }

        void ClothRule::SetMotionConstraintsStreamName(const AZStd::string& name)
        {
            m_motionConstraintsStreamName = name;
        }

        void ClothRule::SetBackstopStreamName(const AZStd::string& name)
        {
            m_backstopStreamName = name;
        }

        bool ClothRule::IsInverseMassesStreamDisabled() const
        {
            return m_inverseMassesStreamName == DefaultInverseMassesString;
        }

        bool ClothRule::IsMotionConstraintsStreamDisabled() const
        {
            return m_motionConstraintsStreamName == DefaultMotionConstraintsString;
        }

        bool ClothRule::IsBackstopStreamDisabled() const
        {
            return m_backstopStreamName == DefaultBackstopString;
        }

        AZ::SceneAPI::DataTypes::ColorChannel ClothRule::GetInverseMassesStreamChannel() const
        {
            return m_inverseMassesChannel;
        }

        AZ::SceneAPI::DataTypes::ColorChannel ClothRule::GetMotionConstraintsStreamChannel() const
        {
            return m_motionConstraintsChannel;
        }

        AZ::SceneAPI::DataTypes::ColorChannel ClothRule::GetBackstopOffsetStreamChannel() const
        {
            return m_backstopOffsetChannel;
        }

        AZ::SceneAPI::DataTypes::ColorChannel ClothRule::GetBackstopRadiusStreamChannel() const
        {
            return m_backstopRadiusChannel;
        }

        void ClothRule::SetInverseMassesStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel)
        {
            m_inverseMassesChannel = channel;
        }

        void ClothRule::SetMotionConstraintsStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel)
        {
            m_motionConstraintsChannel = channel;
        }

        void ClothRule::SetBackstopOffsetStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel)
        {
            m_backstopOffsetChannel = channel;
        }

        void ClothRule::SetBackstopRadiusStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel)
        {
            m_backstopRadiusChannel = channel;
        }

        AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshVertexColorData> ClothRule::FindVertexColorData(
            const AZ::SceneAPI::Containers::SceneGraph& graph,
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex,
            const AZStd::string& vertexColorName,
            const size_t numVertices) const
        {
            if (vertexColorName.empty())
            {
                return nullptr;
            }

            const auto vertexColorNodeIndex = graph.Find(meshNodeIndex, vertexColorName);
            auto vertexColorData = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshVertexColorData*>(graph.GetNodeContent(vertexColorNodeIndex));
            if (vertexColorData)
            {
                if (numVertices != vertexColorData->GetCount())
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow,
                        "Number of vertices in the mesh node '%s' (%zu) doesn't match with the number of stored vertex color stream '%s' (%zu).",
                        GetMeshNodeName().c_str(), numVertices, vertexColorName.c_str(), vertexColorData->GetCount());
                    vertexColorData.reset();
                }
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow,
                    "Vertex color stream '%s' not found for mesh node '%s'.",
                    vertexColorName.c_str(),
                    GetMeshNodeName().c_str());
            }

            return vertexColorData;
        }

        void ClothRule::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<AZ::SceneAPI::DataTypes::IClothRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

            serializeContext->Class<ClothRule, AZ::SceneAPI::DataTypes::IClothRule>()
                ->Version(2, &VersionConverter)
                ->Field("meshNodeName", &ClothRule::m_meshNodeName)
                ->Field("inverseMassesStreamName", &ClothRule::m_inverseMassesStreamName)
                ->Field("inverseMassesChannel", &ClothRule::m_inverseMassesChannel)
                ->Field("motionConstraintsStreamName", &ClothRule::m_motionConstraintsStreamName)
                ->Field("motionConstraintsChannel", &ClothRule::m_motionConstraintsChannel)
                ->Field("backstopStreamName", &ClothRule::m_backstopStreamName)
                ->Field("backstopOffsetChannel", &ClothRule::m_backstopOffsetChannel)
                ->Field("backstopRadiusChannel", &ClothRule::m_backstopRadiusChannel);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ClothRule>("Cloth", "Adds cloth data to the exported CGF asset. The cloth data will be used to determine what meshes to use for cloth simulation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->DataElement("NodeListSelection", &ClothRule::m_meshNodeName, "Select Cloth Mesh", "Mesh used for cloth simulation.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", DefaultChooseNodeName)
                    ->DataElement("NodeListSelection", &ClothRule::m_inverseMassesStreamName, "Inverse Masses",
                        "Select the 'vertex color' stream that contains cloth inverse masses or 'Default: 1.0' to use mass 1.0 for all vertices.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", DefaultInverseMassesString)
                        ->Attribute("UseShortNames", true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ClothRule::m_inverseMassesChannel, "Inverse Masses Channel",
                        "Select which color channel to obtain the inverse mass information from.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Red, "Red")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Green, "Green")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Blue, "Blue")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Alpha, "Alpha")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothRule::IsInverseMassesStreamDisabled)
                    ->DataElement("NodeListSelection", &ClothRule::m_motionConstraintsStreamName, "Motion Constraints",
                        "Select the 'vertex color' stream that contains cloth motion constraints or 'Default: 1.0' to use 1.0 for all vertices.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", DefaultMotionConstraintsString)
                        ->Attribute("UseShortNames", true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ClothRule::m_motionConstraintsChannel, "Motion Constraints Channel",
                        "Select which color channel to obtain the motion constraints information from.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Red, "Red")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Green, "Green")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Blue, "Blue")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Alpha, "Alpha")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothRule::IsMotionConstraintsStreamDisabled)
                    ->DataElement("NodeListSelection", &ClothRule::m_backstopStreamName, "Backstop",
                        "Select the 'vertex color' stream that contains cloth backstop data.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", DefaultBackstopString)
                        ->Attribute("UseShortNames", true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ClothRule::m_backstopOffsetChannel, "Backstop Offset Channel",
                        "Select which color channel to obtain the backstop offset from.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Red, "Red")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Green, "Green")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Blue, "Blue")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Alpha, "Alpha")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothRule::IsBackstopStreamDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ClothRule::m_backstopRadiusChannel, "Backstop Radius Channel",
                        "Select which color channel to obtain the backstop radius from.")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Red, "Red")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Green, "Green")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Blue, "Blue")
                        ->EnumAttribute(AZ::SceneAPI::DataTypes::ColorChannel::Alpha, "Alpha")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ClothRule::IsBackstopStreamDisabled);
            }
        }

        bool ClothRule::VersionConverter(
            AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                AZStd::string vertexColorStreamName;
                classElement.FindSubElementAndGetData(AZ_CRC_CE("vertexColorStreamName"), vertexColorStreamName);
                classElement.RemoveElementByName(AZ_CRC_CE("vertexColorStreamName"));
                classElement.AddElementWithData(context, "inverseMassesStreamName", vertexColorStreamName.empty() ? AZStd::string(DefaultInverseMassesString) : vertexColorStreamName);
                classElement.AddElementWithData(context, "motionConstraintsStreamName", AZStd::string(DefaultMotionConstraintsString));
                classElement.AddElementWithData(context, "backstopStreamName", AZStd::string(DefaultBackstopString));
            }

            return true;
        }
    } // namespace Pipeline
} // namespace NvCloth
