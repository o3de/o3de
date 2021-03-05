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

#include <GridMate/Replica/Interest/ProximityInterestHandler.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>

#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/BvDynamicTree.h>

// for highly verbose internal debugging
//#define INTERNAL_DEBUG_PROXIMITY

namespace GridMate
{

    void ProximityInterestChunk::OnReplicaActivate(const ReplicaContext& rc)
    {
        m_interestHandler = static_cast<ProximityInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4)));
        AZ_Warning("GridMate", m_interestHandler, "No proximity interest handler in the user context");

        if (m_interestHandler)
        {
            m_interestHandler->OnNewRulesChunk(this, rc.m_peer);
        }
    }

    void ProximityInterestChunk::OnReplicaDeactivate(const ReplicaContext& rc)
    {
        if (rc.m_peer && m_interestHandler)
        {
            m_interestHandler->OnDeleteRulesChunk(this, rc.m_peer);
        }
    }

    bool ProximityInterestChunk::AddRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext& ctx)
    {
        if (IsProxy())
        {
            auto rulePtr = m_interestHandler->CreateRule(ctx.m_sourcePeer);
            rulePtr->Set(bbox);
            m_rules.insert(AZStd::make_pair(netId, rulePtr));
        }

        return true;
    }

    bool ProximityInterestChunk::RemoveRuleFn(RuleNetworkId netId, const RpcContext&)
    {
        if (IsProxy())
        {
            m_rules.erase(netId);
        }

        return true;
    }

    bool ProximityInterestChunk::UpdateRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext&)
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

    bool ProximityInterestChunk::AddRuleForPeerFn(RuleNetworkId netId, PeerId peerId, AZ::Aabb bbox, const RpcContext&)
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
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterest
    */
    ProximityInterest::ProximityInterest(ProximityInterestHandler* handler)
        : m_handler(handler)
          , m_bbox(AZ::Aabb::CreateNull())
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
        m_attributeWorld = AZStd::make_unique<SpatialIndex>();
        AZ_Assert(m_attributeWorld, "Out of memory");
    }

    ProximityInterestRule::Ptr ProximityInterestHandler::CreateRule(PeerId peerId)
    {
        ProximityInterestRule* rulePtr = aznew ProximityInterestRule(this, peerId, GetNewRuleNetId());
        if (m_rm && peerId == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->AddRuleRpc(rulePtr->GetNetworkId(), rulePtr->Get());
        }

        CreateAndInsertIntoSpatialStructure(rulePtr);

        return rulePtr;
    }

    ProximityInterestAttribute::Ptr ProximityInterestHandler::CreateAttribute(ReplicaId replicaId)
    {
        auto newAttribute = aznew ProximityInterestAttribute(this, replicaId);
        AZ_Assert(newAttribute, "Out of memory");

        CreateAndInsertIntoSpatialStructure(newAttribute);

        return newAttribute;
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

        MarkAttributesDirtyInRule(rule);

        rule->m_bbox = AZ::Aabb::CreateNull();
        m_removedRules.insert(rule);
        m_localRules.erase(rule);
    }

    void ProximityInterestHandler::UpdateRule(ProximityInterestRule* rule)
    {
        if (m_rm && rule->GetPeerId() == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->UpdateRuleRpc(rule->GetNetworkId(), rule->Get());
        }

        m_dirtyRules.insert(rule);
    }

    void ProximityInterestHandler::FreeAttribute(ProximityInterestAttribute* attrib)
    {
        delete attrib;
    }

    void ProximityInterestHandler::DestroyAttribute(ProximityInterestAttribute* attrib)
    {
        RemoveFromSpatialStructure(attrib);

        m_attributes.erase(attrib);
        m_removedAttributes.insert(attrib);
    }

    void ProximityInterestHandler::RemoveFromSpatialStructure(ProximityInterestAttribute* attribute)
    {
        attribute->m_bbox = AZ::Aabb::CreateNull();
        m_attributeWorld->Remove(attribute->GetNode());
        attribute->SetNode(nullptr);
    }

    void ProximityInterestHandler::UpdateAttribute(ProximityInterestAttribute* attrib)
    {
        auto node = attrib->GetNode();
        AZ_Assert(node, "Attribute wasn't created correctly");
        node->m_volume = attrib->Get();
        m_attributeWorld->Update(node);

        m_dirtyAttributes.insert(attrib);
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

        if (m_rulesReplica)
        {
            return m_rulesReplica->GetReplicaId() | (static_cast<AZ::u64>(m_lastRuleNetId) << 32);
        }

        return (static_cast<AZ::u64>(m_lastRuleNetId) << 32);
    }

    ProximityInterestChunk* ProximityInterestHandler::FindRulesChunkByPeerId(PeerId peerId)
    {
        auto it = m_peerChunks.find(peerId);
        if (it == m_peerChunks.end())
        {
            return nullptr;
        }

        return it->second;
    }

    const InterestMatchResult& ProximityInterestHandler::GetLastResult()
    {
        return m_resultCache;
    }

    ProximityInterestHandler::RuleSet& ProximityInterestHandler::GetAffectedRules()
    {
        /*
         * The expectation that lots of attributes will change frequently,
         * so there is no point in trying to optimize cases
         * where only a few attributes have changed.
         */
        if (m_dirtyAttributes.empty() && !m_dirtyRules.empty())
        {
            return m_dirtyRules;
        }

        /*
         * Assuming all rules might have been affected.
         *
         * There is an optimization chance here if the number of rules is large, as in 1,000+ rules.
         * To handle such scale we would need another spatial structure for rules.
         */
        return m_localRules;
    }

    void ProximityInterestHandler::GetAttributesWithinRule(ProximityInterestRule* rule, SpatialIndex::NodeCollector& nodes)
    {
        m_attributeWorld->Query(rule->Get(), nodes);
    }

    void ProximityInterestHandler::ClearDirtyState()
    {
        m_dirtyAttributes.clear();
        m_dirtyRules.clear();
    }

    void ProximityInterestHandler::CreateAndInsertIntoSpatialStructure(ProximityInterestAttribute* attribute)
    {
        m_attributes.insert(attribute);
        SpatialIndex::Node* node = m_attributeWorld->Insert(attribute->Get(), attribute);
        attribute->SetNode(node);
    }

    void ProximityInterestHandler::CreateAndInsertIntoSpatialStructure(ProximityInterestRule* rule)
    {
        m_localRules.insert(rule);
    }

    void ProximityInterestHandler::UpdateInternal(InterestMatchResult& result)
    {
        /*
         * The goal is to return all dirty attributes that were either dirty because:
         * 1) they changed which rules have apply to
         * 2) rules have changed and no longer apply to those attributes
         * and thus resulted in different peer(s) associated with a given replica.
         */

        const RuleSet& rules = GetAffectedRules();

        for (auto& dirtyAttribute : m_dirtyAttributes)
        {
            result.insert(dirtyAttribute->GetReplicaId());
        }

        /*
        * The exectation is to have a lot more attributes than rules.
        * The amount of rules should grow linear with amount of peers,
        * so it should be OK to iterate through all rules each update.
        */
        for (auto& rule : rules)
        {
            CheckChangesForRule(rule, result);
        }

        for (auto& removedRule : m_removedRules)
        {
            FreeRule(removedRule);
        }
        m_removedRules.clear();

        // mark removed attribute as having no peers
        for (auto& removedAttribute : m_removedAttributes)
        {
            result.insert(removedAttribute->GetReplicaId());
            FreeAttribute(removedAttribute);
        }
        m_removedAttributes.clear();
    }

    void ProximityInterestHandler::CheckChangesForRule(ProximityInterestRule* rule, InterestMatchResult& result)
    {
        SpatialIndex::NodeCollector collector;
        GetAttributesWithinRule(rule, collector);

        auto peerId = rule->GetPeerId();
        for (ProximityInterestAttribute* attr : collector.GetNodes())
        {
            AZ_Assert(attr, "bad node?");

            auto findIt = result.find(attr->GetReplicaId());
            if (findIt != result.end())
            {
                findIt->second.insert(peerId);
            }
            else
            {
                auto resultIt = result.insert(attr->GetReplicaId());
                AZ_Assert(resultIt.second, "Successfully inserted");
                resultIt.first->second.insert(peerId);
            }
        }
    }

    void ProximityInterestHandler::MarkAttributesDirtyInRule(ProximityInterestRule* rule)
    {
        SpatialIndex::NodeCollector collector;
        GetAttributesWithinRule(rule, collector);

        for (ProximityInterestAttribute* attr : collector.GetNodes())
        {
            AZ_Assert(attr, "bad node?");

            UpdateAttribute(attr);
        }
    }

    void ProximityInterestHandler::ProduceChanges(const InterestMatchResult& before, const InterestMatchResult& after)
    {
        m_resultCache.clear();

#if defined(INTERNAL_DEBUG_PROXIMITY)
        before.PrintMatchResult("before");
        after.PrintMatchResult("after");
#endif

        /*
         * 'after' contains only the stuff that might have changed
         */
        for (auto& possiblyDirty : after)
        {
            ReplicaId repId = possiblyDirty.first;
            const InterestPeerSet& peerSet = possiblyDirty.second;

            auto foundInBefore = before.find(repId);
            if (foundInBefore != before.end())
            {
                if (!HasSamePeers(foundInBefore->second, peerSet))
                {
                    // was in the last calculation but has a different peer set now
                    m_resultCache.insert(AZStd::make_pair(repId, peerSet));
                }
            }
            else
            {
                // since it wasn't present during last calculation
                m_resultCache.insert(AZStd::make_pair(repId, peerSet));
            }
        }

        // Mark attributes (replicas) for removal that have not moved but a rule (clients) no longer sees it
        for (auto& possiblyDirty : before)
        {
            ReplicaId repId = possiblyDirty.first;

            const auto foundInAfter = after.find(repId);
            /*
             * If the prior state was a replica A present on peer X: "A{X}", and now A should no longer be present on any peer: "A{}"
             * then by the rules of InterestHandlers interacting with InterestManager, we should return in @m_resultCache the following:
             *
             * A{} - indicating that replica A must be removed all peers.
             *
             * On the next pass, the prior state would be: "A{}" and the current state would be "A{}" as well. At that point, we have
             * already sent the update to remove A from X, so @m_resultCache should no longer mention A at all.
             */
            if (foundInAfter == after.end() && !possiblyDirty.second.empty() /* "not A{}" see the above comment */)
            {
                m_resultCache.insert(AZStd::make_pair(repId, InterestPeerSet()));
            }
        }

#if defined(INTERNAL_DEBUG_PROXIMITY)
        m_resultCache.PrintMatchResult("changes");
#endif

    }

    bool ProximityInterestHandler::HasSamePeers(const InterestPeerSet& one, const InterestPeerSet& another)
    {
        if (one.size() != another.size())
        {
            return false;
        }

        for (auto& peerFromOne : one)
        {
            if (another.find(peerFromOne) == another.end())
            {
                return false;
            }
        }

        // Safe to assume it's the same sets since all entries are unique in a peer sets
        return true;
    }

    void ProximityInterestHandler::Update()
    {
        InterestMatchResult newResult;

        UpdateInternal(newResult);
        ProduceChanges(m_lastResult, newResult);

        m_lastResult = std::move(newResult);
        ClearDirtyState();
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

        ClearDirtyState();
        DestroyAll();

        m_resultCache.clear();
    }

    void ProximityInterestHandler::DestroyAll()
    {
        for (ProximityInterestRule* rule : m_localRules)
        {
            FreeRule(rule);
        }
        m_localRules.clear();

        for (ProximityInterestAttribute* attr : m_attributes)
        {
            FreeAttribute(attr);
        }
        m_attributes.clear();

        for (auto& removedRule : m_removedRules)
        {
            FreeRule(removedRule);
        }
        m_removedRules.clear();

        for (auto& removedAttribute : m_removedAttributes)
        {
            FreeAttribute(removedAttribute);
        }
        m_removedAttributes.clear();
    }

    ///////////////////////////////////////////////////////////////////////////
    ProximityInterestHandler::~ProximityInterestHandler()
    {
        /*
         * If a handler was registered with a InterestManager, then InterestManager ought to have called OnRulesHandlerUnregistered
         * but this is a safety pre-caution.
         */
        DestroyAll();
    }

    SpatialIndex::SpatialIndex()
    {
        m_tree.reset(aznew GridMate::BvDynamicTree());
    }

    void SpatialIndex::Remove(Node* node)
    {
        m_tree->Remove(node);
    }

    void SpatialIndex::Update(Node* node)
    {
        m_tree->Update(node);
    }

    SpatialIndex::Node* SpatialIndex::Insert(const AZ::Aabb& get, ProximityInterestAttribute* attribute)
    {
        return m_tree->Insert(get, attribute);
    }

    void SpatialIndex::Query(const AZ::Aabb& shape, NodeCollector& nodes)
    {
        m_tree->collideTV(m_tree->GetRoot(), shape, nodes);
    }
}
