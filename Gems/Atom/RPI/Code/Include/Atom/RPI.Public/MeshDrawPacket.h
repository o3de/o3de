/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DrawPacketBuilder.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/std/containers/fixed_vector.h>

// Enable this define to print the shader variants used by MeshDrawPacket every time the draw packet get rebuilt.
// Note: the log can be extremely long if there are too many mesh instances (for example, >5K).  
// #define DEBUG_MESH_SHADERVARIANTS

namespace AZ
{
    namespace RHI
    {
        class ConstantsLayout;
    }

    namespace RPI
    {
        class Scene;

        //! Holds and manages an RHI DrawPacket for a specific mesh, and the resources that are needed to build and maintain it.
        class MeshDrawPacket
        {
        public:
            struct ShaderData
            {
                Data::Instance<Shader> m_shader;
                Name m_materialPipelineName;
                Name m_shaderTag;
                ShaderVariantId m_requestedShaderVariantId;
                ShaderVariantId m_activeShaderVariantId;
                ShaderVariantStableId m_activeShaderVariantStableId;
            };

            using ShaderList = AZStd::vector<ShaderData>;

            MeshDrawPacket() = default;
            MeshDrawPacket(
                ModelLod& modelLod,
                size_t modelLodMeshIndex,
                Data::Instance<Material> materialOverride,
                Data::Instance<ShaderResourceGroup> objectSrg,
                const MaterialModelUvOverrideMap& materialModelUvMap = {});

            AZ_DEFAULT_COPY(MeshDrawPacket);
            AZ_DEFAULT_MOVE(MeshDrawPacket);

            bool Update(const Scene& parentScene, bool forceUpdate = false);

            RHI::DrawPacket* GetRHIDrawPacket() { return m_drawPacket.get(); }
            const RHI::DrawPacket* GetRHIDrawPacket() const { return m_drawPacket.get(); }
            const RHI::ConstPtr<RHI::ConstantsLayout> GetRootConstantsLayout() const;

            void SetStencilRef(uint8_t stencilRef);
            void SetSortKey(RHI::DrawItemSortKey sortKey);
            bool SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value);
            bool UnsetShaderOption(const Name& shaderOptionName);
            void ClearShaderOptions();

            // Enable/disable draw filter for a specific draw list tag.
            // If disabled, any draw items with this drawListTag won't be added to the DrawPacket when updated
            void SetEnableDraw(RHI::DrawListTag drawListTag, bool enableDraw);
            RHI::DrawListMask GetDrawListFilter();
            // Remove the draw list filter and enable render for all draw items
            void ClearDrawListFilter();

            Data::Instance<Material> GetMaterial() const;
            const ModelLod::Mesh& GetMesh() const;
            const ShaderList& GetActiveShaderList() const { return m_activeShaders; }

            void DebugOutputShaderVariants();

        private:
            bool DoUpdate(const Scene& parentScene);
            void ForValidShaderOptionName(const Name& shaderOptionName, const AZStd::function<bool(const ShaderCollection::Item&, ShaderOptionIndex)>& callback);

            Ptr<RHI::DrawPacket> m_drawPacket;

            // Note, many of the following items are held locally in the MeshDrawPacket solely to keep them resident in memory as long as they are needed
            // for the m_drawPacket. RHI::DrawPacket uses raw pointers only, but we use smart pointers here to hold on to the data.

            // Maintains references to the shader instances to keep their PSO caches resident (see Shader::Shutdown())
            ShaderList m_activeShaders;

            RHI::ConstPtr<RHI::ConstantsLayout> m_rootConstantsLayout;

            // The model that contains the mesh being represented by the DrawPacket
            Data::Instance<ModelLod> m_modelLod;

            // The index of the mesh within m_modelLod that is represented by the DrawPacket
            size_t m_modelLodMeshIndex;

            // The per-object shader resource group
            Data::Instance<ShaderResourceGroup> m_objectSrg;

            // We hold ConstPtr<RHI::ShaderResourceGroup> instead of Instance<RPI::ShaderResourceGroup> because the Material class
            // does not allow public access to its Instance<RPI::ShaderResourceGroup>.
            ConstPtr<RHI::ShaderResourceGroup> m_materialSrg;

            AZStd::fixed_vector<Data::Instance<ShaderResourceGroup>, RHI::DrawPacketBuilder::DrawItemCountMax> m_perDrawSrgs;

            // A reference to the material, used to rebuild the DrawPacket if needed
            Data::Instance<Material> m_material;

            // Tracks whether the Material has change since the DrawPacket was last built
            Material::ChangeId m_materialChangeId = Material::DEFAULT_CHANGE_ID;

            // A handler which is called when a shader variant of the material is ready 
            Material::OnMaterialShaderVariantReadyEvent::Handler m_shaderVariantHandler;

            // Set the sort key for the draw packet
            RHI::DrawItemSortKey m_sortKey = 0;

            // Set the stencil value for this draw packet
            uint8_t m_stencilRef = 0;

            //! A map matches the index of UV names of this material to the custom names from the model.
            MaterialModelUvOverrideMap m_materialModelUvMap;

            //! List of shader options set for this specific draw packet
            typedef AZStd::pair<Name, RPI::ShaderOptionValue> ShaderOptionPair;
            typedef AZStd::vector<ShaderOptionPair> ShaderOptionVector;
            ShaderOptionVector m_shaderOptions;

            //! A draw list mask which is used to filter draw items which are packed into the DrawPacket
            RHI::DrawListMask m_drawListFilter;

            //! A flag to indicate if the DrawPacket need to be rebuild when updating
            bool m_needUpdate = true;

#ifdef DEBUG_MESH_SHADERVARIANTS
            // For debug shader variants
            // The list of shader variant asset names used by the DrawPackets
            AZStd::vector<AZStd::string_view> m_shaderVariantNames;
#endif
        };
        
        using MeshDrawPacketList = AZStd::vector<RPI::MeshDrawPacket>;
        using MeshDrawPacketLods = AZStd::fixed_vector<MeshDrawPacketList, RPI::ModelLodAsset::LodCountMax>;

    } // namespace RPI
} // namespace AZ
