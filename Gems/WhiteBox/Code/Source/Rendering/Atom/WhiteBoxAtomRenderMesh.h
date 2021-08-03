/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Rendering/Atom/WhiteBoxAttributeBuffer.h>
#include <Rendering/Atom/WhiteBoxBuffer.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Rendering/WhiteBoxRenderMeshInterface.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Name/Name.h>

namespace AZ::RPI
{
    class ModelLodAsset;
    class ModelAsset;
    class Model;
} // namespace AZ::RPI

namespace WhiteBox
{
    class WhiteBoxMeshAtomData;

    //! A concrete implementation of RenderMeshInterface to support Atom rendering for the White Box Tool.
    class AtomRenderMesh : public RenderMeshInterface
    {
    public:
        AZ_RTTI(AtomRenderMesh, "{1F48D2F5-037C-400B-977C-7C0C9A34B84C}", RenderMeshInterface);

        // RenderMeshInterface ...
        void BuildMesh(
            const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal, AZ::EntityId entityId) override;
        void UpdateTransform(const AZ::Transform& worldFromLocal) override;
        void UpdateMaterial(const WhiteBoxMaterial& material) override;
        bool IsVisible() const override;
        void SetVisiblity(bool visibility) override;

    private:
        //! Creates an attribute buffer in the slot dictated by AttributeTypeT.
        template<AttributeType AttributeTypeT, typename VertexStreamDataType>
        void CreateAttributeBuffer(const AZStd::vector<VertexStreamDataType>& data)
        {
            const auto attribute_index = static_cast<size_t>(AttributeTypeT);
            m_attributes[attribute_index] = AZStd::make_unique<AttributeBuffer<AttributeTypeT>>(data);
        }

        //! Updates an attribute buffer in the slot dictated by AttributeTypeT.
        template<AttributeType AttributeTypeT, typename VertexStreamDataType>
        void UpdateAttributeBuffer(const AZStd::vector<VertexStreamDataType>& data)
        {
            const auto attribute_index = static_cast<size_t>(AttributeTypeT);
            auto& att = AZStd::get<attribute_index>(m_attributes[attribute_index]);
            att->UpdateData(data);
        }

        bool CreateMeshBuffers(const WhiteBoxMeshAtomData& meshData);
        bool UpdateMeshBuffers(const WhiteBoxMeshAtomData& meshData);
        bool MeshRequiresFullRebuild(const WhiteBoxMeshAtomData& meshData) const;
        bool CreateMesh(const WhiteBoxMeshAtomData& meshData, AZ::EntityId entityId);
        bool CreateLodAsset(const WhiteBoxMeshAtomData& meshData);
        void CreateModelAsset();
        bool CreateModel(AZ::EntityId entityId);
        void AddLodBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator);
        void AddMeshBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator);
        bool AreAttributesValid() const;
        bool DoesMeshRequireFullRebuild(const WhiteBoxMeshAtomData& meshData) const;

        AZ::Data::Asset<AZ::RPI::ModelLodAsset> m_lodAsset;
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        AZ::Data::Instance<AZ::RPI::Model> m_model;
        AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
        uint32_t m_vertexCount = 0;
        AZStd::unique_ptr<IndexBuffer> m_indexBuffer;
        AZStd::array<
            AZStd::variant<
                AZStd::unique_ptr<PositionAttribute>, AZStd::unique_ptr<NormalAttribute>,
                AZStd::unique_ptr<TangentAttribute>, AZStd::unique_ptr<BitangentAttribute>,
                AZStd::unique_ptr<UVAttribute>, AZStd::unique_ptr<ColorAttribute>>,
            NumAttributes>
            m_attributes;

        //! Default white box mesh material.
        // TODO: LYN-784
        static constexpr AZStd::string_view TexturedMaterialPath = "materials/defaultpbr.azmaterial";
        static constexpr AZStd::string_view SolidMaterialPath = "materials/defaultpbr.azmaterial";
        static constexpr AZ::RPI::ModelMaterialSlot::StableId OneMaterialSlotId = 0;

        //! White box model name.
        static constexpr AZStd::string_view ModelName = "WhiteBoxMesh";
    };
} // namespace WhiteBox
