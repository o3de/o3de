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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Message definition to id management


#pragma once

#include <ISerialize.h>
#include <smartptr.h>

#define NUM_ASPECTS                         32              // Number of GameObject aspects supported.
#define MAXIMUM_NUMBER_OF_CONNECTIONS       64              // Maximum number of connections supported.
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(INetwork_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#define LOBBY_DEFAULT_PORT                  30090           // Default local UDP port.
#endif
#define SERVER_DEFAULT_PORT LOBBY_DEFAULT_PORT
#define SERVER_DEFAULT_PORT_STRING #SERVER_DEFAULT_PORT
static const int NumAspects = NUM_ASPECTS;
#define _CRYNETWORK_CONCAT(x, y) x ## y
#define CRYNETWORK_CONCAT(x, y)      _CRYNETWORK_CONCAT(x, y)
#define ASPECT_TYPE     CRYNETWORK_CONCAT(uint, NUM_ASPECTS)
typedef ASPECT_TYPE NetworkAspectType;
typedef uint8 NetworkAspectID;
#define NET_ASPECT_ALL (NetworkAspectType(0xFFFFFFFF))

typedef uint32 CryLobbyTaskID;
typedef uint32 ECryLobbyError;

struct ISerializableInfo
    : public CMultiThreadRefCount
    , public ISerializable {};
typedef _smart_ptr<ISerializableInfo> ISerializableInfoPtr;

typedef uint32 ChannelId;                                   // Network channel id (derived from GridMember)
static const ChannelId kInvalidChannelId = ChannelId(0);
static const ChannelId kOfflineChannelId = ChannelId(1);

typedef unsigned int EntityId;
static const EntityId kInvalidEntityId = (EntityId)0;

#define NET_PROFILE_COUNT_READ_BITS(count)
#define NET_PROFILE_BEGIN(string, read)
#define NET_PROFILE_BEGIN_BUDGET(string, read, budget)
#define NET_PROFILE_BEGIN_RMI(string, read)
#define NET_PROFILE_END()
#define NET_PROFILE_SCOPE(string, read)
#define NET_PROFILE_SCOPE_RMI(string, read)
#define NET_PROFILE_SCOPE_BUDGET(string, read, budget)

enum ENetReliabilityType
{
    eNRT_ReliableOrdered,
    eNRT_ReliableUnordered,
    eNRT_UnreliableOrdered,
    eNRT_UnreliableUnordered,
    // Must be last.
    eNRT_NumReliabilityTypes
};

// Description:
//   Implementation of CContextView relies on the first two values being
//   as they are.
enum ERMIAttachmentType
{
    eRAT_PreAttach = 0,
    eRAT_PostAttach = 1,
    eRAT_NoAttach,

    // Must be last.
    eRAT_NumAttachmentTypes
};

struct SNetworkPerformance
{
    uint64 m_nNetworkSync;
    float m_threadTime;
};

enum ENetworkGameSync
{
    eNGS_FrameStart = 0,
    eNGS_FrameEnd,
    eNGS_Shutdown_Clear,
    eNGS_Shutdown,
    eNGS_MinimalUpdateForLoading, // Internal use - workaround for sync loading problems
    eNGS_AllowMinimalUpdate,
    eNGS_DenyMinimalUpdate,
    // must be last
    eNGS_NUM_ITEMS
};

#define STATS_MAX_MESSAGEQUEUE_ACCOUNTING_GROUPS (64)
struct SAccountingGroupStats
{
    SAccountingGroupStats()
        : m_sends(0)
        , m_bandwidthUsed(0.0f)
        , m_totalBandwidthUsed(0.0f)
        , m_priority(0)
        , m_maxLatency(0.0f)
        , m_discardLatency(0.0f)
        , m_inUse(false)
    {
        memset(m_name, 0, sizeof(m_name));
    }

    char m_name[8];
    uint32 m_sends;
    float m_bandwidthUsed;
    float m_totalBandwidthUsed;
    uint32 m_priority;
    float m_maxLatency;
    float m_discardLatency;
    bool m_inUse;
};

struct SMessageQueueStats
{
    SMessageQueueStats()
        : m_usedPacketSize(0)
        , m_sentMessages(0)
        , m_unsentMessages(0)
    {
    }

    SAccountingGroupStats m_accountingGroup[STATS_MAX_MESSAGEQUEUE_ACCOUNTING_GROUPS];
    uint32 m_usedPacketSize;
    uint16 m_sentMessages;
    uint16 m_unsentMessages;
};

#define STATS_MAX_NUMBER_OF_CHANNELS (MAXIMUM_NUMBER_OF_CONNECTIONS + 1)
#define STATS_MAX_NAME_SIZE (32)
struct SNetChannelStats
{
    SNetChannelStats()
        : m_ping(0)
        , m_pingSmoothed(0)
        , m_bandwidthInbound(0.0f)
        , m_bandwidthOutbound(0.0f)
        , m_bandwidthShares(0)
        , m_desiredPacketRate(0)
        , m_currentPacketRate(0.0f)
        , m_packetLossRate(0.0f)
        , m_maxPacketSize(0)
        , m_idealPacketSize(0)
        , m_sparePacketSize(0)
        , m_idle(false)
        , m_inUse(false)
    {
        memset(m_name, 0, sizeof(m_name));
    }

    SMessageQueueStats m_messageQueue;

    char m_name[STATS_MAX_NAME_SIZE];
    uint32 m_ping;
    uint32 m_pingSmoothed;
    float m_bandwidthInbound;
    float m_bandwidthOutbound;
    uint32 m_bandwidthShares;
    uint32 m_desiredPacketRate;
    float m_currentPacketRate;
    float m_packetLossRate;
    uint32 m_maxPacketSize;
    uint32 m_idealPacketSize;
    uint32 m_sparePacketSize;
    bool m_idle;
    bool m_inUse;
};

struct SBandwidthStatsSubset
{
    SBandwidthStatsSubset()
        : m_totalBandwidthSent(0)
        , m_lobbyBandwidthSent(0)
        , m_fragmentBandwidthSent(0)
        , m_aspectPayloadBitsSent(0)
        , m_rmiPayloadBitsSent(0)
        , m_totalBandwidthRecvd(0)
        , m_totalPacketsSent(0)
        , m_totalPacketsDropped(0)
        , m_lobbyPacketsSent(0)
        , m_fragmentPacketsSent(0)
        , m_totalPacketsRecvd(0)
    {
    }

    uint64  m_totalBandwidthSent;
    uint64  m_lobbyBandwidthSent;
    uint64  m_fragmentBandwidthSent;
    uint64  m_aspectPayloadBitsSent;
    uint64  m_rmiPayloadBitsSent;
    uint64  m_totalBandwidthRecvd;
    int         m_totalPacketsSent;
    uint64      m_totalPacketsDropped;
    int         m_lobbyPacketsSent;
    int         m_fragmentPacketsSent;
    int         m_totalPacketsRecvd;
};

struct SBandwidthStats
{
    SBandwidthStats()
        : m_total()
        , m_prev()
        , m_1secAvg()
        , m_10secAvg()
    {
    }

    SBandwidthStatsSubset TickDelta()
    {
        SBandwidthStatsSubset ret;
        ret.m_totalBandwidthSent = m_total.m_totalBandwidthSent - m_prev.m_totalBandwidthSent;
        ret.m_lobbyBandwidthSent = m_total.m_lobbyBandwidthSent - m_prev.m_lobbyBandwidthSent;
        ret.m_fragmentBandwidthSent = m_total.m_fragmentBandwidthSent - m_prev.m_fragmentBandwidthSent;
        ret.m_aspectPayloadBitsSent = m_total.m_aspectPayloadBitsSent - m_prev.m_aspectPayloadBitsSent;
        ret.m_rmiPayloadBitsSent = m_total.m_rmiPayloadBitsSent - m_prev.m_rmiPayloadBitsSent;

        ret.m_totalBandwidthRecvd = m_total.m_totalBandwidthRecvd - m_prev.m_totalBandwidthRecvd;
        ret.m_totalPacketsSent = m_total.m_totalPacketsSent - m_prev.m_totalPacketsSent;
        ret.m_lobbyPacketsSent = m_total.m_lobbyPacketsSent - m_prev.m_lobbyPacketsSent;
        ret.m_fragmentPacketsSent = m_total.m_fragmentPacketsSent - m_prev.m_fragmentPacketsSent;
        ret.m_totalPacketsRecvd = m_total.m_totalPacketsRecvd - m_prev.m_totalPacketsRecvd;

        ret.m_totalPacketsDropped = m_total.m_totalPacketsDropped - m_prev.m_totalPacketsDropped;
        return ret;
    }

    SBandwidthStatsSubset       m_total;
    SBandwidthStatsSubset       m_prev;
    SBandwidthStatsSubset       m_1secAvg;
    SBandwidthStatsSubset       m_10secAvg;

    SNetChannelStats    m_channel[STATS_MAX_NUMBER_OF_CHANNELS];
    uint32                      m_numChannels;
};

struct SProfileInfoStat
{
    SProfileInfoStat()
        : m_name("")
        , m_totalBits(0)
        , m_calls(0)
        , m_rmi(false)
    {
    }

    string                      m_name;
    uint32                      m_totalBits;
    uint32                      m_calls;
    bool                            m_rmi;
};

typedef DynArray<SProfileInfoStat> ProfileLeafList;

struct SNetworkProfilingStats
{
    SNetworkProfilingStats()
        : m_ProfileInfoStats()
        , m_numBoundObjects(0)
        , m_maxBoundObjects(0)
    {
    }

    ProfileLeafList                         m_ProfileInfoStats;
    uint                                                m_numBoundObjects;
    uint                                                m_maxBoundObjects;
};

enum EDisconnectionCause
{
    // This cause must be first! - timeout occurred.
    eDC_Timeout = 0,
    // Incompatible protocols.
    eDC_ProtocolError,
    // Failed to resolve an address.
    eDC_ResolveFailed,
    // Versions mismatch.
    eDC_VersionMismatch,
    // Server is full.
    eDC_ServerFull,
    // User initiated kick.
    eDC_Kicked,
    // Teamkill ban/ admin ban.
    eDC_Banned,
    // Context database mismatch.
    eDC_ContextCorruption,
    // Password mismatch, cdkey bad, etc.
    eDC_AuthenticationFailed,
    // Misc. game error.
    eDC_GameError,
    // DX11 not found.
    eDC_NotDX11Capable,
    // The nub has been destroyed.
    eDC_NubDestroyed,
    // Icmp reported error.
    eDC_ICMPError,
    // NAT negotiation error.
    eDC_NatNegError,
    // Demo playback finished.
    eDC_DemoPlaybackFinished,
    // Demo playback file not found.
    eDC_DemoPlaybackFileNotFound,
    // User decided to stop playing.
    eDC_UserRequested,
    // User should have controller connected.
    eDC_NoController,
    // Unable to connect to server.
    eDC_CantConnect,
    // Arbitration failed in a live arbitrated session.
    eDC_ArbitrationFailed,
    // Failed to successfully join migrated game
    eDC_FailedToMigrateToNewHost,
    // The session has just been deleted
    eDC_SessionDeleted,
    // Kicked due to having a high ping
    eDC_KickedHighPing,
    // Kicked due to reserved user joining
    eDC_KickedReservedUser,
    // Class registry mismatch
    eDC_ClassRegistryMismatch,
    // Global ban
    eDC_GloballyBanned,
    // Global ban stage 1 messaging
    eDC_Global_Ban1,
    // Global ban stage 2 messaging
    eDC_Global_Ban2,
    // This cause must be last! - unknown cause.
    eDC_Unknown
};

enum EAspectFlags
{
    // aspect will not be sent to clients that don't control the entity
    eAF_ServerControllerOnly = 0x04,
    // aspect is serialized without using compression manager (useful for data that is allready well quantised/compressed)
    eAF_NoCompression = 0x08,
    // aspect can be client controlled (delegated to the client)
    eAF_Delegatable = 0x10,
    // aspect has more than one profile (serialization format)
    eAF_ServerManagedProfile = 0x20,
    // client should periodically send a hash of what it thinks the current state of an aspect is
    // this hash is compared to the server hash and forces a server update if there's a mismatch
    eAF_HashState = 0x40,
    // aspect needs a timestamp to make sense (i.e. physics)
    eAF_TimestampState = 0x80,
};

/*!
 * Crytek RMI representation. RMI declaration/impl macros subclass this, allowing
 * the network layer to callback for param serialization and invocation.
 * The derived classes are created statically by RMI impl macros, and assigned a
 * unique Id for lookup on the receiving end.
 */
class IRMIRep
{
public:

    IRMIRep()
        : m_uniqueId(0) {}
    virtual ~IRMIRep() {}
    virtual const char* GetDebugName() const = 0;
    virtual void SerializeParamsToBuffer(TSerialize ser, void* params) = 0;
    virtual void* SerializeParamsFromBuffer(TSerialize ser) = 0;
    virtual bool IsServerRMI() const = 0;

    void SetUniqueId(uint32 uniqueId) { m_uniqueId = uniqueId; }
    uint32 GetUniqueId() const { return m_uniqueId; }

    bool operator == (const size_t compareId) const
    {
        return GetUniqueId() == compareId;
    }

private:

    uint32 m_uniqueId;
};

/*!
 * Actor (GameCore) RMI representation. RMI declaration/impl macros subclass this,
 * allowing the network layer to callback for param serialization and invocation.
 * A unique ID is also maintained to enable lookup on the receiving end. IDs are
 * assigned when the rep is 'registered' via RegisterActorRMI().
 */
class IActorRMIRep
{
public:

    IActorRMIRep()
        : m_uniqueId(0) {}
    virtual ~IActorRMIRep() {}

    virtual uint32 GetReliability() const = 0;
    virtual uint32 GetWhere() const = 0;
    virtual void SerializeParams(TSerialize ser) = 0;
    virtual void Invoke(EntityId entityId, uint8 actorExtensionId) = 0;
    virtual const char* GetDebugName() const = 0;

    void SetUniqueId(uint32 uniqueId) { m_uniqueId = uniqueId; }
    uint32 GetUniqueId() const { return m_uniqueId; }

    bool operator == (const size_t compareId) const
    {
        return GetUniqueId() == compareId;
    }

    bool operator < (const size_t compareId) const
    {
        return GetUniqueId() <= compareId;
    }

private:

    uint32 m_uniqueId;
};

// GridMate forwards
namespace GridMate
{
    class IGridMate;
    class Replica;
    class GridSearch;
    class GridMember;
    class GridSession;
    struct SessionParams;
    struct SearchInfo;
    struct InviteInfo;
    struct SearchParams;
    struct CarrierDesc;
} // namespace GridMate

struct INetwork
{
public:
    // Description:
    //    Releases the interface (and delete the object that implements it).
    virtual void Release() = 0;

    // Description:
    //    Gathers memory statistics for the network module.
    virtual void GetMemoryStatistics(ICrySizer* pSizer) = 0;

    // Description:
    //    Gets the socket level bandwidth statistics
    virtual void GetBandwidthStatistics(SBandwidthStats* const pStats) = 0;

    // Description:
    //    Gathers performance statistics for the network module.
    virtual void GetPerformanceStatistics(SNetworkPerformance* pSizer) = 0;

    // Description:
    //    Gets debug and profiling statistics from network members
    virtual void GetProfilingStatistics(SNetworkProfilingStats* const pStats) = 0;

    // Description:
    //    Updates all nubs and contexts.
    // Arguments:
    //    blocking - time to block for network input (zero to not block).
    virtual void SyncWithGame(ENetworkGameSync syncType) = 0;

    // Description:
    //    Gets the local host name.
    virtual const char* GetHostName() = 0;


    // New
    virtual GridMate::IGridMate* GetGridMate() = 0;

    virtual ChannelId GetChannelIdForSessionMember(GridMate::GridMember* member) const = 0;
    virtual ChannelId GetServerChannelId() const = 0;
    virtual ChannelId GetLocalChannelId() const = 0;

    //! Gets the synchronized network time as milliseconds since session creation time.
    virtual CTimeValue GetSessionTime() = 0;

    virtual void ChangedAspects(EntityId id, NetworkAspectType aspectBits) = 0;

    //////// Client-delegatable aspect shim.
    //! Sets mask describing which aspects are globally delegatable.
    virtual void SetDelegatableAspectMask(NetworkAspectType aspectBits) = 0;
    //! Sets mask on a given obejct describing which aspect that object has delegated to the controlling client.
    virtual void SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set) = 0;
    //! Request authority for entityId be delegated to client at clientChannelId.
    virtual void DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId) = 0;
    ////////

    virtual void InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep) = 0;

    virtual void InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId = kInvalidChannelId, ChannelId avoidChannelId = kInvalidChannelId) = 0;

    virtual void RegisterActorRMI(IActorRMIRep* rep) = 0;
    virtual void UnregisterActorRMI(IActorRMIRep* rep) = 0;

    virtual EntityId LocalEntityIdToServerEntityId(EntityId localId) const = 0;
    virtual EntityId ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment = false) const = 0;
};
