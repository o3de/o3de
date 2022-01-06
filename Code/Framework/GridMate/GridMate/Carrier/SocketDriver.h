/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GridMate/Memory.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>

#include <GridMate_Traits_Platform.h>
#include <GridMate/Carrier/SocketDriver_Platform.h>
#include <GridMate/Carrier/Driver.h>

#if AZ_TRAIT_GRIDMATE_SOCKET_IPV6_SUPPORT_EXTENSION

namespace GridMate
{
    /// Emulate in6_addr, it will never be used
    struct in6_addr
    {
        unsigned char   s6_addr[16];   // IPv6 address
    };

    // Emulate sockaddr_in6 structure, it will never be used
    struct sockaddr_in6
    {
        unsigned short sin6_family; // AF_INET6.
        unsigned short sin6_port;   // Transport level port number.
        in6_addr sin6_addr;         // IPv6 address.
    };

    // Emulate addrinfo, it will be used for IPV4 loopups
    struct addrinfo
    {
        int              ai_flags;
        int              ai_family;
        int              ai_socktype;
        int              ai_protocol;
        socklen_t        ai_addrlen;
        sockaddr*        ai_addr;
        char*            ai_canonname;
        addrinfo*        ai_next;
    };

    static in6_addr in6addr_loopback = {
        { 0 }
    };

    // Emulate ipv6_mreq structure, it will never be used
    struct ipv6_mreq
    {
        in6_addr ipv6mr_multiaddr;          // IPv6 multicast address.
        unsigned long ipv6mr_interface;     // Interface index.
    };
}
#else
    struct addrinfo;
#endif

namespace GridMate
{
    using SocketErrorBuffer = AZStd::array<char, 32>;

    class SocketDriverAddress
        : public DriverAddress
    {
        friend class SocketDriver;
        SocketDriverAddress();
    public:
        struct Hasher
        {
            AZStd::size_t operator()(const SocketDriverAddress& v) const;
        };

        SocketDriverAddress(Driver* driver);
        SocketDriverAddress(Driver* driver, const sockaddr* addr);
        SocketDriverAddress(Driver* driver, const AZStd::string& ip, unsigned int port);

        bool operator==(const SocketDriverAddress& rhs) const;
        bool operator!=(const SocketDriverAddress& rhs) const;

        AZStd::string ToString() const override;
        AZStd::string ToAddress() const override;
        AZStd::string GetIP() const override;
        unsigned int  GetPort() const override;
        const void* GetTargetAddress(unsigned int& addressSize) const override;

        union
        {
            sockaddr_in     m_sockAddr;
            sockaddr_in6    m_sockAddr6;
        };
    };

    /**
     * Base common class for all SocketBased drivers, you can't NOT create an instance of the SocketDriverCommon
     * use CreateSocketDriver function for a BSD socket driver.
     */
    class SocketDriverCommon
        : public Driver
    {
    public:
        using SocketType = Platform::SocketType_Platform;
        SocketDriverCommon(bool isFullPackets = false, bool isCrossPlatform = false, bool isHighPerformance = false);
        virtual ~SocketDriverCommon();

        /**
        * Platform specific functionality.
        */
        /// Return maximum number of active connections at the same time.
        unsigned int GetMaxNumConnections() const override       { return 32; }
        /// Return maximum data size we can send/receive at once in bytes, supported by the platform.
        unsigned int GetMaxSendSize() const override;
        /// Return packet overhead size in bytes.
        unsigned int GetPacketOverheadSize() const override;

        /**
         * User should implement create and bind a UDP socket. This socket will be used for all communications.
         * \param ft family type, for the BSD socket it can be AFT_IPV4 or AFT_IPV6.
         * \param address when 0 it we will assume "any address".
         * \param port When left 0, we use implicit bind (assigned by the system). Otherwise provide a valid port number.
         * \param isBroadcast is valid for Ipv4 only (otherwise ignored). Sets the socket to support broadcasts.
         * \param receiveBufferSize socket receive buffer size in bytes, use 0 for default values.
         * \param sendBufferSize socket send buffer size, use 0 for default values.
         */
        ResultCode  Initialize(int familyType = BSD_AF_INET, const char* address = nullptr, unsigned int port = 0, bool isBroadcast = false, unsigned int receiveBufferSize = 0, unsigned int sendBufferSize = 0) override;

        /// Returns communication port (must be called after Initialize, otherwise it will return 0)
        unsigned int GetPort() const override;

        /// Send data to a user defined address
        ResultCode  Send(const AZStd::intrusive_ptr<DriverAddress>& to, const char* data, unsigned int dataSize) override;
        /**
        * Receives a datagram and stores the source address. maxDataSize must be >= than GetMaxSendSize(). Returns the num of of received bytes.
        * \note If a datagram from a new connection is received, NewConnectionCB will be called. If it rejects the connection the returned from pointer
        * will be NULL while the actual data will be returned.
        */
        unsigned int Receive(char* data, unsigned int maxDataSize, AZStd::intrusive_ptr<DriverAddress>& from, ResultCode* resultCode = 0) override;
        /**
         *  Wait for data to be to the ready for receive. Time out is the maximum time to wait
         * before this function returns. If left to default value it will be in blocking mode (wait until data is ready to be received).
         * \returns true if there is data to be received (always true if timeOut == 0), otherwise false.
         */
        bool WaitForData(AZStd::chrono::microseconds timeOut = AZStd::chrono::microseconds(0)) override;

        /**
         * When you enter wait for data mode, for many reasons you might want to stop wait for data.
         * If you implement this function you need to make sure it's a thread safe function.
         */
        void StopWaitForData() override;

        /// Return true if WaitForData was interrupted before the timeOut expired, otherwise false.
        bool WasStopeedWaitingForData() override                 { return m_isStoppedWaitForData; }

        /// @{ Address conversion functionality. They MUST implemented thread safe. Generally this is not a problem since they just part local data.
        ///  Create address from ip and port. If ip == NULL we will assign a broadcast address.
        AZStd::string  IPPortToAddress(const char* ip, unsigned int port) const override                               { return IPPortToAddressString(ip, port); }
        bool    AddressToIPPort(const AZStd::string& address, AZStd::string& ip, unsigned int& port) const override    { return AddressStringToIPPort(address, ip, port); }
        /// Create address for the socket driver from IP and port
        static AZStd::string   IPPortToAddressString(const char* ip, unsigned int port);
        /// Decompose an address to IP and port
        static bool     AddressStringToIPPort(const AZStd::string& address, AZStd::string& ip, unsigned int& port);
        /// Return the family type of the address (AF_INET,AF_INET6 AF_UNSPEC)
        static BSDSocketFamilyType  AddressFamilyType(const AZStd::string& ip);
        static BSDSocketFamilyType  AddressFamilyType(const char* ip)           { return AddressFamilyType(AZStd::string(ip)); }
        /// @}

        AZStd::intrusive_ptr<DriverAddress> CreateDriverAddress(const AZStd::string& address) override = 0;
        /// Additional CreateDriverAddress function should be implemented.
        virtual AZStd::intrusive_ptr<DriverAddress> CreateDriverAddress(const sockaddr* sockAddr) = 0;

    protected:
        /// returns result of socket(af,type,protocol)
        virtual SocketType  CreateSocket(int af, int type, int protocol);
        /// returns the result of bind(sockAddr)
        virtual int         BindSocket(const sockaddr* sockAddr, size_t sockAddrLen);
        /// set's default socket options
        virtual ResultCode  SetSocketOptions(bool isBroadcast, unsigned int receiveBufferSize, unsigned int sendBufferSize);

        class PlatformSocketDriver
        {
        public:
            GM_CLASS_ALLOCATOR(PlatformSocketDriver);
            PlatformSocketDriver(SocketDriverCommon &parent, SocketType &socket);
            virtual ~PlatformSocketDriver();
            virtual ResultCode Initialize(unsigned int receiveBufferSize, unsigned int sendBufferSize);
            virtual SocketType  CreateSocket(int af, int type, int protocol);
            virtual ResultCode Send(const sockaddr* sockAddr, unsigned int addressSize, const char* data, unsigned int dataSize);
            virtual unsigned int Receive(char* data, unsigned maxDataSize, sockaddr* sockAddr, socklen_t sockAddrLen, ResultCode* resultCode = 0);
            virtual bool WaitForData(AZStd::chrono::microseconds timeOut = AZStd::chrono::microseconds(0));
            virtual void StopWaitForData();
            static bool isSupported();
        protected:
            SocketDriverCommon &m_parent;
            SocketType         &m_socket;

        };
#ifdef AZ_SOCKET_RIO_SUPPORT
        class RIOPlatformSocketDriver final : public PlatformSocketDriver
        {
        public:
            GM_CLASS_ALLOCATOR(RIOPlatformSocketDriver);
            RIOPlatformSocketDriver(SocketDriverCommon &parent, SocketType &socket);
            ~RIOPlatformSocketDriver();
            static bool isSupported();
            SocketType CreateSocket(int af, int type, int protocol) override;
            ResultCode Initialize(unsigned int receiveBufferSize, unsigned int sendBufferSize) override;
            void WorkerSendThread();
            ResultCode Send(const sockaddr* sockAddr, unsigned int /*addressSize*/, const char* data, unsigned int dataSize) override;
            unsigned int Receive(char* data, unsigned maxDataSize, sockaddr* sockAddr, socklen_t sockAddrLen, ResultCode* resultCode) override;
            bool WaitForData(AZStd::chrono::microseconds timeOut) override;
            void StopWaitForData() override;

        private:
            AZ::u64 RoundUpAndDivide(AZ::u64 Value, AZ::u64 RoundTo) const
            {
                return ((Value + RoundTo - 1) / RoundTo);
            }
            AZ::u64 RoundUp(AZ::u64 Value, AZ::u64 RoundTo) const
            {
                // rounds value up to multiple of RoundTo
                // Example: RoundTo: 4
                // Value:  0 1 2 3 4 5 6 7 8
                // Result: 0 4 4 4 4 8 8 8 8
                return RoundUpAndDivide(Value, RoundTo) * RoundTo;
            }
            char *AllocRIOBuffer(AZ::u64 bufferSize, AZ::u64 numBuffers, AZ::u64* amountAllocated=nullptr);
            bool FreeRIOBuffer(char *buffer);

        protected:
            //static const GUID               k_functionTableId = WSAID_MULTIPLE_RIO;
            bool                            m_workersQuit = false;
            AZStd::thread                   m_workerSendThread;
            AZStd::mutex                    m_WorkerSendMutex;
            AZStd::condition_variable       m_triggerWorkerSend;
            AZStd::atomic<int>              m_workerBufferCount;
            RIO_EXTENSION_FUNCTION_TABLE    m_RIO_FN_TABLE;
            RIO_RQ                          m_requestQueue = RIO_INVALID_RQ;
            RIO_CQ                          m_RIORecvQueue = RIO_INVALID_CQ;
            AZStd::mutex                    m_RIOSendQueueMutex;
            RIO_CQ                          m_RIOSendQueue = RIO_INVALID_CQ;
            int                             m_RIONextSendBuffer = 0;
            int                             m_workerNextSendBuffer = 0;
            int                             m_RIONextRecvBuffer = 0;
            int                             m_RIOSendBufferCount = 64;
            int                             m_RIORecvBufferCount = 2048;
            int                             m_RIOSendBuffersInUse = 0;
            int                             m_RIORecvBuffersInUse = 0;
            bool                            m_isInitialized = false;
            AZ::u64                         m_pageSize = 0;
            AZStd::vector<RIO_BUF>          m_RIORecvBuffer;
            AZStd::vector<RIO_BUF>          m_RIORecvAddressBuffer;
            AZStd::vector<RIO_BUF>          m_RIOSendBuffer;
            AZStd::vector<RIO_BUF>          m_RIOSendAddressBuffer;
            static const int                m_RIOBufferSize = 1536;
            char*                           m_rawRecvBuffer = nullptr;
            char*                           m_rawRecvAddressBuffer = nullptr;
            char*                           m_rawSendBuffer = nullptr;
            char*                           m_rawSendAddressBuffer = nullptr;
            enum
            {
                ReceiveEvent = 0,
                SendEvent,
                WakeupOnSend,
                NumberOfEvents
            };
            WSAEVENT                        m_events[NumberOfEvents];            //send and recv events
        };
#endif
        SocketType      m_socket;
        unsigned short  m_port;
        bool            m_isStoppedWaitForData;     ///< True if last WaitForData was interrupted otherwise false.
        bool            m_isFullPackets;            ///< True if we use max packet size vs internet safe packet size (64KB vs 1500 usually)
        bool            m_isCrossPlatform;          ///< True if we support cross platform communication. Then we make sure we use common features.
        bool            m_isIpv6;                   ///< True if we use version 6 of the internet protocol, otherwise false.
        bool            m_isDatagram;               ///< True if the socket was created with SOCK_DGRAM
        AZStd::unique_ptr<PlatformSocketDriver> m_platformDriver;      ///< Platform specific implementation of socket calls
        bool            m_isHighPerformance;        ///< True if using platform-specific high-performance implementation
    };

    namespace SocketOperations
    {
        AZ::u32 HostToNetLong(AZ::u32 hstLong);
        AZ::u32 NetToHostLong(AZ::u32 netLong);
        AZ::u16 HostToNetShort(AZ::u16 hstShort);
        AZ::u16 NetToHostShort(AZ::u16 netShort);

        SocketDriverCommon::SocketType CreateSocket(bool isDatagram, Driver::BSDSocketFamilyType familyType);

        enum class SocketOption : AZ::s32
        {
            NonBlockingIO,
            ReuseAddress,
            KeepAlive,
            Broadcast,
            SendBuffer,
            ReceiveBuffer
        };
        Driver::ResultCode SetSocketOptionValue(SocketDriverCommon::SocketType sock, SocketOption option, const char* optval, AZ::s32 optlen);
        Driver::ResultCode SetSocketOptionBoolean(SocketDriverCommon::SocketType sock, SocketOption option, bool enable);
        Driver::ResultCode EnableTCPNoDelay(SocketDriverCommon::SocketType sock, bool enable);
        Driver::ResultCode SetSocketBlockingMode(SocketDriverCommon::SocketType sock, bool blocking);
        Driver::ResultCode SetSocketLingerTime(SocketDriverCommon::SocketType sock, bool bDoLinger, AZ::u16 timeout);

        enum class ConnectionResult : AZ::s32
        {
            Okay,
            AlreadyConnecting,
            Refused,
            InProgress,
            ConnectFailed,
            NetworkUnreachable,
            TimedOut,
            SocketConnected
        };
        Driver::ResultCode Connect(SocketDriverCommon::SocketType sock, const sockaddr* sockAddr, size_t sockAddrSize, ConnectionResult& outConnectionResult);
        Driver::ResultCode Connect(SocketDriverCommon::SocketType sock, const SocketDriverAddress& addr, ConnectionResult& outConnectionResult);
        Driver::ResultCode Listen(SocketDriverCommon::SocketType sock, AZ::s32 backlog);
        Driver::ResultCode Accept(SocketDriverCommon::SocketType sock, sockaddr* outAddr, socklen_t& outAddrSize, SocketDriverCommon::SocketType& outSocket);
        Driver::ResultCode CloseSocket(SocketDriverCommon::SocketType sock);
        Driver::ResultCode Send(SocketDriverCommon::SocketType sock, const char* buf, AZ::u32 bufLen, AZ::u32& bytesSent);
        Driver::ResultCode Receive(SocketDriverCommon::SocketType sock, char* buf, AZ::u32& inOutlen);
        Driver::ResultCode Bind(SocketDriverCommon::SocketType sock, const sockaddr* sockAddr, size_t sockAddrSize);
        bool IsWritable(SocketDriverCommon::SocketType sock, AZStd::chrono::microseconds timeOut);
        bool IsReceivePending(SocketDriverCommon::SocketType sock, AZStd::chrono::microseconds timeOut);
    }

    class SocketDriver
        : public SocketDriverCommon
    {
        friend class SocketDriverAddress;
    public:
        GM_CLASS_ALLOCATOR(SocketDriver);

        SocketDriver(bool isFullPackets, bool isCrossPlatform, bool isHighPerformance = false)
            : SocketDriverCommon(isFullPackets, isCrossPlatform, isHighPerformance) {}
        virtual ~SocketDriver() {}

        /**
        * Creates internal driver address to be used for send/receive calls.
        * \note if the ip and the port are the same, the same pointer will be returned. You can use the returned pointer
        * to compare for unique addresses.
        * \note Driver address allocates internal resources, use it only when you intend to communicate. Otherwise operate with
        * the string address.
        */
        AZStd::intrusive_ptr<DriverAddress> CreateDriverAddress(const AZStd::string& address) override;
        AZStd::intrusive_ptr<DriverAddress> CreateDriverAddress(const sockaddr* addr) override;
        /// Called only from the DriverAddress when the use count becomes 0
        void    DestroyDriverAddress(DriverAddress* address) override;

        typedef AZStd::unordered_set<SocketDriverAddress, SocketDriverAddress::Hasher> AddressSetType;
        AddressSetType  m_addressMap;
    };

    namespace Utils
    {
        ///< Retrieves ip address corresponding to a host name. Blocks thread until dns resolving is happened.
        bool GetIpByHostName(int familyType, const char* hostName, AZStd::string& ip);
    }

    /**
    * Utility class to help retrieve socket address information
    */
    class SocketAddressInfo
    {
    public:
        SocketAddressInfo();
        ~SocketAddressInfo();

        void Reset();

        enum class AdditionalOptionFlags
        {
            None = 0x00,        // Nothing to specify
            Passive = 0x01,     // For wild card IP address
            NumericHost = 0x02, // then 'address' must be a numerical network address
        };

        /**
        * Resolves the an address for either the local host machine (when address is nullptr) or a remote address where address points to a valid string
        * \param address when nullptr it we will assume "any address".
        * \param port When left 0, we use implicit bind (assigned by the system); in native Endian
        * \param familyType family type, for the BSD socket it can be BSD_AF_INET or BSD_AF_INET6
        * \param isDatagram When True then the address hint with be SOCK_DGRAM otherwise SOCK_STREAM
        * \param flags combined AI_* flags to use as a hints
        */
        bool Resolve(const char* address, AZ::u16 port, Driver::BSDSocketFamilyType familyType, bool isDatagram, AdditionalOptionFlags flags);

        /**
          * If Resolve() is True, then this returns the address information requested to resolve
          */
        const addrinfo* GetAddressInfo() const { return m_addrInfo; }

        /**
          * If Resolve() is True and valid socket, return the assigned port after a succesful bind() call
          */
        AZ::u16 RetrieveSystemAssignedPort(SocketDriverCommon::SocketType socket) const;

    private:
        addrinfo* m_addrInfo;
    };
}


