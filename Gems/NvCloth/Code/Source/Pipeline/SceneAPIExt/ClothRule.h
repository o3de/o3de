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
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace AZ
{
    class ReflectContext;
}

namespace NvCloth
{
    namespace Pipeline
    {
        //! This class represents the data of a cloth rule (aka cloth modifier).
        class ClothRule
            : public AZ::SceneAPI::DataTypes::IClothRule
        {
        public:
            AZ_RTTI(ClothRule, "{2F5AC324-314A-4C53-AFFF-DDFA46605DDB}", AZ::SceneAPI::DataTypes::IClothRule);
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(AZ::ReflectContext* context);
            static bool VersionConverter(
                AZ::SerializeContext& context,
                AZ::SerializeContext::DataElementNode& classElement);

            static const char* const DefaultChooseNodeName;
            static const char* const DefaultInverseMassesString;
            static const char* const DefaultMotionConstraintsString;
            static const char* const DefaultBackstopString;

            // IClothRule overrides ...
            const AZStd::string& GetMeshNodeName() const override;
            AZStd::vector<AZ::Color> ExtractClothData(const AZ::SceneAPI::Containers::SceneGraph& graph, const size_t numVertices) const override;

            const AZStd::string& GetInverseMassesStreamName() const;
            const AZStd::string& GetMotionConstraintsStreamName() const;
            const AZStd::string& GetBackstopStreamName() const;

            void SetMeshNodeName(const AZStd::string& name);
            void SetInverseMassesStreamName(const AZStd::string& name);
            void SetMotionConstraintsStreamName(const AZStd::string& name);
            void SetBackstopStreamName(const AZStd::string& name);

            bool IsInverseMassesStreamDisabled() const;
            bool IsMotionConstraintsStreamDisabled() const;
            bool IsBackstopStreamDisabled() const;

            AZ::SceneAPI::DataTypes::ColorChannel GetInverseMassesStreamChannel() const;
            AZ::SceneAPI::DataTypes::ColorChannel GetMotionConstraintsStreamChannel() const;
            AZ::SceneAPI::DataTypes::ColorChannel GetBackstopOffsetStreamChannel() const;
            AZ::SceneAPI::DataTypes::ColorChannel GetBackstopRadiusStreamChannel() const;

            void SetInverseMassesStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel);
            void SetMotionConstraintsStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel);
            void SetBackstopOffsetStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel);
            void SetBackstopRadiusStreamChannel(AZ::SceneAPI::DataTypes::ColorChannel channel);

        protected:
            AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshVertexColorData> FindVertexColorData(
                const AZ::SceneAPI::Containers::SceneGraph& graph,
                const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex,
                const AZStd::string& vertexColorName,
                const size_t numVertices) const;

            AZStd::string m_meshNodeName;

            AZStd::string m_inverseMassesStreamName;
            AZStd::string m_motionConstraintsStreamName;
            AZStd::string m_backstopStreamName;

            AZ::SceneAPI::DataTypes::ColorChannel m_inverseMassesChannel = AZ::SceneAPI::DataTypes::ColorChannel::Red;
            AZ::SceneAPI::DataTypes::ColorChannel m_motionConstraintsChannel = AZ::SceneAPI::DataTypes::ColorChannel::Red;
            AZ::SceneAPI::DataTypes::ColorChannel m_backstopOffsetChannel = AZ::SceneAPI::DataTypes::ColorChannel::Red;
            AZ::SceneAPI::DataTypes::ColorChannel m_backstopRadiusChannel = AZ::SceneAPI::DataTypes::ColorChannel::Green;
        };
    } // namespace Pipeline
} // namespace NvCloth
