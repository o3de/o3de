/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshInfo.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
{
    class MeshInfoManager
    {
    public:
        MeshInfoManager();
        void Activate(RPI::Scene* scene);
        void Deactivate();

        // integration with the MeshFeatureProcessor
        MeshInfoHandle AcquireMeshInfoEntry();
        void ReleaseMeshInfoEntry(const MeshInfoHandle handle);
        const RHI::Ptr<MeshInfoEntry>& GetMeshInfoEntry(const MeshInfoHandle handle) const;
        void UpdateMeshInfoEntry(const MeshInfoHandle handle, AZStd::function<bool(MeshInfoEntry*)> updateFunction);
        const Data::Instance<RPI::Buffer>& GetMeshInfoBuffer() const;

        void UpdateMeshInfoBuffer();

        int32_t GetMaxMeshInfoIndex()
        {
            return m_meshInfoIndices.MaxCount();
        }

        // utility function to initialize the buffer indices from default models
        static void InitMeshInfoGeometryBuffers(
            const AZ::RPI::Model* model,
            const size_t lod,
            const size_t meshIndex,
            const RPI::Material* material,
            const RPI::MaterialModelUvOverrideMap& uvMapping,
            MeshInfoEntry* entry);

    private:
        bool m_isEnabled = false;

        RPI::SceneId m_sceneId;

        RHI::Ptr<MeshInfoEntry> m_emptyEntry{ nullptr };
        RPI::PersistentIndexAllocator<int32_t> m_meshInfoIndices;

        AZStd::vector<RHI::Ptr<MeshInfoEntry>> m_meshInfoData;
        RPI::RingBuffer m_meshInfoBuffer;

        bool m_meshInfoNeedsUpdate = false;

        RHI::ShaderInputNameIndex m_meshInfoIndex = "m_meshInfo";
        RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
    };

} // namespace AZ::Render