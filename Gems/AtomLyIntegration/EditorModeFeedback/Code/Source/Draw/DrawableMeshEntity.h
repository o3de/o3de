/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/EditorStateMeshDrawPacket.h>

#include <Atom/RPI.Reflect/Model/ModelLodIndex.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Render
{
    //! Representation of a focused entity's Atom mesh (if any).
    //! @note It is not an error for an entity to not have any Atom mesh.
    class DrawableMeshEntity
        : private AZ::Render::MeshHandleStateNotificationBus::Handler
    {
    public:
        DrawableMeshEntity(EntityId entityId, Data::Instance<RPI::Material> maskMaterial, Name drawList);
        ~DrawableMeshEntity();

        //! Returns true if this entity can be drawn, otherwise false.
        bool CanDraw() const;

        //! Draws the entity's Atom mesh.
        void Draw();

    private:

        //! Retrieves the levol of detail index for this entity's Atom mesh.
        RPI::ModelLodIndex GetModelLodIndex(const RPI::ViewPtr view, Data::Instance<RPI::Model> model) const;

        // MeshHandleStateNotificationBus overrides ...
        void OnMeshHandleSet(const MeshFeatureProcessorInterface::MeshHandle* meshHandle) override;

        //! Builds the entity's drawable mesh data from scratch, overwriting any existing data.
        void CreateOrUpdateMeshDrawPackets(
            const MeshFeatureProcessorInterface* featureProcessor,
            const RPI::ModelLodIndex modelLodIndex,
            Data::Instance<RPI::Model> model);

        //! Clears the entity's mesh draw packets and other draw state.
        void ClearDrawData();

        // Builds the mesh draw packets for the Atom mesh.
        void BuildMeshDrawPackets(
            const Data::Asset<RPI::ModelAsset> modelAsset, Data::Instance<RPI::ShaderResourceGroup> meshObjectSrg);

        // Creates the mask shader resource group for the Atom mesh.
        Data::Instance<RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(
            const MeshFeatureProcessorInterface* featureProcessor) const;

        EntityId m_entityId;
        const MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;
        Data::Instance<RPI::Material> m_maskMaterial = nullptr;
        Name m_drawList;
        RPI::ModelLodIndex m_modelLodIndex = RPI::ModelLodIndex::Null;
        AZStd::vector<EditorStateMeshDrawPacket> m_meshDrawPackets;
    };
} // namespace AZ::Render
