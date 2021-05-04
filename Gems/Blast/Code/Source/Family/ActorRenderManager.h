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

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <AzCore/std/containers/vector.h>
#include <Azcore/Math/Vector3.h>

namespace Blast
{
    class BlastMeshData;
    class BlastActor;

    //! Responsible for synchronizing render meshes of BlastFamily to corresponding BlastActors.
    //! Ideally we would want to just have meshes directly on the BlastActor,
    //! but that was impossible with LmbrCentral MeshComponent which cannot hold several
    //! meshes and several MeshComponents cannot exist on the same entity.
    //! It is possible to do with Atom and should be addressed in SPEC-3880
    class ActorRenderManager
    {
    public:
        // Initializes the manager by creating an entity with a render mesh for each chunk.
        // Initially all meshes are invisible.
        ActorRenderManager(
            AZ::Render::MeshFeatureProcessorInterface* meshFeatureProcessor, BlastMeshData* meshData,
            AZ::EntityId entityId, uint32_t chunkCount, const AZ::Vector3& scale);

        // Callback that makes meshes corresponding to the actor visible and follows it's transform.
        void OnActorCreated(const BlastActor& actor);

        // Callback that makes meshes corresponding to the actor invisible.
        void OnActorDestroyed(const BlastActor& actor);

        // Update positions of entities with render meshes corresponding to their right dynamic bodies.
        void SyncMeshes();

    protected:
        BlastMeshData* m_meshData;
        AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

        AZStd::vector<AZ::Render::MeshFeatureProcessorInterface::MeshHandle> m_chunkMeshHandles;
        AZStd::vector<const BlastActor*> m_chunkActors;

        uint32_t m_chunkCount;
        AZ::Vector3 m_scale;
        AZ::Render::MaterialAssignmentMap m_materialMap;
    };
} // namespace Blast
