/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>
#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace Render
    {
        struct MorphTargetInstanceMetaData;

        using SkinnedMeshOutputVertexOffsets = AZStd::array<uint32_t, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)>;
        using SkinnedMeshOutputVertexCounts  = AZStd::array<uint32_t, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)>;

        //! SkinnedMeshInstance contains the data that is needed to represent the output from skinning a single instance of a skinned mesh.
        //! It does not contain the actual skinned vertex data, but rather views into the buffers that do contain the data, which are owned by the SkinnedMeshOutputStreamManager
        class SkinnedMeshInstance
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshInstance, AZ::SystemAllocator);

            //! The target model, which is used by the MeshFeatureProcessor to render the mesh
            Data::Instance<RPI::Model> m_model;

            //! Offsets into the skinned vertex data which are used by SkinnedMeshDispatchItem to target the correct location to store the skinning results
            AZStd::vector<AZStd::vector<SkinnedMeshOutputVertexOffsets>> m_outputStreamOffsetsInBytes;
            
            //! Offsets to the start of the position history buffer for each mesh
            AZStd::vector<AZStd::vector<uint32_t>> m_positionHistoryBufferOffsetsInBytes;

            //! Virtual addresses that represent the location of the data within the skinned mesh output stream.
            //! When they are released, they automatically mark the memory as freed so the SkinnedMeshOutputStreamManager can re-purpose the memory
            AZStd::vector<AZStd::vector<AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation>>> m_allocations;

            //! Offsets into the output stream buffer to a location that contains accumulated morph target deltas from the morph pass. One offset per-lod.
            //! Set to MorphTargetConstants::s_invalidDeltaOffset if there are no morph targets for the lod
            AZStd::vector<AZStd::vector<MorphTargetInstanceMetaData>> m_morphTargetInstanceMetaData;

            //! Meshes that have no influences or are skinned by another system (cloth) should be skipped
            AZStd::vector<AZStd::vector<bool>> m_isSkinningEnabled;

            //! Typically, when a SkinnedMeshInstance goes out of scope and the memory is freed, the SkinnedMeshOutputStreamManager will signal an event indicating more memory is available
            //! If the creation of a SkinnedMeshInstance fails part way through after some memory has already been allocated,
            //! calling SupressSignalOnDeallocate before releasing the SkinnedMeshInstance will prevent this event since there is not really any new memory available that wasn't available before
            void SuppressSignalOnDeallocate();

            //! Set a flag to skip skinning for a particular mesh
            void DisableSkinning(uint32_t lodIndex, uint32_t meshIndex);

            //! Set a flag to enable skinning for a particular mesh
            void EnableSkinning(uint32_t lodIndex, uint32_t meshIndex);

            //! Returns true if skinning should be executed for this mesh
            bool IsSkinningEnabled(uint32_t lodIndex, uint32_t meshIndex) const;
        };
    }// Render
}// AZ
