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

#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Driller/Stream.h>
#include <AzCore/Driller/DefaultStringPool.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Component/TickBus.h>

namespace AzFramework
{
    //---------------------------------------------------------------------
    // TEMP FOR DEBUGGING ONLY!!!
    //---------------------------------------------------------------------
    class DebugDrillerRemoteSession
        : public DrillerRemoteSession
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrillerRemoteSession, AZ::OSAllocator, 0);

        DebugDrillerRemoteSession()
        {
            AZStd::string filename = AZStd::string::format("remotedrill_%llu", static_cast<AZ::u64>(reinterpret_cast<size_t>(static_cast<DrillerRemoteSession*>(this))));
            m_file.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);
        }

        ~DebugDrillerRemoteSession()
        {
            m_file.Close();
        }

        virtual void ProcessIncomingDrillerData(const char* streamIdentifier, const void* data, size_t dataSize)
        {
            (void)streamIdentifier;

            m_file.Write(data, dataSize);
        }

        virtual void OnDrillerConnectionLost()
        {
            delete this;
        }

        AZ::IO::SystemFile  m_file;
    };
    //---------------------------------------------------------------------

    /**
     * These are the different synchronization messages that are used.
     */
    namespace NetworkDrillerSyncMsgId
    {
        static const AZ::Crc32 NetDrillMsg_RequestDrillerEnum  = AZ_CRC("NetDrillMsg_RequestEnum", 0x517cca25);
        static const AZ::Crc32 NetDrillMsg_RequestStartSession = AZ_CRC("NetDrillMsg_RequestStartSession", 0x5238b5fe);
        static const AZ::Crc32 NetDrillMsg_RequestStopSession  = AZ_CRC("NetDrillMsg_RequestStopSession", 0x1abe6888);
        static const AZ::Crc32 NetDrillMsg_DrillerEnum         = AZ_CRC("NetDrillMsg_Enum", 0x3d0a0f76);
    };

    struct NetDrillerStartSessionRequest
        : public TmMsg
    {
        AZ_CLASS_ALLOCATOR(NetDrillerStartSessionRequest, AZ::OSAllocator, 0);
        AZ_RTTI(NetDrillerStartSessionRequest, "{FF899D61-A445-44B5-9B67-8319ACC8BB06}");

        NetDrillerStartSessionRequest()
            : TmMsg(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStartSession) {}

        // TODO: Replace this with the DrillerListType from driller.h
        DrillerListType m_drillerIds;
        AZ::u64         m_sessionId;
    };

    struct NetDrillerStopSessionRequest
        : public TmMsg
    {
        AZ_CLASS_ALLOCATOR(NetDrillerStopSessionRequest, AZ::OSAllocator, 0);
        AZ_RTTI(NetDrillerStopSessionRequest, "{BCC6524F-287F-48D2-A21A-029215DB24DD}");

        NetDrillerStopSessionRequest(AZ::u64 sessionId = 0)
            : TmMsg(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStopSession)
            , m_sessionId(sessionId) {}

        AZ::u64 m_sessionId;
    };

    struct NetDrillerEnumeration
        : public TmMsg
    {
        AZ_CLASS_ALLOCATOR(NetDrillerEnumeration, AZ::OSAllocator, 0);
        AZ_RTTI(NetDrillerEnumeration, "{60E5BED2-F492-4A55-8EF6-2628CD390991}");

        NetDrillerEnumeration()
            : TmMsg(NetworkDrillerSyncMsgId::NetDrillMsg_DrillerEnum) {}

        DrillerInfoListType m_enumeration;
    };

    //---------------------------------------------------------------------
    // DrillerRemoteSession
    //---------------------------------------------------------------------
    DrillerRemoteSession::DrillerRemoteSession()
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        : m_decompressor(&AZ::AllocatorInstance<AZ::OSAllocator>::Get())
#endif
    {
    }
    //---------------------------------------------------------------------
    DrillerRemoteSession::~DrillerRemoteSession()
    {
    }
    //---------------------------------------------------------------------
    void DrillerRemoteSession::StartDrilling(const DrillerListType& drillers, const char* captureFile)
    {
        if (captureFile)
        {
            m_captureFile.Open(captureFile, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
            AZ_Warning("DrillerRemoteSession", m_captureFile.IsOpen(), "Failed to open %s. Driller data will not be saved.", captureFile);
        }

#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        m_decompressor.StartDecompressor();
#endif
        BusConnect(static_cast<AZ::u64>(reinterpret_cast<size_t>(this)));
        EBUS_EVENT(DrillerNetworkConsoleCommandBus, StartRemoteDrillerSession, drillers, this);
    }
    //---------------------------------------------------------------------
    void DrillerRemoteSession::StopDrilling()
    {
        EBUS_EVENT(DrillerNetworkConsoleCommandBus, StopRemoteDrillerSession, static_cast<AZ::u64>(reinterpret_cast<size_t>(this)));
        BusDisconnect();
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        if (m_decompressor.IsDecompressorStarted())
        {
            m_decompressor.StopDecompressor();
        }
#endif

        m_captureFile.Close();
    }
    //---------------------------------------------------------------------
    void DrillerRemoteSession::LoadCaptureData(const char* fileName)
    {
        m_captureFile.Open(fileName, AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
        AZ_Warning("DrillerRemoteSession", m_captureFile.IsOpen(), "Failed to open %s. No driller data could be loaded.", fileName);
        if (m_captureFile.IsOpen())
        {
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
            m_decompressor.StartDecompressor();
#endif
            AZ::IO::SystemFile::SizeType bytesRemaining = m_captureFile.Length();
            AZ::IO::SystemFile::SizeType maxReadChunkSize = 1024 * 1024;
            AZStd::vector<char> readBuffer;
            readBuffer.resize_no_construct(static_cast<size_t>(maxReadChunkSize));
            while (bytesRemaining > 0)
            {
                AZ::IO::SystemFile::SizeType bytesToRead = bytesRemaining < maxReadChunkSize ? bytesRemaining : maxReadChunkSize;
                if (m_captureFile.Read(bytesToRead, readBuffer.data()) != bytesToRead)
                {
                    AZ_Warning("DrillerRemoteSession", false, "Failed reading driller data. No more driller data can be read.");
                    break;
                }
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
                Decompress(readBuffer.data(), static_cast<size_t>(bytesToRead));
                ProcessIncomingDrillerData(fileName, m_uncompressedMsgBuffer.data(), m_uncompressedMsgBuffer.size());
#else
                ProcessIncomingDrillerData(fileName, readBuffer.data(), readBuffer.size());
#endif
                bytesRemaining -= bytesToRead;
            }
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
            m_decompressor.StopDecompressor();
#endif
            m_captureFile.Close();
        }
    }
    //---------------------------------------------------------------------
    void DrillerRemoteSession::OnReceivedMsg(TmMsgPtr msg)
    {
        AZ_Assert(msg->GetCustomBlob(), "Missing driller frame data!");

        if (msg->GetCustomBlobSize() == 0)
        {
            return;
        }

        if (m_captureFile.IsOpen())
        {
            if (m_captureFile.Write(msg->GetCustomBlob(), msg->GetCustomBlobSize()) != msg->GetCustomBlobSize())
            {
                AZ_Warning("DrillerRemoteSession", false, "Failed writing capture data to %s, no more data will be written out.", m_captureFile.Name());
                m_captureFile.Close();
            }
        }

#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        Decompress(msg->GetCustomBlob(), msg->GetCustomBlobSize());
        ProcessIncomingDrillerData(m_captureFile.Name(),m_uncompressedMsgBuffer.data(), m_uncompressedMsgBuffer.size());
#else
        ProcessIncomingDrillerData(m_captureFile.Name(),msg->GetCustomBlob(), msg->GetCustomBlobSize());
#endif
    }
    //---------------------------------------------------------------------
    void DrillerRemoteSession::Decompress(const void* compressedBuffer, size_t compressedBufferSize)
    {
        m_uncompressedMsgBuffer.clear();
        if (m_uncompressedMsgBuffer.capacity() < compressedBufferSize * 10)
        {
            m_uncompressedMsgBuffer.reserve(compressedBufferSize * 10);
        }
#if defined(ENABLE_COMPRESSION_FOR_REMOTE_DRILLER)
        unsigned int compressedBytesRemaining = static_cast<unsigned int>(compressedBufferSize);
        unsigned int decompressedBytes = 0;
        while (compressedBytesRemaining > 0)
        {
            unsigned int uncompressedBytes = c_decompressionBufferSize;
            unsigned int bytesConsumed = m_decompressor.Decompress(reinterpret_cast<const char*>(compressedBuffer) + decompressedBytes, compressedBytesRemaining, m_decompressionBuffer, uncompressedBytes);
            decompressedBytes += bytesConsumed;
            compressedBytesRemaining -= bytesConsumed;
            m_uncompressedMsgBuffer.insert(m_uncompressedMsgBuffer.end(), &m_decompressionBuffer[0], &m_decompressionBuffer[uncompressedBytes]);
        }
#else
        m_uncompressedMsgBuffer.insert(m_uncompressedMsgBuffer.end(), &((char*)compressedBuffer)[0], &((char*)compressedBuffer)[compressedBufferSize]);
#endif
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // DrillerNetSessionStream
    //---------------------------------------------------------------------
    /**
     * Represents a driller session on the target machine.
     * It is responsible for listening for driller events and forwarding
     * them to the console machine.
     */
    class DrillerNetSessionStream
        : public AZ::Debug::DrillerOutputStream
        , AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(DrillerNetSessionStream, AZ::OSAllocator, 0);

        DrillerNetSessionStream(AZ::u64 sessionId);
        ~DrillerNetSessionStream();

        //---------------------------------------------------------------------
        // DrillerOutputStream
        //---------------------------------------------------------------------
        virtual void WriteBinary(const void* data, unsigned int dataSize);
        virtual void OnEndOfFrame();
        //---------------------------------------------------------------------

        //---------------------------------------------------------------------
        // AZ::SystemTickBus
        //---------------------------------------------------------------------
        void    OnSystemTick() override;
        //---------------------------------------------------------------------

        static const size_t                                         c_defaultUncompressedBufferSize = 256 * 1024;
        static const size_t                                         c_defaultCompressedBufferSize = 32 * 1024;
        static const size_t                                         c_bufferCount = 2;

        AZ::Debug::DrillerSession*                                  m_session;
        AZ::u64                                                     m_sessionId;
        TargetInfo                                                  m_requestor;
        size_t                                                      m_activeBuffer;
        AZStd::vector<char, AZ::OSStdAllocator>                     m_uncompressedBuffer[c_bufferCount];
        AZStd::vector<char, AZ::OSStdAllocator>                     m_compressedBuffer[c_bufferCount];

#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        // Compression
        AZ::ZLib                                                    m_compressor;
        AZStd::fixed_vector<char, c_defaultCompressedBufferSize>    m_compressionBuffer;
#endif

        // String Pooling
        AZ::Debug::DrillerDefaultStringPool m_stringPool;

        // TEMP Debug
        //AZ::IO::SystemFile    m_file;
    };

    DrillerNetSessionStream::DrillerNetSessionStream(AZ::u64 sessionId)
        : m_session(NULL)
        , m_sessionId(sessionId)
        , m_activeBuffer(0)
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        , m_compressor(&AZ::AllocatorInstance<AZ::OSAllocator>::Get())
#endif
    {
        for (size_t i = 0; i < c_bufferCount; ++i)
        {
            m_uncompressedBuffer[i].reserve(c_defaultUncompressedBufferSize);
            m_compressedBuffer[i].reserve(c_defaultCompressedBufferSize);
        }

#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        // Level 3 compression seems to give pretty good compression at decent speed.
        // Speed is paramount for us because initial driller packets can be huge and
        // we need to be able to compress the data within the driller report call
        // without blocking for too long.
        m_compressor.StartCompressor(3);
#endif

        SetStringPool(&m_stringPool);

        AZ::SystemTickBus::Handler::BusConnect();
    }
    //---------------------------------------------------------------------
    DrillerNetSessionStream::~DrillerNetSessionStream()
    {
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        m_compressor.StopCompressor();
#endif

        // Debug
        //m_file.Close();
    }
    //---------------------------------------------------------------------
    void DrillerNetSessionStream::WriteBinary(const void* data, unsigned int dataSize)
    {
        size_t activeBuffer = m_activeBuffer;

        if (dataSize > 0)
        {
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
            // Only do the compression when the buffer is full so we don't run the compression all the time
            if (m_uncompressedBuffer[activeBuffer].size() + dataSize > c_defaultUncompressedBufferSize)
            {
                // compress
                unsigned int curDataSize = static_cast<unsigned int>(m_uncompressedBuffer[activeBuffer].size());
                unsigned int remaining = curDataSize;
                while (remaining > 0)
                {
                    unsigned int processedBytes = curDataSize - remaining;
                    unsigned int compressedBytes = m_compressor.Compress(m_uncompressedBuffer[activeBuffer].data() + processedBytes, remaining, m_compressionBuffer.data(), static_cast<unsigned int>(c_defaultCompressedBufferSize));
                    if (compressedBytes > 0)
                    {
                        m_compressedBuffer[activeBuffer].insert(m_compressedBuffer[activeBuffer].end(), m_compressionBuffer.data(), m_compressionBuffer.data() + compressedBytes);
                    }
                }
                m_uncompressedBuffer[activeBuffer].clear();
            }

            m_uncompressedBuffer[activeBuffer].insert(m_uncompressedBuffer[activeBuffer].end(), reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + dataSize);
#else
            // Since we are not compressing, transfer the input directly into our compressed buffer
            m_compressedBuffer[activeBuffer].insert(m_compressedBuffer[activeBuffer].end(), reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + dataSize);
#endif
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetSessionStream::OnEndOfFrame()
    {
        size_t activeBuffer = m_activeBuffer;

#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        // Write whatever data has not yet been compressed and flush the compressor
        unsigned int curDataSize = static_cast<unsigned int>(m_uncompressedBuffer[activeBuffer].size());
        unsigned int remaining = curDataSize;
        unsigned int compressedBytes = 0;
        do
        {
            unsigned int processedBytes = curDataSize - remaining;
            compressedBytes = m_compressor.Compress(m_uncompressedBuffer[activeBuffer].data() + processedBytes, remaining, m_compressionBuffer.data(), static_cast<unsigned int>(c_defaultCompressedBufferSize), AZ::ZLib::FT_SYNC_FLUSH);
            if (compressedBytes > 0)
            {
                m_compressedBuffer[activeBuffer].insert(m_compressedBuffer[activeBuffer].end(), m_compressionBuffer.data(), m_compressionBuffer.data() + compressedBytes);
            }
        } while (compressedBytes > 0 || remaining > 0);
#endif

        m_activeBuffer = (activeBuffer + 1) % 2;    // switch buffers
    }
    //-------------------------------------------------------------------------
    void DrillerNetSessionStream::OnSystemTick()
    {
        // The buffer index we want to send is the one we wrote to in the previous frame.
        size_t bufferIndex = (m_activeBuffer + 1) % 2;

        if (m_compressedBuffer[bufferIndex].empty())
        {
            return;
        }

        TmMsg msg(m_sessionId);

        msg.AddCustomBlob(m_compressedBuffer[bufferIndex].data(), m_compressedBuffer[bufferIndex].size());
        EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_requestor, msg);


        // Debug
        //if (!m_file.IsOpen())
        //{
        //  AZStd::string filename = AZStd::string::format("localdrill_%llu", m_sessionId);
        //  m_file.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);
        //}
        //m_file.Write(msg.GetCustomBlob(), msg.GetCustomBlobSize());

        // Reset buffers
        m_uncompressedBuffer[bufferIndex].clear();
        m_compressedBuffer[bufferIndex].clear();

        // Buffers may grow during exceptional circumstances. Re-shrink them to their default sizes
        // so we don't keep holding on to the memory.
        m_uncompressedBuffer[bufferIndex].reserve(c_defaultUncompressedBufferSize);
        m_compressedBuffer[bufferIndex].reserve(c_defaultCompressedBufferSize);
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // DrillerNetworkAgent
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::Init()
    {
        m_cbDrillerEnumRequest = TmMsgCallback(AZStd::bind(&DrillerNetworkAgentComponent::OnRequestDrillerEnum, this, AZStd::placeholders::_1));
        m_cbDrillerStartRequest = TmMsgCallback(AZStd::bind(&DrillerNetworkAgentComponent::OnRequestDrillerStart, this, AZStd::placeholders::_1));
        m_cbDrillerStopRequest = TmMsgCallback(AZStd::bind(&DrillerNetworkAgentComponent::OnRequestDrillerStop, this, AZStd::placeholders::_1));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::Activate()
    {
        m_cbDrillerEnumRequest.BusConnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestDrillerEnum);
        m_cbDrillerStartRequest.BusConnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStartSession);
        m_cbDrillerStopRequest.BusConnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStopSession);
        TargetManagerClient::Bus::Handler::BusConnect();
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::Deactivate()
    {
        TargetManagerClient::Bus::Handler::BusDisconnect();
        m_cbDrillerEnumRequest.BusDisconnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestDrillerEnum);
        m_cbDrillerStartRequest.BusDisconnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStartSession);
        m_cbDrillerStopRequest.BusDisconnect(NetworkDrillerSyncMsgId::NetDrillMsg_RequestStopSession);

        AZ::Debug::DrillerManager* mgr = NULL;
        EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
        for (size_t i = 0; i < m_activeSessions.size(); ++i)
        {
            if (mgr)
            {
                mgr->Stop(m_activeSessions[i]->m_session);
            }
            delete m_activeSessions[i];
        }
        m_activeSessions.clear();
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DrillerNetworkAgentService", 0xcd2ab821));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DrillerNetworkAgentService", 0xcd2ab821));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DrillerNetworkAgentComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DrillerNetworkAgentComponent>(
                    "Driller Network Agent", "Runs on the machine being drilled and communicates with tools")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }

            ReflectNetDrillerClasses(context);
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::TargetLeftNetwork(TargetInfo info)
    {
        AZ::Debug::DrillerManager* mgr = NULL;
        EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
        for (AZStd::vector<DrillerNetSessionStream*>::iterator it = m_activeSessions.begin(); it != m_activeSessions.end(); )
        {
            if ((*it)->m_requestor.GetNetworkId() == info.GetNetworkId())
            {
                if (mgr)
                {
                    mgr->Stop((*it)->m_session);
                }
                delete *it;
                it = m_activeSessions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::OnRequestDrillerEnum(TmMsgPtr msg)
    {
        AZ::Debug::DrillerManager* mgr = NULL;
        EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
        if (!mgr)
        {
            return;
        }

        TargetInfo sendTo;
        EBUS_EVENT_RESULT(sendTo, TargetManager::Bus, GetTargetInfo, msg->GetSenderTargetId());
        NetDrillerEnumeration drillerEnum;
        for (int i = 0; i < mgr->GetNumDrillers(); ++i)
        {
            AZ::Debug::Driller* driller = mgr->GetDriller(i);
            AZ_Assert(driller, "DrillerManager returned a NULL driller. This is not legal!");
            drillerEnum.m_enumeration.push_back();
            drillerEnum.m_enumeration.back().m_id = driller->GetId();
            drillerEnum.m_enumeration.back().m_groupName = driller->GroupName();
            drillerEnum.m_enumeration.back().m_name = driller->GetName();
            drillerEnum.m_enumeration.back().m_description = driller->GetDescription();
        }
        EBUS_EVENT(TargetManager::Bus, SendTmMessage, sendTo, drillerEnum);
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::OnRequestDrillerStart(TmMsgPtr msg)
    {
        AZ::Debug::DrillerManager* mgr = NULL;
        EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
        if (!mgr)
        {
            return;
        }

        NetDrillerStartSessionRequest* request = azdynamic_cast<NetDrillerStartSessionRequest*>(msg.get());
        AZ_Assert(request, "Not a NetDrillerStartSessionRequest msg!");
        AZ::Debug::DrillerManager::DrillerListType drillers;
        for (size_t i = 0; i < request->m_drillerIds.size(); ++i)
        {
            AZ::Debug::DrillerManager::DrillerInfo di;
            di.id = request->m_drillerIds[i];
            drillers.push_back(di);
        }
        DrillerNetSessionStream* session = aznew DrillerNetSessionStream(request->m_sessionId);
        EBUS_EVENT_RESULT(session->m_requestor, TargetManager::Bus, GetTargetInfo, msg->GetSenderTargetId());
        m_activeSessions.push_back(session);
        session->m_session = mgr->Start(*session, drillers);
    }
    //---------------------------------------------------------------------
    void DrillerNetworkAgentComponent::OnRequestDrillerStop(TmMsgPtr msg)
    {
        NetDrillerStopSessionRequest* request = azdynamic_cast<NetDrillerStopSessionRequest*>(msg.get());
        for (AZStd::vector<DrillerNetSessionStream*>::iterator it = m_activeSessions.begin(); it != m_activeSessions.end(); ++it)
        {
            if ((*it)->m_sessionId == request->m_sessionId)
            {
                AZ::Debug::DrillerManager* mgr = NULL;
                EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
                if (mgr)
                {
                    mgr->Stop((*it)->m_session);
                }
                delete *it;
                m_activeSessions.erase(it);
                return;
            }
        }
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // DrillerRemoteConsole
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::Init()
    {
        m_cbDrillerEnum = TmMsgCallback(AZStd::bind(&DrillerNetworkConsoleComponent::OnReceivedDrillerEnum, this, AZStd::placeholders::_1));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::Activate()
    {
        m_cbDrillerEnum.BusConnect(NetworkDrillerSyncMsgId::NetDrillMsg_DrillerEnum);
        DrillerNetworkConsoleCommandBus::Handler::BusConnect();
        TargetManagerClient::Bus::Handler::BusConnect();
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::Deactivate()
    {
        TargetManagerClient::Bus::Handler::BusDisconnect();
        DrillerNetworkConsoleCommandBus::Handler::BusDisconnect();
        m_cbDrillerEnum.BusDisconnect(NetworkDrillerSyncMsgId::NetDrillMsg_DrillerEnum);

        for (size_t i = 0; i < m_activeSessions.size(); ++i)
        {
            EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_curTarget, NetDrillerStopSessionRequest(static_cast<AZ::u64>(reinterpret_cast<size_t>(m_activeSessions[i]))));
            m_activeSessions[i]->OnDrillerConnectionLost();
        }
        m_activeSessions.clear();
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DrillerNetworkConsoleService", 0x2286125d));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DrillerNetworkConsoleService", 0x2286125d));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DrillerNetworkConsoleComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<DrillerNetworkConsoleComponent>(
                    "Driller Network Console", "Runs on the tool machine and is responsible for communications with the DrillerNetworkAgent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }

            ReflectNetDrillerClasses(context);
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::EnumerateAvailableDrillers()
    {
        EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_curTarget, TmMsg(NetworkDrillerSyncMsgId::NetDrillMsg_RequestDrillerEnum));
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::StartRemoteDrillerSession(const DrillerListType& drillers, DrillerRemoteSession* handler)
    {
        NetDrillerStartSessionRequest request;
        request.m_drillerIds = drillers;
        request.m_sessionId = static_cast<AZ::u64>(reinterpret_cast<size_t>(handler));
        m_activeSessions.push_back(handler);
        EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_curTarget, request);
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::StopRemoteDrillerSession(AZ::u64 sessionId)
    {
        for (size_t i = 0; i < m_activeSessions.size(); ++i)
        {
            if (sessionId == static_cast<AZ::u64>(reinterpret_cast<size_t>(m_activeSessions[i])))
            {
                EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_curTarget, NetDrillerStopSessionRequest(sessionId));
                m_activeSessions[i] = m_activeSessions.back();
                m_activeSessions.pop_back();
            }
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::DesiredTargetConnected(bool connected)
    {
        if (connected)
        {
            EBUS_EVENT_RESULT(m_curTarget, TargetManager::Bus, GetDesiredTarget);
            EBUS_EVENT(DrillerNetworkConsoleCommandBus, EnumerateAvailableDrillers);
        }
        else
        {
            for (size_t i = 0; i < m_activeSessions.size(); ++i)
            {
                m_activeSessions[i]->OnDrillerConnectionLost();
            }
            m_activeSessions.clear();
            EBUS_EVENT(DrillerNetworkConsoleEventBus, OnReceivedDrillerEnumeration, DrillerInfoListType());
        }
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID)
    {
        (void)oldTargetID;
        (void)newTargetID;
        EBUS_EVENT(DrillerNetworkConsoleEventBus, OnReceivedDrillerEnumeration, DrillerInfoListType());
        for (size_t i = 0; i < m_activeSessions.size(); ++i)
        {
            EBUS_EVENT(TargetManager::Bus, SendTmMessage, m_curTarget, NetDrillerStopSessionRequest(static_cast<AZ::u64>(reinterpret_cast<size_t>(m_activeSessions[i]))));
            m_activeSessions[i]->OnDrillerConnectionLost();
        }
        m_activeSessions.clear();
    }
    //---------------------------------------------------------------------
    void DrillerNetworkConsoleComponent::OnReceivedDrillerEnum(TmMsgPtr msg)
    {
        NetDrillerEnumeration* drillerEnum = azdynamic_cast<NetDrillerEnumeration*>(msg.get());
        AZ_Assert(drillerEnum, "No NetDrillerEnumeration message!");

        EBUS_EVENT(DrillerNetworkConsoleEventBus, OnReceivedDrillerEnumeration, drillerEnum->m_enumeration);
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // ReflectNetDrillerClasses
    //---------------------------------------------------------------------
    void ReflectNetDrillerClasses(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            // Assume no one else will register our classes.
            if (serialize->FindClassData(DrillerInfo::RTTI_Type()) == nullptr)
            {
                serialize->Class<DrillerInfo>()
                    ->Field("Id", &DrillerInfo::m_id)
                    ->Field("GroupName", &DrillerInfo::m_groupName)
                    ->Field("Name", &DrillerInfo::m_name)
                    ->Field("Description", &DrillerInfo::m_description);
                serialize->Class<NetDrillerStartSessionRequest, TmMsg>()
                    ->Field("DrillerIds", &NetDrillerStartSessionRequest::m_drillerIds)
                    ->Field("SessionId", &NetDrillerStartSessionRequest::m_sessionId);
                serialize->Class<NetDrillerStopSessionRequest, TmMsg>()
                    ->Field("SessionId", &NetDrillerStopSessionRequest::m_sessionId);
                serialize->Class<NetDrillerEnumeration, TmMsg>()
                    ->Field("Enumeration", &NetDrillerEnumeration::m_enumeration);
            }
        }
    }
    //---------------------------------------------------------------------
}   // namespace AzFramework
