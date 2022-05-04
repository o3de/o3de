/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Reflect/Model/ModelLodIndex.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/AtomMeshBus.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        class FocusedEntity
            : private AZ::Render::AtomMeshNotificationBus::Handler
        {
        public:
            FocusedEntity(EntityId entityId, Data::Instance<RPI::Material> maskMaterial);

            //FocusedEntity(const FocusedEntity& other);
            //FocusedEntity(FocusedEntity&& other);

            ~FocusedEntity();

            bool CanDraw() const;
            void Draw();

        private:
            RPI::ModelLodIndex GetModelLodIndex(const RPI::ViewPtr view, Data::Instance<RPI::Model> model) const;

            //AtomMeshNotificationBus overrides ...
            void OnAcquireMesh(const MeshFeatureProcessorInterface::MeshHandle* meshHandle) override;

            void CreateOrUpdateMeshDrawPackets(
                const MeshFeatureProcessorInterface* featureProcessor,
                const RPI::ModelLodIndex modelLodIndex,
                Data::Instance<RPI::Model> model);

            void ClearDrawData();

            void AtomBusConnect();
            //void AtomBusSoftConnect();
            void AtomBusDisconnect();

            EntityId m_entityId;
            const MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;
            Data::Instance<RPI::Material> m_maskMaterial = nullptr;
            RPI::ModelLodIndex m_modelLodIndex = RPI::ModelLodIndex::Null;
            AZStd::vector<RPI::MeshDrawPacket> m_meshDrawPackets;
            //bool m_consumeMeshAcquisition = true;
        };
    } // namespace Render
} // namespace AZ
