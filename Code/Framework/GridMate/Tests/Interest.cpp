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
#include "Tests.h"
#include <GridMate_Traits_Platform.h>

#include <GridMate/Session/LANSession.h>

#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/BitmaskInterestHandler.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using namespace GridMate;

// An easy switch to use an old brute force ProximityInterestHandler for performance comparison.
#if 0
namespace GridMate
{
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
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


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

        // Creates new proximity rule and binds it to the peer
        ProximityInterestRule::Ptr CreateRule(PeerId peerId);

        // Creates new proximity attribute and binds it to the replica
        ProximityInterestAttribute::Ptr CreateAttribute(ReplicaId replicaId);

        // Calculates rules and attributes matches
        void Update() override;

        // Returns last recalculated results
        const InterestMatchResult& GetLastResult() override;

        // Returns manager its bound with
        InterestManager* GetManager() override { return m_im; }

        const RuleSet& GetLocalRules() { return m_localRules; }

    private:
        void UpdateInternal(InterestMatchResult& result);

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

        InterestManager* m_im;
        ReplicaManager* m_rm;

        AZ::u32 m_lastRuleNetId;

        unordered_map<PeerId, ProximityInterestChunk*> m_peerChunks;
        RuleSet m_localRules;

        AttributeSet m_attributes;
        RuleSet m_rules;

        ProximityInterestChunk* m_rulesReplica;

        InterestMatchResult m_resultCache;
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
        static const char* GetChunkName() { return "ProximityInterestChunk"; }
        bool IsBroadcast() { return true; }
        ///////////////////////////////////////////////////////////////////////////


        ProximityInterestChunk()
            : m_interestHandler(nullptr)
            , AddRuleRpc("AddRule")
            , RemoveRuleRpc("RemoveRule")
            , UpdateRuleRpc("UpdateRule")
            , AddRuleForPeerRpc("AddRuleForPeerRpc")
        {

        }

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            m_interestHandler = static_cast<ProximityInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4)));
            AZ_Assert(m_interestHandler, "No proximity interest handler in the user context");

            if (m_interestHandler)
            {
                m_interestHandler->OnNewRulesChunk(this, rc.m_peer);
            }
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            if (rc.m_peer && m_interestHandler)
            {
                m_interestHandler->OnDeleteRulesChunk(this, rc.m_peer);
            }
        }

        bool AddRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext& ctx)
        {
            if (IsProxy())
            {
                auto rulePtr = m_interestHandler->CreateRule(ctx.m_sourcePeer);
                rulePtr->Set(bbox);
                m_rules.insert(AZStd::make_pair(netId, rulePtr));
            }

            return true;
        }

        bool RemoveRuleFn(RuleNetworkId netId, const RpcContext&)
        {
            if (IsProxy())
            {
                m_rules.erase(netId);
            }

            return true;
        }

        bool UpdateRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext&)
        {
            if (IsProxy())
            {
                auto it = m_rules.find(netId);
                if (it != m_rules.end())
                {
                    it->second->Set(bbox);
                }
            }

            return true;
        }

        bool AddRuleForPeerFn(RuleNetworkId netId, PeerId peerId, AZ::Aabb bbox, const RpcContext&)
        {
            ProximityInterestChunk* peerChunk = m_interestHandler->FindRulesChunkByPeerId(peerId);
            if (peerChunk)
            {
                auto it = peerChunk->m_rules.find(netId);
                if (it == peerChunk->m_rules.end())
                {
                    auto rulePtr = m_interestHandler->CreateRule(peerId);
                    peerChunk->m_rules.insert(AZStd::make_pair(netId, rulePtr));
                    rulePtr->Set(bbox);
                }
            }
            return false;
        }

        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleFn> AddRuleRpc;
        Rpc<RpcArg<RuleNetworkId>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::RemoveRuleFn> RemoveRuleRpc;
        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::UpdateRuleFn> UpdateRuleRpc;

        Rpc<RpcArg<RuleNetworkId>, RpcArg<PeerId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleForPeerFn> AddRuleForPeerRpc;

        ProximityInterestHandler* m_interestHandler;
        unordered_map<RuleNetworkId, ProximityInterestRule::Ptr> m_rules;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterest
    */
    ProximityInterest::ProximityInterest(ProximityInterestHandler* handler)
        : m_bbox(AZ::Aabb::CreateNull())
        , m_handler(handler)
    {
        AZ_Assert(m_handler, "Invalid interest handler");
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestRule
    */
    void ProximityInterestRule::Set(const AZ::Aabb& bbox)
    {
        m_bbox = bbox;
        m_handler->UpdateRule(this);
    }

    void ProximityInterestRule::Destroy()
    {
        m_handler->DestroyRule(this);
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestAttribute
    */
    void ProximityInterestAttribute::Set(const AZ::Aabb& bbox)
    {
        m_bbox = bbox;
        m_handler->UpdateAttribute(this);
    }

    void ProximityInterestAttribute::Destroy()
    {
        m_handler->DestroyAttribute(this);
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestHandler
    */
    ProximityInterestHandler::ProximityInterestHandler()
        : m_im(nullptr)
        , m_rm(nullptr)
        , m_lastRuleNetId(0)
        , m_rulesReplica(nullptr)
    {
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ProximityInterestChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<ProximityInterestChunk>();
        }
    }

    ProximityInterestRule::Ptr ProximityInterestHandler::CreateRule(PeerId peerId)
    {
        ProximityInterestRule* rulePtr = aznew ProximityInterestRule(this, peerId, GetNewRuleNetId());
        if (peerId == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->AddRuleRpc(rulePtr->GetNetworkId(), rulePtr->Get());
            m_localRules.insert(rulePtr);
        }

        return rulePtr;
    }

    void ProximityInterestHandler::FreeRule(ProximityInterestRule* rule)
    {
        //TODO: should be pool-allocated
        delete rule;
    }

    void ProximityInterestHandler::DestroyRule(ProximityInterestRule* rule)
    {
        if (m_rm && rule->GetPeerId() == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->RemoveRuleRpc(rule->GetNetworkId());
        }

        rule->m_bbox = AZ::Aabb::CreateNull();
        m_rules.insert(rule);
        m_localRules.erase(rule);
    }

    void ProximityInterestHandler::UpdateRule(ProximityInterestRule* rule)
    {
        if (rule->GetPeerId() == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->UpdateRuleRpc(rule->GetNetworkId(), rule->Get());
        }

        m_rules.insert(rule);
    }

    ProximityInterestAttribute::Ptr ProximityInterestHandler::CreateAttribute(ReplicaId replicaId)
    {
        return aznew ProximityInterestAttribute(this, replicaId);
    }

    void ProximityInterestHandler::FreeAttribute(ProximityInterestAttribute* attrib)
    {
        //TODO: should be pool-allocated
        delete attrib;
    }

    void ProximityInterestHandler::DestroyAttribute(ProximityInterestAttribute* attrib)
    {
        attrib->m_bbox = AZ::Aabb::CreateNull();
        m_attributes.insert(attrib);
    }

    void ProximityInterestHandler::UpdateAttribute(ProximityInterestAttribute* attrib)
    {
        m_attributes.insert(attrib);
    }

    void ProximityInterestHandler::OnNewRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer)
    {
        if (chunk != m_rulesReplica) // non-local
        {
            m_peerChunks.insert(AZStd::make_pair(peer->GetId(), chunk));

            for (auto& rule : m_localRules)
            {
                chunk->AddRuleForPeerRpc(rule->GetNetworkId(), rule->GetPeerId(), rule->Get());
            }
        }
    }

    void ProximityInterestHandler::OnDeleteRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer)
    {
        (void)chunk;
        m_peerChunks.erase(peer->GetId());
    }

    RuleNetworkId ProximityInterestHandler::GetNewRuleNetId()
    {
        ++m_lastRuleNetId;
        return m_rulesReplica->GetReplicaId() | (static_cast<AZ::u64>(m_lastRuleNetId) << 32);
    }

    ProximityInterestChunk* ProximityInterestHandler::FindRulesChunkByPeerId(PeerId peerId)
    {
        auto it = m_peerChunks.find(peerId);
        if (it == m_peerChunks.end())
        {
            return nullptr;
        }
        else
        {
            return it->second;
        }
    }

    const InterestMatchResult& ProximityInterestHandler::GetLastResult()
    {
        return m_resultCache;
    }

    void ProximityInterestHandler::UpdateInternal(InterestMatchResult& result)
    {
        //////////////////////////////////////////////
        // just recalculating the whole state for now
        for (auto attrIt = m_attributes.begin(); attrIt != m_attributes.end(); )
        {
            ProximityInterestAttribute* attr = *attrIt;

            auto resultIt = result.insert(attr->GetReplicaId());
            for (auto ruleIt = m_rules.begin(); ruleIt != m_rules.end(); ++ruleIt)
            {
                ProximityInterestRule* rule = *ruleIt;
                if (rule->m_bbox.Overlaps(attr->m_bbox))
                {
                    resultIt.first->second.insert(rule->GetPeerId());
                }
            }

            if ((*attrIt)->IsDeleted())
            {
                attrIt = m_attributes.erase(attrIt);
                delete attr;
            }
            else
            {
                ++attrIt;
            }
        }

        for (auto ruleIt = m_rules.begin(); ruleIt != m_rules.end(); )
        {
            ProximityInterestRule* rule = *ruleIt;

            if (rule->IsDeleted())
            {
                ruleIt = m_rules.erase(ruleIt);
                delete rule;
            }
            else
            {
                ++ruleIt;
            }
        }
        //////////////////////////////////////////////
    }

    void ProximityInterestHandler::Update()
    {
        m_resultCache.clear();

        UpdateInternal(m_resultCache);
    }

    void ProximityInterestHandler::OnRulesHandlerRegistered(InterestManager* manager)
    {
        AZ_Assert(m_im == nullptr, "Handler is already registered with manager %p (%p)\n", m_im, manager);
        AZ_Assert(m_rulesReplica == nullptr, "Rules replica is already created\n");
        AZ_TracePrintf("GridMate", "Proximity interest handler is registered\n");
        m_im = manager;
        m_rm = m_im->GetReplicaManager();
        m_rm->RegisterUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4), this);

        auto replica = Replica::CreateReplica("ProximityInterestHandlerRules");
        m_rulesReplica = CreateAndAttachReplicaChunk<ProximityInterestChunk>(replica);
        m_rm->AddMaster(replica);
    }

    void ProximityInterestHandler::OnRulesHandlerUnregistered(InterestManager* manager)
    {
        (void)manager;
        AZ_Assert(m_im == manager, "Handler was not registered with manager %p (%p)\n", manager, m_im);
        AZ_TracePrintf("GridMate", "Proximity interest handler is unregistered\n");
        m_rulesReplica = nullptr;
        m_im = nullptr;
        m_rm->UnregisterUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4));
        m_rm = nullptr;

        for (auto& chunk : m_peerChunks)
        {
            chunk.second->m_interestHandler = nullptr;
        }
        m_peerChunks.clear();
        m_localRules.clear();

        for (ProximityInterestRule* rule : m_rules)
        {
            delete rule;
        }

        for (ProximityInterestAttribute* attr : m_attributes)
        {
            delete attr;
        }

        m_attributes.clear();
        m_rules.clear();

        m_resultCache.clear();
    }
    ///////////////////////////////////////////////////////////////////////////
}
#else
// Optimized spatial handler.
#include "GridMate/Replica/Interest/ProximityInterestHandler.h"
#endif

namespace UnitTest {

/*
 * Helper class to capture performance of various Interest Managers
 */
class PerfForInterestManager
{
public:
    void Reset();

    void PreUpdate();
    void PostUpdate();

    AZ::u32 GetTotalFrames() const;
    float GetAverageFrame() const;
    float GetWorstFrame() const;
    float GetBestFrame() const;

private:
    AZ::Debug::Timer m_timer;
    AZ::u32          m_frameCount = 0;
    float            m_totalUpdateTime = 0.f;
    float            m_fastestFrame = 100.f;
    float            m_slowestFrame = 0.f;
};

PerfForInterestManager g_PerfIM = PerfForInterestManager();
PerfForInterestManager g_PerfUpdatingAttributes = PerfForInterestManager();

/*
* Utility function to tick the replica manager
*/
static void UpdateReplicas(ReplicaManager* replicaManager, InterestManager* interestManager)
{
    if (interestManager)
    {
        // Measuring time it takes to execute an update.
        g_PerfIM.PreUpdate();
        interestManager->Update();
        g_PerfIM.PostUpdate();
    }

    if (replicaManager)
    {
        replicaManager->Unmarshal();
        replicaManager->UpdateFromReplicas();
        replicaManager->UpdateReplicas();
        replicaManager->Marshal();
    }
}

class Integ_InterestTest
    : public GridMateMPTestFixture
{
    ///////////////////////////////////////////////////////////////////
    class InterestTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(InterestTestChunk);

        InterestTestChunk()
            : m_data("Data", 0)
            , m_bitmaskAttributeData("BitmaskAttributeData")
            , m_attribute(nullptr)
        {
        }

        ///////////////////////////////////////////////////////////////////
        typedef AZStd::intrusive_ptr<InterestTestChunk> Ptr;
        static const char* GetChunkName() { return "InterestTestChunk"; }
        bool IsReplicaMigratable() override { return false; }
        bool IsBroadcast() override
        {
            return false;
        }
        ///////////////////////////////////////////////////////////////////

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            AZ_Printf("GridMate", "InterestTestChunk::OnReplicaActivate repId=%08X(%s) fromPeerId=%08X localPeerId=%08X\n",
                GetReplicaId(),
                IsMaster() ? "master" : "proxy",
                rc.m_peer ? rc.m_peer->GetId() : 0,
                rc.m_rm->GetLocalPeerId());

            BitmaskInterestHandler* ih = static_cast<BitmaskInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("BitmaskInterestHandler", 0x5bf5d75b)));
            if (ih)
            {
                m_attribute = ih->CreateAttribute(GetReplicaId());
                m_attribute->Set(m_bitmaskAttributeData.Get());
            }
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            AZ_Printf("GridMate", "InterestTestChunk::OnReplicaDeactivate repId=%08X(%s) fromPeerId=%08X localPeerId=%08X\n",
                GetReplicaId(),
                IsMaster() ? "master" : "proxy",
                rc.m_peer ? rc.m_peer->GetId() : 0,
                rc.m_rm->GetLocalPeerId());

            m_attribute = nullptr;
        }

        void BitmaskHandler(const InterestBitmask& bitmask, const TimeContext&)
        {
            if (m_attribute)
            {
                m_attribute->Set(bitmask);
            }
        }

        DataSet<int> m_data;
        DataSet<InterestBitmask>::BindInterface<InterestTestChunk, &InterestTestChunk::BitmaskHandler> m_bitmaskAttributeData;
        BitmaskInterestAttribute::Ptr m_attribute;
    };
    ///////////////////////////////////////////////////////////////////

    class TestPeerInfo
        : public SessionEventBus::Handler
    {
    public:
        TestPeerInfo()
            : m_gridMate(nullptr)
            , m_lanSearch(nullptr)
            , m_session(nullptr)
            , m_im(nullptr)
            , m_bitmaskHandler(nullptr)
            , m_num(0)
        {
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::BitmaskInterestChunk>();
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::ProximityInterestChunk>();
        }

        void CreateTestReplica()
        {
            m_im = aznew InterestManager();
            InterestManagerDesc desc;
            desc.m_rm = m_session->GetReplicaMgr();

            m_im->Init(desc);

            m_bitmaskHandler = aznew BitmaskInterestHandler();
            m_im->RegisterHandler(m_bitmaskHandler);

            m_rule = m_bitmaskHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(1 << m_num);

            auto r = Replica::CreateReplica("InterestTestReplica");
            m_replica = CreateAndAttachReplicaChunk<InterestTestChunk>(r);

            // Initializing attribute
            // Shifing all by one - peer0 will recv from peer1, peer2 will recv from peer2, peer2 will recv from peer0
            unsigned i = (m_num + 2) % Integ_InterestTest::k_numMachines;
            m_replica->m_data.Set(m_num);
            m_replica->m_bitmaskAttributeData.Set(1 << i);

            m_session->GetReplicaMgr()->AddMaster(r);
        }

        void UpdateAttribute()
        {
            // Shifing all by one - peer0 will recv from peer2, peer1 will recv from peer0, peer2 will recv from peer1
            unsigned i = (m_num + 1) % Integ_InterestTest::k_numMachines;
            m_replica->m_bitmaskAttributeData.Set(1 << i);
            m_replica->m_attribute->Set(1 << i);
        }

        void DeleteAttribute()
        {
            m_replica->m_attribute = nullptr;
        }

        void UpdateRule()
        {
            m_rule->Set(0xffff);
        }

        void DeleteRule()
        {
            m_rule = nullptr;
        }

        void CreateRule()
        {
            m_rule = m_bitmaskHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(0xffff);
        }

        void OnSessionCreated(GridSession* session) override
        {
            m_session = session;
            if (m_session->IsHost())
            {
                CreateTestReplica();
            }
        }

        void OnSessionJoined(GridSession* session) override
        {
            m_session = session;
            CreateTestReplica();
        }

        void OnSessionDelete(GridSession* session) override
        {
            if (session == m_session)
            {
                m_rule = nullptr;
                m_session = nullptr;
                m_im->UnregisterHandler(m_bitmaskHandler);
                delete m_bitmaskHandler;
                delete m_im;
                m_im = nullptr;
                m_bitmaskHandler = nullptr;
            }
        }

        void OnSessionError(GridSession* session, const string& errorMsg) override
        {
            (void)session;
            (void)errorMsg;
            AZ_TracePrintf("GridMate", "Session error: %s\n", errorMsg.c_str());
        }

        IGridMate* m_gridMate;
        GridSearch* m_lanSearch;
        GridSession* m_session;
        InterestManager* m_im;
        BitmaskInterestHandler* m_bitmaskHandler;

        BitmaskInterestRule::Ptr m_rule;
        unsigned m_num;
        InterestTestChunk::Ptr m_replica;
    };

public:
    Integ_InterestTest()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<InterestTestChunk>();

        //////////////////////////////////////////////////////////////////////////
        // Create all grid mates
        m_peers[0].m_gridMate = m_gridMate;
        m_peers[0].SessionEventBus::Handler::BusConnect(m_peers[0].m_gridMate);
        m_peers[0].m_num = 0;
        for (int i = 1; i < k_numMachines; ++i)
        {
            GridMateDesc desc;
            m_peers[i].m_gridMate = GridMateCreate(desc);
            AZ_TEST_ASSERT(m_peers[i].m_gridMate);

            m_peers[i].m_num = i;
            m_peers[i].SessionEventBus::Handler::BusConnect(m_peers[i].m_gridMate);
        }
        //////////////////////////////////////////////////////////////////////////

        for (int i = 0; i < k_numMachines; ++i)
        {
            // start the multiplayer service (session mgr, extra allocator, etc.)
            StartGridMateService<LANSessionService>(m_peers[i].m_gridMate, SessionServiceDesc());
            AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_peers[i].m_gridMate) != nullptr);
        }
    }
    virtual ~Integ_InterestTest()
    {
        StopGridMateService<LANSessionService>(m_peers[0].m_gridMate);

        for (int i = 1; i < k_numMachines; ++i)
        {
            if (m_peers[i].m_gridMate)
            {
                m_peers[i].SessionEventBus::Handler::BusDisconnect();
                GridMateDestroy(m_peers[i].m_gridMate);
            }
        }

        // this will stop the first IGridMate which owns the memory allocators.
        m_peers[0].SessionEventBus::Handler::BusDisconnect();
    }

    void run()
    {
        TestCarrierDesc carrierDesc;
        carrierDesc.m_enableDisconnectDetection = false;// true;
        carrierDesc.m_threadUpdateTimeMS = 10;
        carrierDesc.m_familyType = Driver::BSD_AF_INET;


        LANSessionParams sp;
        sp.m_topology = ST_PEER_TO_PEER;
        sp.m_numPublicSlots = 64;
        sp.m_port = k_hostPort;
        EBUS_EVENT_ID_RESULT(m_peers[k_host].m_session, m_peers[k_host].m_gridMate, LANSessionServiceBus, HostSession, sp, carrierDesc);
        m_peers[k_host].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

        int listenPort = k_hostPort;
        for (int i = 0; i < k_numMachines; ++i)
        {
            if (i == k_host)
            {
                continue;
            }

            LANSearchParams searchParams;
            searchParams.m_serverPort = k_hostPort;
            searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;  // first client will use ephemeral port, the rest specify return ports
            searchParams.m_familyType = Driver::BSD_AF_INET;
            EBUS_EVENT_ID_RESULT(m_peers[i].m_lanSearch, m_peers[i].m_gridMate, LANSessionServiceBus, StartGridSearch, searchParams);
        }


        static const int maxNumUpdates = 300;
        int numUpdates = 0;
        TimeStamp time = AZStd::chrono::system_clock::now();

        while (numUpdates <= maxNumUpdates)
        {
            if (numUpdates == 100)
            {
                // Checking everybody received only one replica:
                // peer0 -> rep1, peer1 -> rep2, peer2 -> rep0
                for (int i = 0; i < k_numMachines; ++i)
                {
                    ReplicaId repId = m_peers[(i + 1) % k_numMachines].m_replica->GetReplicaId();
                    auto repRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repRecv != nullptr);

                    repId = m_peers[(i + 2) % k_numMachines].m_replica->GetReplicaId();
                    auto repNotRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repNotRecv == nullptr);

                    // rotating mask left
                    m_peers[i].UpdateAttribute();
                }
            }

            if (numUpdates == 150)
            {
                // Checking everybody received only one replica:
                // peer0 -> rep2, peer1 -> rep0, peer2 -> rep1
                for (int i = 0; i < k_numMachines; ++i)
                {
                    ReplicaId repId = m_peers[(i + 2) % k_numMachines].m_replica->GetReplicaId();
                    auto repRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repRecv != nullptr);

                    repId = m_peers[(i + 1) % k_numMachines].m_replica->GetReplicaId();
                    auto repNotRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repNotRecv == nullptr);

                    // setting rules to accept all replicas
                    m_peers[i].UpdateRule();
                }
            }

            if (numUpdates == 200)
            {
                // Checking everybody received all replicas
                for (int i = 0; i < k_numMachines; ++i)
                {
                    for (int j = 0; j < k_numMachines; ++j)
                    {
                        ReplicaId repId = m_peers[j].m_replica->GetReplicaId();
                        auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                        AZ_TEST_ASSERT(rep);
                    }

                    // Deleting all attributes
                    m_peers[i].DeleteAttribute();
                }
            }

            if (numUpdates == 250)
            {
                // Checking everybody lost all replicas (except master)
                for (int i = 0; i < k_numMachines; ++i)
                {
                    for (int j = 0; j < k_numMachines; ++j)
                    {
                        if (i == j)
                        {
                            continue;
                        }

                        ReplicaId repId = m_peers[j].m_replica->GetReplicaId();
                        auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                        AZ_TEST_ASSERT(rep == nullptr);
                    }

                    // deleting all rules
                    m_peers[i].DeleteRule();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_gridMate)
                {
                    m_peers[i].m_gridMate->Update();
                    if (m_peers[i].m_session)
                    {
                        UpdateReplicas(m_peers[i].m_session->GetReplicaMgr(), m_peers[i].m_im);
                    }
                }
            }
            Update();
            //////////////////////////////////////////////////////////////////////////

            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_lanSearch && m_peers[i].m_lanSearch->IsDone())
                {
                    AZ_TEST_ASSERT(m_peers[i].m_lanSearch->GetNumResults() == 1);
                    JoinParams jp;
                    EBUS_EVENT_ID_RESULT(m_peers[i].m_session, m_peers[i].m_gridMate, LANSessionServiceBus, JoinSessionBySearchInfo, static_cast<const LANSearchInfo&>(*m_peers[i].m_lanSearch->GetResult(0)), jp, carrierDesc);
                    m_peers[i].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

                    m_peers[i].m_lanSearch->Release();
                    m_peers[i].m_lanSearch = nullptr;
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Debug Info
            TimeStamp now = AZStd::chrono::system_clock::now();
            if (AZStd::chrono::milliseconds(now - time).count() > 1000)
            {
                time = now;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].m_session == nullptr)
                    {
                        continue;
                    }

                    if (m_peers[i].m_session->IsHost())
                    {
                        AZ_Printf("GridMate", "------ Host %d ------\n", i);
                    }
                    else
                    {
                        AZ_Printf("GridMate", "------ Client %d ------\n", i);
                    }

                    AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_peers[i].m_session->GetId().c_str(), m_peers[i].m_session->GetNumberOfMembers(), m_peers[i].m_session->IsHost() ? "yes" : "no", m_peers[i].m_session->GetTime());
                    for (unsigned int iMember = 0; iMember < m_peers[i].m_session->GetNumberOfMembers(); ++iMember)
                    {
                        GridMember* member = m_peers[i].m_session->GetMemberByIndex(iMember);
                        AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                    }
                    AZ_Printf("GridMate", "\n");
                }
            }
            //////////////////////////////////////////////////////////////////////////

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
            numUpdates++;
        }
    }

    static const int k_numMachines = 3;
    static const int k_host = 0;
    static const int k_hostPort = 5450;

    TestPeerInfo m_peers[k_numMachines];
};

/*
 * Testing worse case performance of thousands of replicas and a few peers where all replicas/attributes change every frame
 * and peers/rules change every frame as well.
 */
class LargeWorldTest
    : public GridMateMPTestFixture
{
    ///////////////////////////////////////////////////////////////////
    class LargeWorldTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(LargeWorldTestChunk);

        LargeWorldTestChunk()
            : m_data("Data", 0)
            , m_proximityAttributeData("LargeWorldAttributeData")
            , m_attribute(nullptr)
        {
        }

        ///////////////////////////////////////////////////////////////////
        typedef AZStd::intrusive_ptr<LargeWorldTestChunk> Ptr;
        static const char* GetChunkName() { return "LargeWorldTestChunk"; }
        bool IsReplicaMigratable() override { return false; }
        bool IsBroadcast() override
        {
            return false;
        }
        ///////////////////////////////////////////////////////////////////

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            /*if (!IsMaster())*/
            /*{
                AZ_Printf("GridMate", "LargeWorldTestChunk::OnReplicaActivate repId=%08X(%s) fromPeerId=%08X localPeerId=%08X\n",
                    GetReplicaId(),
                    IsMaster() ? "master" : "proxy",
                    rc.m_peer ? rc.m_peer->GetId() : 0,
                    rc.m_rm->GetLocalPeerId());
            }*/

            if (ProximityInterestHandler* ih = static_cast<ProximityInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4))))
            {
                m_attribute = ih->CreateAttribute(GetReplicaId());
                m_attribute->Set(m_proximityAttributeData.Get());
            }
        }

        void OnReplicaDeactivate(const ReplicaContext& /*rc*/) override
        {
            m_attribute = nullptr;
        }

        void ProximityHandler(const AZ::Aabb& bounds, const TimeContext&)
        {
            if (m_attribute)
            {
                m_attribute->Set(bounds);
            }
        }

        DataSet<int> m_data;
        DataSet<AZ::Aabb>::BindInterface<LargeWorldTestChunk, &LargeWorldTestChunk::ProximityHandler> m_proximityAttributeData;
        ProximityInterestAttribute::Ptr m_attribute;
    };
    ///////////////////////////////////////////////////////////////////

    struct LargeWorldParams
    {
        AZ::u32 index = 0;

        const float commonSize = 50;
        const AZ::Aabb box = AZ::Aabb::CreateFromMinMax(
            AZ::Vector3::CreateZero(),
            AZ::Vector3::CreateOne() * commonSize);
        const float commonStep = commonSize + 1;
    };

    static LargeWorldParams& GetWorldParams()
    {
        static LargeWorldParams worldParams;
        return worldParams;
    }

    /*
     * Create a chain of boxes in spaces along X axis
     */
    static AZ::Aabb CreateNextRuleSpace()
    {
        auto offset = GetWorldParams().commonStep * aznumeric_cast<float>(GetWorldParams().index);

        float min[] = { offset, 0, 0 };
        float max[] = {
            GetWorldParams().commonSize + offset,
            GetWorldParams().commonSize,
            GetWorldParams().commonSize };

        auto bounds = AZ::Aabb::CreateFromMinMax(
            AZ::Vector3::CreateFromFloat3(min),
            AZ::Vector3::CreateFromFloat3(max));

        GetWorldParams().index++;
        return bounds;
    }

    class LargeWorldTestPeerInfo
        : public SessionEventBus::Handler
    {
    public:
        LargeWorldTestPeerInfo()
            : m_gridMate(nullptr)
            , m_lanSearch(nullptr)
            , m_session(nullptr)
            , m_im(nullptr)
            , m_proximityHandler(nullptr)
            , m_num(0)
        {
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::BitmaskInterestChunk>();
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::ProximityInterestChunk>();
        }

        ~LargeWorldTestPeerInfo()
        {
            SessionEventBus::Handler::BusDisconnect();
        }

        void CreateHostRuleHandler()
        {
            m_im = aznew InterestManager();
            InterestManagerDesc desc;
            desc.m_rm = m_session->GetReplicaMgr();

            m_im->Init(desc);

            m_proximityHandler = aznew ProximityInterestHandler();
            m_im->RegisterHandler(m_proximityHandler);

            m_rule = m_proximityHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(AZ::Aabb::CreateNull()); // host rule doesn't matter in this test
        }

        void CreateRuleHandler()
        {
            m_im = aznew InterestManager();
            InterestManagerDesc desc;
            desc.m_rm = m_session->GetReplicaMgr();

            m_im->Init(desc);

            m_proximityHandler = aznew ProximityInterestHandler();
            m_im->RegisterHandler(m_proximityHandler);

            m_rule = m_proximityHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(CreateNextRuleSpace());
        }

        void CreateTestReplica(const AZ::Aabb& bounds)
        {
            auto r = Replica::CreateReplica("LargeWorldTestReplica");
            auto replica = CreateAndAttachReplicaChunk<LargeWorldTestChunk>(r);

            // Initializing attribute
            replica->m_data.Set(m_num);
            replica->m_proximityAttributeData.Set(bounds);

            m_replicas.push_back(replica);

            m_session->GetReplicaMgr()->AddMaster(r);
        }

        void PopulateWorld()
        {
            AZ_Printf("GridMate", "LargeWorldTestChunk::PopulateWorld() starting...\n");

            const float worldSizeInBoxes = 50;
            const auto oneBox = AZ::Vector3::CreateOne();
            const auto thickness = 1;
            for (float dx = 0; dx < worldSizeInBoxes; ++dx)
            {
                for (float dy = 0; dy < thickness; ++dy)
                {
                    for (float dz = 0; dz < thickness; ++dz)
                    {
                        auto aabb = AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(50 * dx + 5, dy, dz),
                            AZ::Vector3(50 * dx + 5, dy, dz) + oneBox);

                        CreateTestReplica(aabb);
                    }
                }
            }

            AZ_Printf("GridMate", "LargeWorldTestChunk::PopulateWorld() ... DONE\n");
        }

        void UpdateAttribute(LargeWorldTestChunk::Ptr replica)
        {
            if (replica && replica->m_attribute)
            {
                auto sameValue = replica->m_attribute->Get();

                replica->m_proximityAttributeData.Set(sameValue);
                replica->m_attribute->Set(sameValue);
            }
        }

        void UpdateRule()
        {
            // just make it dirty for now
            if (m_rule)
            {
                auto sameValue = m_rule->Get();
                m_rule->Set(sameValue);
            }
        }

        void DeleteRule()
        {
            m_rule = nullptr;
        }

        void OnSessionCreated(GridSession* session) override
        {
            m_session = session;
            if (m_session->IsHost())
            {
                CreateHostRuleHandler();
                PopulateWorld();
            }
        }

        void OnSessionJoined(GridSession* session) override
        {
            m_session = session;
            CreateRuleHandler();
        }

        void OnSessionDelete(GridSession* session) override
        {
            if (session == m_session)
            {
                m_rule = nullptr;
                m_session = nullptr;
                m_im->UnregisterHandler(m_proximityHandler);
                delete m_proximityHandler;
                delete m_im;
                m_im = nullptr;
                m_proximityHandler = nullptr;
            }
        }

        void OnSessionError(GridSession* session, const string& errorMsg) override
        {
            (void)session;
            (void)errorMsg;
            AZ_TracePrintf("GridMate", "Session error: %s\n", errorMsg.c_str());
        }

        IGridMate* m_gridMate;
        GridSearch* m_lanSearch;
        GridSession* m_session;
        InterestManager* m_im;
        ProximityInterestHandler* m_proximityHandler;

        ProximityInterestRule::Ptr m_rule;
        unsigned m_num;

        AZStd::vector<LargeWorldTestChunk::Ptr> m_replicas;
    };

public:
    LargeWorldTest() : UnitTest::GridMateMPTestFixture(500u * 1024u * 1024u) // 500Mb
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<LargeWorldTestChunk>();

        //////////////////////////////////////////////////////////////////////////
        // Create all grid mates
        m_peers[0].m_gridMate = m_gridMate;
        m_peers[0].SessionEventBus::Handler::BusConnect(m_peers[0].m_gridMate);
        m_peers[0].m_num = 0;
        for (int i = 1; i < k_numMachines; ++i)
        {
            GridMateDesc desc;
            m_peers[i].m_gridMate = GridMateCreate(desc);
            AZ_TEST_ASSERT(m_peers[i].m_gridMate);

            m_peers[i].m_num = i;
            m_peers[i].SessionEventBus::Handler::BusConnect(m_peers[i].m_gridMate);
        }
        //////////////////////////////////////////////////////////////////////////

        for (int i = 0; i < k_numMachines; ++i)
        {
            // start the multiplayer service (session mgr, extra allocator, etc.)
            StartGridMateService<LANSessionService>(m_peers[i].m_gridMate, SessionServiceDesc());
            AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_peers[i].m_gridMate) != nullptr);
        }
    }
    virtual ~LargeWorldTest()
    {
        StopGridMateService<LANSessionService>(m_peers[0].m_gridMate);

        for (int i = 1; i < k_numMachines; ++i)
        {
            if (m_peers[i].m_gridMate)
            {
                m_peers[i].SessionEventBus::Handler::BusDisconnect();
                GridMateDestroy(m_peers[i].m_gridMate);
            }
        }

        // this will stop the first IGridMate which owns the memory allocators.
        m_peers[0].SessionEventBus::Handler::BusDisconnect();
    }

    void run()
    {
        g_PerfIM.Reset();
        g_PerfUpdatingAttributes.Reset();

        TestCarrierDesc carrierDesc;
        carrierDesc.m_enableDisconnectDetection = false;
        carrierDesc.m_threadUpdateTimeMS = 10;
        carrierDesc.m_familyType = Driver::BSD_AF_INET;

        LANSessionParams sp;
        sp.m_topology = ST_PEER_TO_PEER;
        sp.m_numPublicSlots = 64;
        sp.m_port = k_hostPort;
        EBUS_EVENT_ID_RESULT(m_peers[k_host].m_session, m_peers[k_host].m_gridMate, LANSessionServiceBus, HostSession, sp, carrierDesc);
        m_peers[k_host].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

        int listenPort = k_hostPort;
        for (int i = 0; i < k_numMachines; ++i)
        {
            if (i == k_host)
            {
                continue;
            }

            LANSearchParams searchParams;
            searchParams.m_serverPort = k_hostPort;
            searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;  // first client will use ephemeral port, the rest specify return ports
            searchParams.m_familyType = Driver::BSD_AF_INET;
            EBUS_EVENT_ID_RESULT(m_peers[i].m_lanSearch, m_peers[i].m_gridMate, LANSessionServiceBus, StartGridSearch, searchParams);
        }


        static const int maxNumUpdates = 300;
        int numUpdates = 0;
        TimeStamp time = AZStd::chrono::system_clock::now();

        while (numUpdates <= maxNumUpdates)
        {
            g_PerfUpdatingAttributes.PreUpdate();
            for (LargeWorldTestChunk::Ptr replica : m_peers[0].m_replicas)
            {
                m_peers[0].UpdateAttribute(replica);
            }
            g_PerfUpdatingAttributes.PostUpdate();

            for (auto peer : m_peers)
            {
                peer.UpdateRule();
            }

            //if (numUpdates == 100)
            //{
            //    // check how many replicas each client got
            //    for (AZ::u32 i = 1; i < k_numMachines; ++i)
            //    {
            //        AZ::u32 count = 0;
            //        for (auto& replica : m_peers[0].m_replicas)
            //        {
            //            ReplicaId repId = replica->GetReplicaId();
            //            if (auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId))
            //            {
            //                count++;
            //            }
            //        }

            //        const AZ::Aabb& bounds = m_peers[i].m_rule->Get();

            //        AZ_Printf("GridMate", "Session %s Members: %d Bounds: %f.%f.%f-%f.%f.%f Replicas: %d\n", m_peers[i].m_session->GetId().c_str(), m_peers[i].m_session->GetNumberOfMembers(),
            //            static_cast<float>(bounds.GetMin().GetX()),
            //            static_cast<float>(bounds.GetMin().GetY()),
            //            static_cast<float>(bounds.GetMin().GetZ()),
            //            static_cast<float>(bounds.GetMax().GetX()),
            //            static_cast<float>(bounds.GetMax().GetY()),
            //            static_cast<float>(bounds.GetMax().GetZ()), count);

            //        AZ_Assert(count == 1, "Should have at least some replicas to start with");
            //    }
            //}

            if (numUpdates == 200)
            {
                // Deleting all attributes
                for (auto& replica : m_peers[0].m_replicas)
                {
                    replica->m_attribute = nullptr;
                }
            }

            if (numUpdates == 250)
            {
                // Checking everybody lost all replicas (except master)
                for (int i = 0; i < k_numMachines; ++i)
                {
                    /*for (int j = 0; j < k_numMachines; ++j)
                    {
                        if (i == j)
                        {
                            continue;
                        }

                        ReplicaId repId = m_peers[j].m_replica->GetReplicaId();
                        auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                        AZ_TEST_ASSERT(rep == nullptr);
                    }*/

                    // deleting all rules
                    m_peers[i].DeleteRule();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_gridMate)
                {
                    m_peers[i].m_gridMate->Update();
                    if (m_peers[i].m_session)
                    {
                        UpdateReplicas(m_peers[i].m_session->GetReplicaMgr(), m_peers[i].m_im);
                    }
                }
            }
            Update();
            //////////////////////////////////////////////////////////////////////////

            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_lanSearch && m_peers[i].m_lanSearch->IsDone())
                {
                    AZ_TEST_ASSERT(m_peers[i].m_lanSearch->GetNumResults() == 1);
                    JoinParams jp;
                    EBUS_EVENT_ID_RESULT(m_peers[i].m_session, m_peers[i].m_gridMate, LANSessionServiceBus, JoinSessionBySearchInfo, static_cast<const LANSearchInfo&>(*m_peers[i].m_lanSearch->GetResult(0)), jp, carrierDesc);
                    m_peers[i].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

                    m_peers[i].m_lanSearch->Release();
                    m_peers[i].m_lanSearch = nullptr;
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Debug Info
            TimeStamp now = AZStd::chrono::system_clock::now();
            if (AZStd::chrono::milliseconds(now - time).count() > 1000)
            {
                time = now;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].m_session == nullptr)
                    {
                        continue;
                    }

                    if (m_peers[i].m_session->IsHost())
                    {
                        AZ_Printf("GridMate", "------ Host %d ------\n", i);
                    }
                    else
                    {
                        AZ_Printf("GridMate", "------ Client %d ------\n", i);
                    }

                    AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_peers[i].m_session->GetId().c_str(), m_peers[i].m_session->GetNumberOfMembers(), m_peers[i].m_session->IsHost() ? "yes" : "no", m_peers[i].m_session->GetTime());
                    for (unsigned int iMember = 0; iMember < m_peers[i].m_session->GetNumberOfMembers(); ++iMember)
                    {
                        GridMember* member = m_peers[i].m_session->GetMemberByIndex(iMember);
                        AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                    }
                    AZ_Printf("GridMate", "\n");
                }
            }
            //////////////////////////////////////////////////////////////////////////

            //AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
            numUpdates++;
        }

        auto averageFrame = g_PerfIM.GetAverageFrame();
        auto bestFrame = g_PerfIM.GetBestFrame();
        auto worstFrame = g_PerfIM.GetWorstFrame();
        auto frames = g_PerfIM.GetTotalFrames();
        AZ_Printf("GridMate", "Interest manager performance: average_frame = %f sec, frames = %d, best= %f sec, worst= %f sec\n",
            averageFrame, frames, bestFrame, worstFrame);

        AZ_Printf("GridMate", "Updating attributes: average_frame = %f sec, frames = %d, best= %f sec, worst= %f sec\n",
            g_PerfUpdatingAttributes.GetAverageFrame(),
            g_PerfUpdatingAttributes.GetTotalFrames(),
            g_PerfUpdatingAttributes.GetBestFrame(),
            g_PerfUpdatingAttributes.GetWorstFrame());
    }

    static const int k_numMachines = 3;
    static const int k_host = 0;
    static const int k_hostPort = 5450;

    LargeWorldTestPeerInfo m_peers[k_numMachines];
};

void PerfForInterestManager::Reset()
{
    m_frameCount = 0;
    m_totalUpdateTime = 0;
    m_slowestFrame = 0;
    m_fastestFrame = 100.f;
}

void PerfForInterestManager::PreUpdate()
{
    m_timer.Stamp();
}

void PerfForInterestManager::PostUpdate()
{
    auto frameTime = m_timer.StampAndGetDeltaTimeInSeconds();
    m_totalUpdateTime += frameTime;
    m_frameCount++;

    m_slowestFrame = AZStd::max<float>(m_slowestFrame, frameTime);
    m_fastestFrame = AZStd::min<float>(m_fastestFrame, frameTime);
}

AZ::u32 PerfForInterestManager::GetTotalFrames() const
{
    return m_frameCount;
}

float PerfForInterestManager::GetAverageFrame() const
{
    if (m_frameCount > 0)
    {
        return m_totalUpdateTime / m_frameCount;
    }

    return 0;
}

float PerfForInterestManager::GetWorstFrame() const
{
    return m_slowestFrame;
}

float PerfForInterestManager::GetBestFrame() const
{
    return m_fastestFrame;
}

class ProximityHandlerTests
    : public GridMateMPTestFixture
{
public:
    struct xyz
    {
        float x, y, z;
    };

    static AZ::Aabb CreateBox(xyz min, float size)
    {
        return AZ::Aabb::CreateFromMinMax(
            AZ::Vector3::CreateFromFloat3(&min.x),
            AZ::Vector3::CreateFromFloat3(&min.x) + AZ::Vector3::CreateOne() * size);
    }

    static void run()
    {
        SimpleFirstUpdate();
        SecondUpdateAfterNoChanges();
        SimpleOutsideOfRule();
        AttributeMovingOutsideOfRule();
        RuleMovingAndAttributeIsOut();
        RuleDestroyed();
        AttributeDestroyed();
    }

    static void RuleMovingAndAttributeIsOut()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        handler->Update();
        InterestMatchResult results = handler->GetLastResult();
        //ProximityInterestHandler::DebugPrint(results, "1st");

        AZ_TEST_ASSERT(results[1].size() == 1);
        AZ_TEST_ASSERT(results[1].find(100) != results[1].end());

        // now move the attribute outside of the rule
        rule1->Set(CreateBox({ 1000, 0, 0 }, 100));

        handler->Update();
        results = handler->GetLastResult();
        //ProximityInterestHandler::DebugPrint(results, "2nd");

        AZ_TEST_ASSERT(results[1].size() == 0);
    }

    static void AttributeMovingOutsideOfRule()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        handler->Update();
        InterestMatchResult results = handler->GetLastResult();
        //ProximityInterestHandler::DebugPrint(results, "1st");

        AZ_TEST_ASSERT(results[1].size() == 1);
        AZ_TEST_ASSERT(results[1].find(100) != results[1].end());

        // now move the attribute outside of the rule
        attribute1->Set(CreateBox({ -1000, 0, 0 }, 10));

        handler->Update();
        results = handler->GetLastResult();
        //ProximityInterestHandler::DebugPrint(results, "2nd");

        AZ_TEST_ASSERT(results[1].size() == 0);
    }

    static void SimpleFirstUpdate()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        handler->Update();

        InterestMatchResult results = handler->GetLastResult();

        //ProximityInterestHandler::PrintMatchResult(results, "test");

        AZ_TEST_ASSERT(results[1].size() == 1);
        AZ_TEST_ASSERT(results[1].find(100) != results[1].end());
    }

    static void SecondUpdateAfterNoChanges()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        handler->Update();
        handler->Update();

        InterestMatchResult results = handler->GetLastResult();

        //ProximityInterestHandler::PrintMatchResult(results, "test");

        AZ_TEST_ASSERT(results.size() == 0);
    }

    static void SimpleOutsideOfRule()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ -1000, 0, 0 }, 10));

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        handler->Update();

        InterestMatchResult results = handler->GetLastResult();

        //ProximityInterestHandler::PrintMatchResult(results, "test");

        AZ_TEST_ASSERT(results.size() == 1);
        AZ_TEST_ASSERT(results[1].size() == 0);
    }

    static void RuleDestroyed()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto attribute1 = handler->CreateAttribute(1);
        attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

        {
            auto rule1 = handler->CreateRule(100);
            rule1->Set(CreateBox({ 0, 0, 0 }, 100));

            handler->Update();

            InterestMatchResult results = handler->GetLastResult();
            AZ_TEST_ASSERT(results.size() == 1);
            AZ_TEST_ASSERT(results[1].size() == 1);
        }

        // rule1 should have been destroyed by now

        handler->Update();
        InterestMatchResult results = handler->GetLastResult();
        //ProximityInterestHandler::PrintMatchResult(results, "last");

        AZ_TEST_ASSERT(results.size() == 1);
        AZ_TEST_ASSERT(results[1].size() == 0);
    }

    static void AttributeDestroyed()
    {
        AZStd::unique_ptr<ProximityInterestHandler> handler(aznew ProximityInterestHandler());

        auto rule1 = handler->CreateRule(100);
        rule1->Set(CreateBox({ 0, 0, 0 }, 100));

        {
            auto attribute1 = handler->CreateAttribute(1);
            attribute1->Set(CreateBox({ 0, 0, 0 }, 10));

            handler->Update();

            InterestMatchResult results = handler->GetLastResult();
            AZ_TEST_ASSERT(results.size() == 1);
            AZ_TEST_ASSERT(results[1].size() == 1);
        }

        // attribute1 should have been destroyed by now, but it will show up once to remove it from affected peers
        handler->Update();
        InterestMatchResult results = handler->GetLastResult();
        results.PrintMatchResult("last");

        AZ_TEST_ASSERT(results.size() == 1);
        AZ_TEST_ASSERT(results[1].size() == 0);

        // and now attribute1 should not show up in the changes
        handler->Update();
        results = handler->GetLastResult();
        results.PrintMatchResult("last");

        AZ_TEST_ASSERT(results.size() == 0);
    }
};

}; // namespace UnitTest

GM_TEST_SUITE(InterestSuite)
    GM_TEST(Integ_InterestTest);
#if AZ_TRAIT_GRIDMATE_TEST_EXCLUDE_LARGEWORLDTEST != 0
        GM_TEST(LargeWorldTest);
#endif
    GM_TEST(ProximityHandlerTests);
GM_TEST_SUITE_END()
