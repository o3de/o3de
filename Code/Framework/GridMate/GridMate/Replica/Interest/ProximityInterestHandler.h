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

#ifndef GM_REPLICA_PROXIMITYINTERESTHANDLER_H
#define GM_REPLICA_PROXIMITYINTERESTHANDLER_H

#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/Interest/RulesHandler.h>
#include <GridMate/Replica/Interest/BvDynamicTree.h>
#include <GridMate/Serialize/UtilityMarshal.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace GridMate
{
    class ProximityInterestHandler;
    class ProximityInterestAttribute;

    /*
    * Base interest
    */
    class ProximityInterest
    {
        friend class ProximityInterestHandler;

    public:
        const AZ::Aabb& Get() const { return m_bbox; }

    protected:
        explicit ProximityInterest(ProximityInterestHandler* handler);

        ProximityInterestHandler* m_handler;
        AZ::Aabb m_bbox;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Proximity rule
    */
    class ProximityInterestRule
        : public InterestRule
        , public ProximityInterest
    {
        friend class ProximityInterestHandler;

    public:
        using Ptr = AZStd::intrusive_ptr<ProximityInterestRule>;

        GM_CLASS_ALLOCATOR(ProximityInterestRule);

        void Set(const AZ::Aabb& bbox);

    private:

        // Intrusive ptr
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { --m_refCount; if (!m_refCount) Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        ProximityInterestRule(ProximityInterestHandler* handler, PeerId peerId, RuleNetworkId netId)
            : InterestRule(peerId, netId)
            , ProximityInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////

    class SpatialIndex
    {
    public:
        typedef Internal::DynamicTreeNode Node;

        class NodeCollector
        {
            typedef AZStd::vector<ProximityInterestAttribute*> Type;

        public:
            void Process(const Internal::DynamicTreeNode* node)
            {
                m_nodes.push_back(reinterpret_cast<ProximityInterestAttribute*>(node->m_data));
            }

            const Type& GetNodes() const
            {
                return m_nodes;
            }

        private:
            Type m_nodes;
        };

        SpatialIndex();
        ~SpatialIndex() = default;

        AZ_FORCE_INLINE void Remove(Node* node);
        AZ_FORCE_INLINE void Update(Node* node);
        AZ_FORCE_INLINE Node* Insert(const AZ::Aabb& get, ProximityInterestAttribute* attribute);
        AZ_FORCE_INLINE void Query(const AZ::Aabb& get, NodeCollector& nodes);

    private:
        AZStd::unique_ptr<BvDynamicTree> m_tree;
    };

    /*
    * Proximity attribute
    */
    class ProximityInterestAttribute
        : public InterestAttribute
        , public ProximityInterest
    {
        friend class ProximityInterestHandler;
        template<class T> friend class InterestPtr;

    public:
        using Ptr = AZStd::intrusive_ptr<ProximityInterestAttribute>;

        GM_CLASS_ALLOCATOR(ProximityInterestAttribute);

        void Set(const AZ::Aabb& bbox);

    private:

        // Intrusive ptr
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        ProximityInterestAttribute(ProximityInterestHandler* handler, ReplicaId repId)
            : InterestAttribute(repId)
            , ProximityInterest(handler)
            , m_worldNode(nullptr)
        {}

        void Destroy();

        void SetNode(SpatialIndex::Node* node) { m_worldNode = node; }
        SpatialIndex::Node* GetNode() const { return m_worldNode; }
        SpatialIndex::Node* m_worldNode; ///< non-owning pointer
    };
    ///////////////////////////////////////////////////////////////////////////

    class ProximityInterestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(ProximityInterestChunk);

        // ReplicaChunk
        typedef AZStd::intrusive_ptr<ProximityInterestChunk> Ptr;
        bool IsReplicaMigratable() override { return false; }
        bool IsBroadcast() override { return true; }
        static const char* GetChunkName() { return "ProximityInterestChunk"; }

        ProximityInterestChunk()
            : AddRuleRpc("AddRule")
            , RemoveRuleRpc("RemoveRule")
            , UpdateRuleRpc("UpdateRule")
            , AddRuleForPeerRpc("AddRuleForPeerRpc")
            , m_interestHandler(nullptr)
        {
        }

        void OnReplicaActivate(const ReplicaContext& rc) override;
        void OnReplicaDeactivate(const ReplicaContext& rc) override;

        bool AddRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext& ctx);
        bool RemoveRuleFn(RuleNetworkId netId, const RpcContext&);
        bool UpdateRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext&);
        bool AddRuleForPeerFn(RuleNetworkId netId, PeerId peerId, AZ::Aabb bbox, const RpcContext&);

        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleFn> AddRuleRpc;
        Rpc<RpcArg<RuleNetworkId>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::RemoveRuleFn> RemoveRuleRpc;
        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::UpdateRuleFn> UpdateRuleRpc;

        Rpc<RpcArg<RuleNetworkId>, RpcArg<PeerId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleForPeerFn> AddRuleForPeerRpc;

        unordered_map<RuleNetworkId, ProximityInterestRule::Ptr> m_rules;
        ProximityInterestHandler* m_interestHandler;
    };

    /*
    * Rules handler
    */
    class ProximityInterestHandler
        : public BaseRulesHandler
    {
        friend class ProximityInterestRule;
        friend class ProximityInterestAttribute;
        friend class ProximityInterestChunk;

    public:

        typedef unordered_set<ProximityInterestAttribute*> AttributeSet;
        typedef unordered_set<ProximityInterestRule*> RuleSet;

        GM_CLASS_ALLOCATOR(ProximityInterestHandler);

        ProximityInterestHandler();
        ~ProximityInterestHandler();

        /*
         * Creates new proximity rule and binds it to the peer.
         * Note: the lifetime of the created rule is tied to the lifetime of this handler.
         */
        ProximityInterestRule::Ptr CreateRule(PeerId peerId);

        /*
         * Creates new proximity attribute and binds it to the replica.
         * Note: the lifetime of the created attribute is tied to the lifetime of this handler.
         */
        ProximityInterestAttribute::Ptr CreateAttribute(ReplicaId replicaId);

        // Calculates rules and attributes matches
        void Update() override;

        // Returns last recalculated results
        const InterestMatchResult& GetLastResult() override;

        // Returns the manager it's bound to
        InterestManager* GetManager() override { return m_im; }

        // Rules that this handler is aware of
        const RuleSet& GetLocalRules() const { return m_localRules; }

    private:

        // BaseRulesHandler
        void OnRulesHandlerRegistered(InterestManager* manager) override;
        void OnRulesHandlerUnregistered(InterestManager* manager) override;

        void DestroyRule(ProximityInterestRule* rule);
        void FreeRule(ProximityInterestRule* rule);
        void UpdateRule(ProximityInterestRule* rule);

        void DestroyAttribute(ProximityInterestAttribute* attrib);
        void FreeAttribute(ProximityInterestAttribute* attrib);
        void UpdateAttribute(ProximityInterestAttribute* attrib);

        void OnNewRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer);
        void OnDeleteRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer);

        RuleNetworkId GetNewRuleNetId();

        ProximityInterestChunk* FindRulesChunkByPeerId(PeerId peerId);

        void DestroyAll();

        InterestManager* m_im;
        ReplicaManager* m_rm;

        AZ::u32 m_lastRuleNetId;

        unordered_map<PeerId, ProximityInterestChunk*> m_peerChunks;

        RuleSet m_localRules;
        RuleSet m_removedRules;
        RuleSet m_dirtyRules;

        AttributeSet m_attributes;
        AttributeSet m_removedAttributes;
        AttributeSet m_dirtyAttributes;

        ProximityInterestChunk* m_rulesReplica;

        // collection of all known attributes
        AZStd::unique_ptr<SpatialIndex> m_attributeWorld;

        InterestMatchResult m_resultCache;

        ///////////////////////////////////////////////////////////////////////////////////////////////////
        // internal processing helpers
        AZ_FORCE_INLINE RuleSet& GetAffectedRules();
        AZ_FORCE_INLINE void GetAttributesWithinRule(ProximityInterestRule* rule, SpatialIndex::NodeCollector& nodes);
        AZ_FORCE_INLINE void ClearDirtyState();

        AZ_FORCE_INLINE void CreateAndInsertIntoSpatialStructure(ProximityInterestAttribute* attribute);
        AZ_FORCE_INLINE void RemoveFromSpatialStructure(ProximityInterestAttribute* attribute);
        AZ_FORCE_INLINE void CreateAndInsertIntoSpatialStructure(ProximityInterestRule* rule);

        void UpdateInternal(InterestMatchResult& result);
        void CheckChangesForRule(ProximityInterestRule* rule, InterestMatchResult& result);
        void MarkAttributesDirtyInRule(ProximityInterestRule* rule);

        static bool HasSamePeers(const InterestPeerSet& one, const InterestPeerSet& another);
        void ProduceChanges(const InterestMatchResult& before, const InterestMatchResult& after);

        InterestMatchResult m_lastResult;
        ///////////////////////////////////////////////////////////////////////////////////////////////////
    };
    ///////////////////////////////////////////////////////////////////////////
}

#endif // GM_REPLICA_PROXIMITYINTERESTHANDLER_H
