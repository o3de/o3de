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

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DrawPacketBuilder.h>

#include <AzCore/Math/Obb.h>


namespace AZ
{
    namespace RPI
    {
        class Scene;

        //! Holds and manages an RHI DrawPacket for a specific mesh, and the resources that are needed to build and maintain it.
        class MeshDrawPacket
        {
        public:
            using ShaderList = AZStd::vector<Data::Instance<Shader>>;

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

            const RHI::DrawPacket* GetRHIDrawPacket() const;

            void SetStencilRef(uint8_t stencilRef) { m_stencilRef = stencilRef; }
            void SetSortKey(RHI::DrawItemSortKey sortKey) { m_sortKey = sortKey; };

            Data::Instance<Material> GetMaterial();

        private:
            bool DoUpdate(const Scene& parentScene);

            ConstPtr<RHI::DrawPacket> m_drawPacket;

            // Note, many of the following items are held locally in the MeshDrawPacket solely to keep them resident in memory as long as they are needed
            // for the m_drawPacket. RHI::DrawPacket uses raw pointers only, but we use smart pointers here to hold on to the data.

            // Maintains references to the shader instances to keep their PSO caches resident (see Shader::Shutdown())
            ShaderList m_activeShaders;

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

            // Set the sort key for the draw packet
            RHI::DrawItemSortKey m_sortKey = 0;

            // Set the stencil value for this draw packet
            uint8_t m_stencilRef = 0;

            //! A map matches the index of UV names of this material to the custom names from the model.
            MaterialModelUvOverrideMap m_materialModelUvMap;
        };
    } // namespace RPI
} // namespace AZ
