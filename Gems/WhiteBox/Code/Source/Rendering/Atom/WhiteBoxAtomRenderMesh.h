/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>

#include <Rendering/Atom/WhiteBoxAttributeBuffer.h>
#include <Rendering/Atom/WhiteBoxBuffer.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Rendering/WhiteBoxRenderMeshInterface.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ::RPI
{
    class ModelLodAsset;
    class ModelAsset;
    class Model;
} // namespace AZ::RPI

namespace WhiteBox
{
    class WhiteBoxMeshAtomData;
    struct SharedModel;
    struct SharedMaterial;
    struct WhiteBoxMaterial;

    //! Atom RenderMeshInterface for White Box. The primary mesh is a material consumer, so a Material
    //! component can override its slots as for the Mesh component.
    class AtomRenderMesh
        : public RenderMeshInterface
        , private AZ::Render::MeshHandleStateRequestBus::Handler
        , private AZ::Render::MaterialConsumerRequestBus::Handler
        , private AZ::Render::MaterialComponentNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_RTTI(AtomRenderMesh, "{1F48D2F5-037C-400B-977C-7C0C9A34B84C}", RenderMeshInterface);

        //! @param isPrimary when false this is an auxiliary mesh (e.g. one of several per-layer
        //! tinted meshes on the same entity) and must NOT own the single per-entity mesh-handle state.
        explicit AtomRenderMesh(AZ::EntityId entityId, bool isPrimary = true);
        ~AtomRenderMesh();

        // RenderMeshInterface ...
        void BuildMesh(
            const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal) override;
        void UpdateTransform(const AZ::Transform& worldFromLocal) override;
        void UpdateMaterial(const WhiteBoxMaterial& material) override;
        void SetMaterialAssetOverride(const AZ::Data::AssetId& materialAssetId) override;
        bool IsVisible() const override;
        void SetVisiblity(bool visibility) override;

        // AZ::TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Console helpers (wb_renderStats, wb_renderVisible): what every live White Box mesh submits and stores,
        //! and a switch that shows or hides all of them for an A/B frame-time comparison.
        static void PrintRenderStats();
        static void SetAllVisible(bool visible);

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

        // MeshHandleStateRequestBus overrides ...
        const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* GetMeshHandle() const override;

        // MaterialConsumerRequestBus overrides ...
        AZ::Render::MaterialAssignmentId FindMaterialAssignmentId(
            const AZ::Render::MaterialAssignmentLodIndex lod, const AZStd::string& label) const override;
        AZ::Render::MaterialAssignmentLabelMap GetMaterialLabels() const override;
        AZ::Render::MaterialAssignmentMap GetDefaultMaterialMap() const override;
        AZStd::unordered_set<AZ::Name> GetModelUvNames() const override;

        // MaterialComponentNotificationBus overrides ...
        void OnMaterialsUpdated(const AZ::Render::MaterialAssignmentMap& materials) override;
        void OnMaterialPropertiesUpdated(const AZ::Render::MaterialAssignmentMap& materials) override;

        //! Per slot: the Material component's override (slot, then whole model) or else our own instance.
        AZ::Render::CustomMaterialMap BuildCustomMaterials(const AZ::Render::MaterialAssignmentMap& overrides) const;
        //! Hand the Material component's instances to the mesh, skipping the rebind when every slot already draws with them.
        void BindCustomMaterials(const AZ::Render::MaterialAssignmentMap& overrides, bool force);
        //! Remember which instance each slot draws with, so a later notification can tell whether anything changed.
        void RememberBoundMaterials(const AZ::Render::CustomMaterialMap& materials);
        //! An override made before its slot's default asset was known has no instance; ask the Material component to reload.
        void ReloadOverridesWithoutInstance(const AZ::Render::MaterialAssignmentMap& overrides);
        //! Tells a Material component the slot list changed, when it did since the last call.
        void PublishMaterialSlots();

        bool CreateMeshBuffers(const WhiteBoxMeshAtomData& meshData);
        bool UpdateMeshBuffers(const WhiteBoxMeshAtomData& meshData);
        bool MeshRequiresFullRebuild(const WhiteBoxMeshAtomData& meshData) const;
        bool CreateMesh(const WhiteBoxMeshAtomData& meshData, uint64_t modelKey, const WhiteBoxMaterial& material);
        //! Give each group the material instance for its asset and @p material settings, shared with every other
        //! White Box that asks for the same thing. Fills @p assets with the asset each model slot defaults to.
        bool CreateMaterialInstances(const WhiteBoxMaterial& material, AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>>& assets);
        //! Draw with a model another White Box already built from identical geometry instead of building one.
        bool UseSharedModel(const AZStd::shared_ptr<SharedModel>& shared, const WhiteBoxMaterial& material);
        bool CreateLodAsset(const WhiteBoxMeshAtomData& meshData);
        bool CreateModelAsset(const AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>>& materialAssets);
        bool CreateModel();
        //! Hand m_modelAsset to the mesh feature processor with this entity's materials.
        bool AcquireMesh();
        void AddLodBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator);
        void AddMeshBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator, uint32_t indexOffset, uint32_t indexCount);
        bool AreAttributesValid() const;
        bool DoesMeshRequireFullRebuild(const WhiteBoxMeshAtomData& meshData) const;

        AZ::EntityId m_entityId;
        bool m_isPrimary = true; //!< Only the primary mesh owns the per-entity MeshHandleState bus.
        AZ::Data::AssetId m_materialAssetOverride; //!< External material asset to use instead of the default.
        AZ::Data::Asset<AZ::RPI::ModelLodAsset> m_lodAsset;
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        AZ::Data::Instance<AZ::RPI::Model> m_model;
        AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
        struct MaterialGroup
        {
            AZ::Data::AssetId m_assetId;
            AZ::u32 m_paintColor = 0;
            uint32_t m_indexOffset = 0;
            uint32_t m_indexCount = 0;
            AZ::Data::Instance<AZ::RPI::Material> m_instance;
            AZStd::shared_ptr<SharedMaterial> m_shared; //!< Keeps m_instance in the shared cache while this group uses it.
            AZ::u64 m_materialKey = 0;                  //!< What m_instance was made for; changes when the settings do.
            bool m_customMaterial = false;
            AZ::u32 m_stableId = 0; //!< Model slot id, hashed from material and paint colour so overrides survive edits.
        };
        AZStd::vector<MaterialGroup> m_materialGroups;
        AZStd::vector<AZ::u32> m_publishedSlots; //!< Sorted slot ids a Material component was last told about.
        AZStd::unordered_map<AZ::u32, const AZ::RPI::Material*> m_boundMaterials; //!< Instance each slot draws with.
        AZStd::unordered_set<AZ::u32> m_reloadRequested; //!< Slots already sent for a reload, so it is asked once.
        uint32_t m_vertexCount = 0;
        AZStd::shared_ptr<SharedModel> m_sharedModel; //!< Keeps the model alive while any White Box draws with it.
        AZStd::unique_ptr<IndexBuffer> m_indexBuffer;
        AZStd::array<
            AZStd::variant<
                AZStd::unique_ptr<PositionAttribute>, AZStd::unique_ptr<NormalAttribute>,
                AZStd::unique_ptr<TangentAttribute>, AZStd::unique_ptr<BitangentAttribute>,
                AZStd::unique_ptr<UVAttribute>, AZStd::unique_ptr<ColorAttribute>, AZStd::unique_ptr<LightmapUVAttribute>>,
            NumAttributes>
            m_attributes;
        bool m_visible = true;

        //! Default white box mesh material.
        static constexpr AZStd::string_view TexturedMaterialPath = "materials/whiteboxdefault.azmaterial";

        //! White box model name.
        static constexpr AZStd::string_view ModelName = "WhiteBoxMesh";
    };
} // namespace WhiteBox
