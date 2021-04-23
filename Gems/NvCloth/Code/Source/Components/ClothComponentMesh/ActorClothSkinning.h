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

#include <AzCore/Component/Entity.h>

#include <NvCloth/Types.h>

#include <Components/ClothComponentMesh/ClothComponentMesh.h>

namespace NvCloth
{
    struct MeshNodeInfo;

    //! Skinning information of a particle.
    struct SkinningInfo
    {
        //! Weights of each joint that influence the particle.
        AZStd::vector<float> m_jointWeights;

        //! List of joints that influence the particle.
        AZStd::vector<AZ::u16> m_jointIndices;
    };

    //! Class to retrieve skinning information from an actor on the same entity
    //! and use that data to apply skinning to vertices.
    class ActorClothSkinning
    {
    public:
        AZ_TYPE_INFO(ActorClothSkinning, "{3E7C664D-096B-4126-8553-3241BA965533}");

        virtual ~ActorClothSkinning() = default;

        static AZStd::unique_ptr<ActorClothSkinning> Create(
            AZ::EntityId entityId, 
            const MeshNodeInfo& meshNodeInfo,
            const size_t numSimParticles);

        explicit ActorClothSkinning(AZ::EntityId entityId);

        //! Updates skinning with the current pose of the actor.
        virtual void UpdateSkinning() = 0;

        //! Applies skinning to a list of positions.
        //! @note w components are not affected.
        void ApplySkinning(
            const AZStd::vector<AZ::Vector4>& originalPositions, 
            AZStd::vector<AZ::Vector4>& positions,
            const AZStd::vector<int>& meshRemappedVertices);

        //! Applies skinning to a list of positions and vectors whose vertices
        //! have not been used for simulation (remapped index is negative).
        void ApplySkinninOnRemovedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData,
            const AZStd::vector<int>& meshRemappedVertices);

        //! Updates visibility variables.
        void UpdateActorVisibility();

        //! Returns true if actor is currently visible on screen.
        bool IsActorVisible() const;

        //! Returns true if actor was visible on screen in previous update.
        bool WasActorVisible() const;

    protected:
        //! Returns true if it has valid skinning pose of the actor.
        virtual bool HasSkinningPoseData() = 0;

        //! Computes vertex skinning
        virtual AZ::Vector4 ComputeSkinning(
            const AZ::Vector4& original,
            const SkinningInfo& skinningInfo) = 0;

        AZ::EntityId m_entityId;

        // Skinning information of all particles
        AZStd::vector<SkinningInfo> m_skinningData;

        // Collection of skeleton joint indices that influence the particles
        AZStd::vector<AZ::u16> m_jointIndices;

        // Visibility variables
        bool m_wasActorVisible = false;
        bool m_isActorVisible = false;
    };
}// namespace NvCloth
