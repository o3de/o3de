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

    //! One skinning influence of a vertex.
    struct SkinningInfluence
    {
        //! Weight of the joint that influence the vertex.
        float m_jointWeight;

        //! Index of the joint that influences the vertex.
        AZ::u16 m_jointIndex;
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
            const size_t numVertices,
            const size_t numSimulatedVertices,
            const AZStd::vector<int>& meshRemappedVertices);

        explicit ActorClothSkinning(AZ::EntityId entityId);

        //! Updates skinning with the current pose of the actor.
        virtual void UpdateSkinning() = 0;

        //! Applies skinning to a list of positions.
        //! @note w components are not affected.
        virtual void ApplySkinning(
            const AZStd::vector<AZ::Vector4>& originalPositions, 
            AZStd::vector<AZ::Vector4>& positions) = 0;

        //! Applies skinning to a list of positions and vectors whose vertices
        //! have not been used for simulation.
        virtual void ApplySkinninOnNonSimulatedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData) = 0;

        //! Updates visibility variables.
        void UpdateActorVisibility();

        //! Returns true if actor is currently visible on screen.
        bool IsActorVisible() const;

        //! Returns true if actor was visible on screen in previous update.
        bool WasActorVisible() const;

    protected:
        AZ::EntityId m_entityId;

        size_t m_numberOfInfluencesPerVertex = 0;

        // Skinning influences of all vertices
        AZStd::vector<SkinningInfluence> m_skinningInfluences;

        // Indices to skinning influences that are part of the simulation
        AZStd::vector<AZ::u32> m_simulatedVertices;

        // Indices to skinning influences that are not part of the simulation
        AZStd::vector<AZ::u32> m_nonSimulatedVertices;

        // Collection of skeleton joint indices that influence the vertices
        AZStd::vector<AZ::u16> m_jointIndices;

        // Visibility variables
        bool m_wasActorVisible = false;
        bool m_isActorVisible = false;
    };
}// namespace NvCloth
