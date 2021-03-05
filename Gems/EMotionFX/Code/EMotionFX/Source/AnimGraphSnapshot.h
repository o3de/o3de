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
#include <MCore/Source/Attribute.h>
#include <EMotionFX/Source/AnimGraphNetworkSerializer.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>


namespace EMotionFX
{
    namespace Network
    {
        class AnimGraphReplicaChunk;
    }

    class AnimGraphInstance;
    class AnimGraphNode;

    using AttributeContainer = AZStd::vector<MCore::Attribute*>;
    using NodeIndexContainer = AZStd::vector<AZ::u32>;
    using MotionPlayTimeEntry = AZStd::pair<AZ::u32, float>;
    using MotionNodePlaytimeContainer = AZStd::vector<MotionPlayTimeEntry>; // Stores AnimGraphMotionNode index and currentPlaytime.

    class AnimGraphSnapshot
    {
    public:

        AnimGraphSnapshot(const AnimGraphInstance& instance, bool networkAuthoritative);
        ~AnimGraphSnapshot();

        bool IsNetworkAuthoritative() const { return m_networkAuthoritative; };
        void OnNetworkConnected(AnimGraphInstance& instance);

        void SetParameters(const AttributeContainer& attributes);
        const AttributeContainer& GetParameters() const;
        void SetActiveNodes(const NodeIndexContainer& activeNodes);
        const NodeIndexContainer& GetActiveNodes() const;
        void SetMotionNodePlaytimes(const MotionNodePlaytimeContainer& motionNodePlaytimes);
        const MotionNodePlaytimeContainer& GetMotionNodePlaytimes() const;

        // Function to collect snapshot data from animGraphInstance.
        void CollectAttributes(const AnimGraphInstance& instance);
        void CollectActiveNodes(AnimGraphInstance& instance);
        void CollectMotionNodePlaytimes(AnimGraphInstance& instance);

        // Function to restore snapshot data to animGraphInstance
        void RestoreAttributes(AnimGraphInstance& instance);
        void RestoreActiveNodes(AnimGraphInstance& instance);
        void RestoreMotionNodePlaytimes(AnimGraphInstance& instance);
        void Restore(AnimGraphInstance& instance);

        // Function to generate networking data from snapshot using serializer.
        void Serialize();

        void SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer);
        void SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer);

    private:

        enum LODFlag
        {
            Parameter           = 1 << 0,
            ActiveNodes         = 1 << 1,
            MotionPlaytimes     = 1 << 2
        };

        void Init(const AnimGraphInstance& instance);

        AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer>         m_bundleSerializer;
        AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer>    m_chunkSerializer;

        AttributeContainer                                              m_parameters;
        MotionNodePlaytimeContainer                                     m_motionNodePlaytimes;
        NodeIndexContainer                                              m_activeStateNodes;

        bool                                                            m_networkAuthoritative;         // When true, the snapshot will update with the animgraphinstance update call.
        AZ::u8                                                          m_dirtyFlag;                    // Controls the update for each lod.
        bool                                                            m_doFullRestore;                // When true, perform an initial sync in restore function.
    };
}