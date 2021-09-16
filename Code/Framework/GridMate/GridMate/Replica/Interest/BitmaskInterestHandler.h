/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef GM_REPLICA_BITMASKINTERESTHANDLER_H
#define GM_REPLICA_BITMASKINTERESTHANDLER_H

#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/Interest/RulesHandler.h>
#include <GridMate/Serialize/UtilityMarshal.h>

#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/unordered_set.h>
#include <AzCore/std/containers/array.h>


namespace GridMate
{
    class BitmaskInterestHandler;
    using InterestBitmask = AZ::u32;

    /*
     * Base interest
    */
    class BitmaskInterest
    {
        friend class BitmaskInterestHandler;

    public:
        InterestBitmask Get() const { return m_bits; }

    protected:
        explicit BitmaskInterest(BitmaskInterestHandler* handler);

        BitmaskInterestHandler* m_handler;
        InterestBitmask m_bits;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Bitmask rule
    */
    class BitmaskInterestRule 
        : public InterestRule
        , public BitmaskInterest
    {
        friend class BitmaskInterestHandler;

    public:
        using Ptr = AZStd::intrusive_ptr<BitmaskInterestRule>;

        GM_CLASS_ALLOCATOR(BitmaskInterestRule);

        void Set(InterestBitmask newBitmask);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { --m_refCount; if (!m_refCount) Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        BitmaskInterestRule(BitmaskInterestHandler* handler, PeerId peerId, RuleNetworkId netId)
            : InterestRule(peerId, netId)
            , BitmaskInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Bitmask attribute
    */
    class BitmaskInterestAttribute
        : public InterestAttribute
        , public BitmaskInterest
    {
        friend class BitmaskInterestHandler;
        template<class T> friend class InterestPtr;

    public:
        using Ptr = AZStd::intrusive_ptr<BitmaskInterestAttribute>;

        GM_CLASS_ALLOCATOR(BitmaskInterestAttribute);

        void Set(InterestBitmask newBitmask);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        BitmaskInterestAttribute(BitmaskInterestHandler* handler, ReplicaId repId)
            : InterestAttribute(repId)
            , BitmaskInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////

    class BitmaskInterestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(BitmaskInterestChunk);

        BitmaskInterestChunk()
            : AddRuleRpc("AddRule")
            , RemoveRuleRpc("RemoveRule")
            , UpdateRuleRpc("UpdateRule")
            , AddRuleForPeerRpc("AddRuleForPeerRpc")
            , m_interestHandler(nullptr)
        {}

        typedef AZStd::intrusive_ptr<BitmaskInterestChunk> Ptr;
        bool IsReplicaMigratable() override { return false; }
        bool IsBroadcast() override { return true; }
        static const char* GetChunkName() { return "BitmaskInterestChunk"; }

        void OnReplicaActivate(const ReplicaContext& rc) override;
        void OnReplicaDeactivate(const ReplicaContext& rc) override;

        bool AddRuleFn(RuleNetworkId netId, InterestBitmask bits, const RpcContext& ctx);
        bool RemoveRuleFn(RuleNetworkId netId, const RpcContext&);
        bool UpdateRuleFn(RuleNetworkId netId, InterestBitmask bits, const RpcContext&);
        bool AddRuleForPeerFn(RuleNetworkId netId, PeerId peerId, InterestBitmask bitmask, const RpcContext&);

        Rpc<RpcArg<RuleNetworkId>, RpcArg<InterestBitmask>>::BindInterface<BitmaskInterestChunk, &BitmaskInterestChunk::AddRuleFn> AddRuleRpc;
        Rpc<RpcArg<RuleNetworkId>>::BindInterface<BitmaskInterestChunk, &BitmaskInterestChunk::RemoveRuleFn> RemoveRuleRpc;
        Rpc<RpcArg<RuleNetworkId>, RpcArg<InterestBitmask>>::BindInterface<BitmaskInterestChunk, &BitmaskInterestChunk::UpdateRuleFn> UpdateRuleRpc;

        Rpc<RpcArg<RuleNetworkId>, RpcArg<PeerId>, RpcArg<InterestBitmask>>::BindInterface<BitmaskInterestChunk, &BitmaskInterestChunk::AddRuleForPeerFn> AddRuleForPeerRpc;

        unordered_map<RuleNetworkId, BitmaskInterestRule::Ptr> m_rules;
        BitmaskInterestHandler* m_interestHandler;
    };

    /*
    * Rules handler
    */
    class BitmaskInterestHandler 
        : public BaseRulesHandler
    {
        friend class BitmaskInterestRule;
        friend class BitmaskInterestAttribute;
        friend class BitmaskInterestChunk;

    public:

        GM_CLASS_ALLOCATOR(BitmaskInterestHandler);

        BitmaskInterestHandler();

        // Creates new bitmask rule and binds it to the peer
        BitmaskInterestRule::Ptr CreateRule(PeerId peerId);

        // Creates new bitmask attribute and binds it to the replica
        BitmaskInterestAttribute::Ptr CreateAttribute(ReplicaId replicaId);

        // Calculates rules and attributes matches
        void Update() override;

        // Returns last recalculated results
        const InterestMatchResult& GetLastResult() override;

        InterestManager* GetManager() override { return m_im; }
    private:

        // BaseRulesHandler
        void OnRulesHandlerRegistered(InterestManager* manager) override;
        void OnRulesHandlerUnregistered(InterestManager* manager) override;

        void DestroyRule(BitmaskInterestRule* rule);
        void FreeRule(BitmaskInterestRule* rule);
        void UpdateRule(BitmaskInterestRule* rule);

        void DestroyAttribute(BitmaskInterestAttribute* attrib);
        void FreeAttribute(BitmaskInterestAttribute* attrib);
        void UpdateAttribute(BitmaskInterestAttribute* attrib);


        void OnNewRulesChunk(BitmaskInterestChunk::Ptr chunk, ReplicaPeer* peer);
        void OnDeleteRulesChunk(BitmaskInterestChunk::Ptr chunk, ReplicaPeer* peer);

        RuleNetworkId GetNewRuleNetId();

        BitmaskInterestChunk::Ptr FindRulesChunkByPeerId(PeerId peerId);

        typedef unordered_set<BitmaskInterestAttribute*> AttributeSet;
        typedef unordered_set<BitmaskInterestRule*> RuleSet;
        static const size_t k_numGroups = sizeof(InterestBitmask) * CHAR_BIT;

        InterestManager* m_im;
        ReplicaManager* m_rm;
        
        AZ::u32 m_lastRuleNetId;

        unordered_map<PeerId, BitmaskInterestChunk::Ptr> m_peerChunks;

        RuleSet m_localRules;

        AttributeSet m_dirtyAttributes;
        RuleSet m_dirtyRules;

        AZStd::array<AttributeSet, k_numGroups> m_attrGroups;
        AZStd::array<RuleSet, k_numGroups> m_ruleGroups;

        InterestMatchResult m_resultCache;

        BitmaskInterestChunk* m_rulesReplica;

        AttributeSet m_attrs;
        RuleSet m_rules;
    };
    ///////////////////////////////////////////////////////////////////////////
}

#endif
