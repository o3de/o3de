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
#include <INetwork.h>

struct NetworkMock : public INetwork
{
    NetworkMock() : m_gridMate(nullptr)
    {
    }
    GridMate::IGridMate* m_gridMate;

    void Release() override {}
    void GetMemoryStatistics([[maybe_unused]] ICrySizer* pSizer) override {}
    void GetBandwidthStatistics([[maybe_unused]] SBandwidthStats* const pStats) override {}
    void GetPerformanceStatistics([[maybe_unused]] SNetworkPerformance* pSizer) override {}
    void GetProfilingStatistics([[maybe_unused]] SNetworkProfilingStats* const pStats) override {}
    void SyncWithGame([[maybe_unused]] ENetworkGameSync syncType) override {}
    const char* GetHostName() override { return "testhostname"; }
    GridMate::IGridMate* GetGridMate() override
    {
        return m_gridMate;
    }
    ChannelId GetChannelIdForSessionMember([[maybe_unused]] GridMate::GridMember* member) const override { return ChannelId(); }
    ChannelId GetServerChannelId() const override { return ChannelId(); }
    ChannelId GetLocalChannelId() const override { return ChannelId(); }
    CTimeValue GetSessionTime() override { return CTimeValue(); }
    void ChangedAspects([[maybe_unused]] EntityId id, [[maybe_unused]] NetworkAspectType aspectBits) override {}
    void SetDelegatableAspectMask([[maybe_unused]] NetworkAspectType aspectBits) override {}
    void SetObjectDelegatedAspectMask([[maybe_unused]] EntityId entityId, [[maybe_unused]] NetworkAspectType aspects, [[maybe_unused]] bool set) override {}
    void DelegateAuthorityToClient([[maybe_unused]] EntityId entityId, [[maybe_unused]] ChannelId clientChannelId) override {}
    void InvokeActorRMI([[maybe_unused]] EntityId entityId, [[maybe_unused]] uint8 actorExtensionId, [[maybe_unused]] ChannelId targetChannelFilter, [[maybe_unused]] IActorRMIRep& rep) override {}
    void InvokeScriptRMI([[maybe_unused]] ISerializable* serializable, [[maybe_unused]] bool isServerRMI, [[maybe_unused]] ChannelId toChannelId = kInvalidChannelId, [[maybe_unused]] ChannelId avoidChannelId = kInvalidChannelId) override {}
    void RegisterActorRMI([[maybe_unused]] IActorRMIRep* rep) override {}
    void UnregisterActorRMI([[maybe_unused]] IActorRMIRep* rep) override {}
    EntityId LocalEntityIdToServerEntityId([[maybe_unused]] EntityId localId) const override { return EntityId(); }
    EntityId ServerEntityIdToLocalEntityId([[maybe_unused]] EntityId serverId, [[maybe_unused]] bool allowForcedEstablishment = false) const override { return EntityId(); }
};
