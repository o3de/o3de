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

namespace AZ::RPI
{
    class Scene;
}

namespace AZ::Render
{
    //! Holds and manages an RHI DrawPacket for a specific mesh, and the resources that are needed to build and maintain it.
    //! @note This class is based on the meshDrawPacket class and could be paired down even further to leave only the pertinent
    //! parts of the interface and implementation.
    class EditorStateMeshDrawPacket
    {
    public:
        using ShaderList = AZStd::vector<Data::Instance<RPI::Shader>>;

        EditorStateMeshDrawPacket() = default;
        EditorStateMeshDrawPacket(
            RPI::ModelLod& modelLod,
            size_t modelLodMeshIndex,
            Data::Instance<RPI::Material> materialOverride,
            AZ::Name drawList,
            Data::Instance<RPI::ShaderResourceGroup> objectSrg,
            const RPI::MaterialModelUvOverrideMap& materialModelUvMap = {});

        AZ_DEFAULT_COPY(EditorStateMeshDrawPacket);
        AZ_DEFAULT_MOVE(EditorStateMeshDrawPacket);

        bool Update(const RPI::Scene& parentScene, bool forceUpdate = false);

        const RHI::DrawPacket* GetRHIDrawPacket() const;

        void SetStencilRef(uint8_t stencilRef) { m_stencilRef = stencilRef; }
        void SetSortKey(RHI::DrawItemSortKey sortKey) { m_sortKey = sortKey; };
        bool SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value);

        Data::Instance<RPI::Material> GetMaterial();

    private:
        bool DoUpdate(const RPI::Scene& parentScene);

        RPI::ConstPtr<RHI::DrawPacket> m_drawPacket;

        // Note, many of the following items are held locally in the EditorStateMeshDrawPacket solely to keep them resident in memory as long as they are needed
        // for the m_drawPacket. RHI::DrawPacket uses raw pointers only, but we use smart pointers here to hold on to the data.

        // Maintains references to the shader instances to keep their PSO caches resident (see Shader::Shutdown())
        ShaderList m_activeShaders;

        // The model that contains the mesh being represented by the DrawPacket
        Data::Instance<RPI::ModelLod> m_modelLod;

        // The index of the mesh within m_modelLod that is represented by the DrawPacket
        size_t m_modelLodMeshIndex;

        // The per-object shader resource group
        Data::Instance<RPI::ShaderResourceGroup> m_objectSrg;

        // We hold ConstPtr<RHI::SingleDeviceShaderResourceGroup> instead of Instance<RPI::ShaderResourceGroup> because the Material class
        // does not allow public access to its Instance<RPI::ShaderResourceGroup>.
        RPI::ConstPtr<RHI::SingleDeviceShaderResourceGroup> m_materialSrg;

        AZStd::fixed_vector<Data::Instance<RPI::ShaderResourceGroup>, RHI::DrawPacketBuilder::DrawItemCountMax> m_perDrawSrgs;

        // A reference to the material, used to rebuild the DrawPacket if needed
        Data::Instance<RPI::Material> m_material;

        // Tracks whether the Material has change since the DrawPacket was last built
        RPI::Material::ChangeId m_materialChangeId = RPI::Material::DEFAULT_CHANGE_ID;

        // Set the sort key for the draw packet
        RHI::DrawItemSortKey m_sortKey = 0;

        // Set the stencil value for this draw packet
        uint8_t m_stencilRef = 0;

        //! A map matches the index of UV names of this material to the custom names from the model.
        RPI::MaterialModelUvOverrideMap m_materialModelUvMap;

        //! List of shader options set for this specific draw packet
        typedef AZStd::pair<Name, RPI::ShaderOptionValue> ShaderOptionPair;
        typedef AZStd::vector<ShaderOptionPair> ShaderOptionVector;
        ShaderOptionVector m_shaderOptions;
        RHI::DrawListTag m_drawListTag;
    };
} // namespace AZ::Render
