/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CARRIER_H
#define GM_CARRIER_H

#include <GridMate/Types.h>
#include <GridMate/EBus.h>
#include <GridMate/Carrier/Compressor.h>
#include <GridMate/Carrier/Driver.h>
#include <GridMate/Carrier/TrafficControl.h>

#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/Driller/Driller.h>
#include "AzCore/std/smart_ptr/weak_ptr.h"

namespace GridMate
{
    class IGridMate;

    class CarrierACKCallback
    {
    public:
        virtual void Run() = 0;
        virtual ~CarrierACKCallback() {};
    };

    /**
    * Carrier Interface.
    */
    class Carrier
    {
    public:
        typedef TrafficControl::Statistics Statistics;
        /**
        * Data delivery priorities.
        */
        enum DataPriority
        {
            PRIORITY_SYSTEM,    ///< System priority messages have the highest priority. (reserved for INTERNAL USE)
            PRIORITY_HIGH,      ///< High priority messages are send before normal priority messages.
            PRIORITY_NORMAL,    ///< Normal priority messages are send before low priority messages.
            PRIORITY_LOW,       ///< Low priority messages are only sent when no other messages are waiting.

            PRIORITY_MAX        // Must be last
        };
        /**
        * Data delivery reliability.
        */
        enum DataReliability
        {
            SEND_UNRELIABLE,        ///< Send data unreliable ordered, out of order packets will be dropped
            SEND_RELIABLE,          ///< Send data reliable ordered

            SEND_RELIABILITY_MAX    // Must be last
        };

        enum ConnectionStates
        {
            CST_CONNECTING,
            CST_CONNECTED,
            CST_DISCONNECTING,
            CST_DISCONNECTED,
        };

        struct ReceiveResult
        {
            enum States
            {
                RECEIVED,                   ///< We have received a message and it's payload has been copied to the data buffer. m_numBytes containes number of bytes copied
                UNSUFFICIENT_BUFFER_SIZE,   ///< Destination buffer is not enough, m_numBytes contains the miminum buffer size to receive that message.
                NO_MESSAGE_TO_RECEIVE,      ///< No message ready to be received, m_numBytes should be zero.
            };

            States          m_state;    ///< \ref States
            unsigned int    m_numBytes; ///< Number of bytes received/copied into the data array
        };

        virtual void            Shutdown() = 0;

        virtual ~Carrier() {}

        /// Connect with host and port. This is ASync operation, the connection is active after OnConnectionEstablished is called.
        virtual ConnectionID    Connect(const char* hostAddress, unsigned int port) = 0;
        /// Connect with internal address format. This is ASync operation, the connection is active after OnConnectionEstablished is called.
        virtual ConnectionID    Connect(const AZStd::string& address) = 0;
        /// Request a disconnect procedure. This is ASync operation, the connection is closed after OnDisconnect is called.
        virtual void            Disconnect(ConnectionID id) = 0;

        virtual unsigned int    GetPort() const = 0;

        /// Returns the maximum message size that will fit in one datagram
        virtual unsigned int GetMessageMTU() = 0;

        /// Returns maximum message size (with splitting or without). Splitting will make your message reliable, which might not be optimal for Unreliable messages. It's better to send two unreliable.
        //virtual unsigned int  GetMaxMessageSize(bool withSplitting = true) = 0;

        virtual AZStd::string   ConnectionToAddress(ConnectionID id) = 0;

        /**
        * Sends buffer with an ACK callback. When the transport layer recieves an ACK it will run the callback.
        * The carrier runs in the main game thread, so if the callback executes a function in another thread it is the
        * responsibility of the callback creator to add thread safety.
        *
        * This adds reasonable overhead to the carrier data handling, and so should only be used when a callback is essential to operations.
         *
        * Note: ACK callback is not supported with broadcast targets and will assert.
         */
        virtual void            SendWithCallback(const char* data, unsigned int dataSize, AZStd::unique_ptr<CarrierACKCallback> ackCallback, ConnectionID target = AllConnections, DataReliability reliability = SEND_RELIABLE, DataPriority priority = PRIORITY_NORMAL, unsigned char channel = 0) = 0;
        /**
        * Sends a buffer to the target with the parameterized reliability, priority and channel.
        *
        * Note: Unreliable sends with buffers larger than the MTU will get upgraded to reliable.
        */
        virtual void            Send(const char* data, unsigned int dataSize, ConnectionID target = AllConnections, DataReliability reliability = SEND_RELIABLE, DataPriority priority = PRIORITY_NORMAL, unsigned char channel = 0) = 0;
        /**
         * Receive the data for the specific connection.
         * \note Internal buffers are used make sure you periodically receive data for all connections,
         * otherwise you might cause buffer overflow error.
         */
        virtual ReceiveResult   Receive(char* data, unsigned int maxDataSize, ConnectionID from, unsigned char channel = 0) = 0;

        /**
         * Query the next received message (which can the retreived with receive) maximum size.
         * \note This is NOT always the actually message size, but a big enough buffer (rounded the nearest internal max datagram size)
         * to hold that message.
         */
        unsigned int            QueryNextReceiveMessageMaxSize(ConnectionID from, unsigned char channel = 0)
        {
            return Receive(nullptr, 0, from, channel).m_numBytes;
        }

        // Add new connection pool function or a callback ?

        /**
         * Update must be called once per frame. In processes system messages and callback data from the carrier thread.
         */
        virtual void                Update() = 0;

        virtual unsigned int        GetNumConnections() const = 0;

        //virtual ConnectionStates  GetConnectionState(unsigned int index) const = 0;
        //virtual ConnectionStates  GetConnectionState(ConnectionID id) const = 0;

        struct FlowInformation
        {
            size_t      m_numToSendMessages;
            size_t      m_numToReceiveMessages;

            size_t      m_dataInTransfer;           ///< Current data in transfer (out of the ToSendQueue but NOT confirmed - reliable only)
            size_t      m_congestionWindow;         ///<
        };

        /**
         * Stores connection statistics, it's ok to pass NULL for any of the statistics
         * \param id Connection ID
         * \param lastSecond last second statistics for all data
         * \param lifetime lifetime statistics for all data
         * \param effectiveLastSecond last second statistics for effective data (actual data - carrier overhead excluded)
         * \param effectiveLifetime lifetime statistics for effective data (actual data - carrier overhead excluded)
         * \returns connection state
         */
        virtual ConnectionStates    QueryStatistics(ConnectionID id, TrafficControl::Statistics* lastSecond = nullptr, TrafficControl::Statistics* lifetime = nullptr,
            TrafficControl::Statistics* effectiveLastSecond = nullptr, TrafficControl::Statistics* effectiveLifetime = nullptr,
            FlowInformation* flowInformation = nullptr) = 0;

        /**
         * Debug function, prints the connection status report to the stdout.
         */
        virtual void            DebugStatusReport(ConnectionID id, unsigned char channel = 0)  { (void)id; (void)channel; }
        virtual void            DebugDeleteConnection(ConnectionID id)                          { (void)id; }
        // @{ Debug functions that control detect disconnect at runtime. IMPORTANT: This will override the default setting in the CarrierDescriptor!
        virtual void            DebugEnableDisconnectDetection(bool isEnabled)                  { (void)isEnabled; }
        virtual bool            DebugIsEnableDisconnectDetection() const                        { return false; }
        // @}
        virtual ConnectionID    DebugGetConnectionId(unsigned int index) const = 0;

        //////////////////////////////////////////////////////////////////////////
        // Synchronized clock in milliseconds. It will wrap around ~49.7 days.
        /**
         * Enables sync of the clock every syncInterval milliseconds.
         * Only one peer in the connected grid can have the clock sync enabled, all the others will sync to it,
         * if you enable it on two or more an assert will occur.
         * \note syncInterval is just so the clocks stay is sync, GetTime will adjust the time using the last received sync value (or the creation of the carrier).
         * keep the synIterval high >= 1 sec. Systems will use their internal timer to adjust anyway, this will minimize the bandwidth usage.
         * We adjust the time with the RTT so the accuracy will depend on the RTT and it's fluctuations. On average you can expect accuracy <250 ms (implantation dependent)
         * in practice we can reduce this to <100 but this will require a lot more complex clock scheme.
         */
        virtual void            StartClockSync(unsigned int syncInterval = 1000, bool isReset = false) = 0;
        virtual void            StopClockSync() = 0;
        /**
         * Returns current carrier time in milliseconds. If nobody syncs the clock the time will be relative
         * to the carrier creation.
         */
        virtual AZ::u32         GetTime() = 0;
        //////////////////////////////////////////////////////////////////////////

        /// Returns the max frequency we will grab messages from the queues and send in milliseconds.
        unsigned int            GetMaxSendRate() const      { return m_maxSendRateMS; }

        /// Return the owning instance of the gridmate.
        AZ_FORCE_INLINE IGridMate*       GetGridMate() const         { return m_gridMate; }
    protected:
        explicit Carrier(IGridMate* gridMate)
            : m_gridMate(gridMate)  {}
        Carrier(const Carrier& rhs);
        Carrier& operator=(const Carrier& rhs);

        IGridMate*   m_gridMate;                ///< Pointer to the owning gridmate instance.
        unsigned int m_maxSendRateMS;           ///< Maximum send rate in milliseconds.
        unsigned int m_connectionRetryIntervalBase;
        unsigned int m_connectionRetryIntervalMax;
        unsigned int m_batchPacketCount;        ///< Number of packets queued to force send (rather than wait for m_maxSendRateMS expiration)
    };

    /**
    * Carrier descriptor, required structure when we create a carrier (so
    * we know how to set up all parameters)
    */
    struct CarrierDesc
    {
        CarrierDesc()
            : m_driver(nullptr)
            , m_trafficControl(nullptr)
            , m_handshake(nullptr)
            , m_simulator(nullptr)
            , m_compressionFactory(nullptr)
            , m_familyType(0)
            , m_address(nullptr)
            , m_port(0)
            , m_driverReceiveBufferSize(0)
            , m_driverSendBufferSize(0)
            , m_driverIsFullPackets(false)
            , m_driverIsCrossPlatform(false)
            , m_version(1)
            , m_securityData(nullptr)
            , m_enableDisconnectDetection(true)
            , m_connectionTimeoutMS(5000)
            , m_disconnectDetectionRttThreshold(500.0f)
            , m_disconnectDetectionPacketLossThreshold(0.3f)
            , m_connectionEvaluationThreshold(0.5f)
            , m_threadCpuID(-1)
            , m_threadPriority(-100000)
            , m_threadUpdateTimeMS(30)
            , m_threadInstantResponse(true) //Default true to prevent packet loss
            , m_recvPacketsLimit(0)
            , m_maxConnections(~0u)
            , m_connectionRetryIntervalBase(10)
            , m_connectionRetryIntervalMax(1000)
            , m_sendBatchPacketCount(0)         //0 = instant send; N = wait for N full packets or m_threadUpdateTimeMS timeout
        {}

        // connection params, driver interfaces, status callbacks
        class Driver*                   m_driver;
        class TrafficControl*           m_trafficControl;
        class Handshake*                m_handshake;
        class Simulator*                m_simulator;

        AZStd::shared_ptr<CompressionFactory>       m_compressionFactory;       ///< Abstract factory to provide carrier with compression implementation

        int                             m_familyType;               ///< Family type (this is driver specific value) for default family use 0.
        const char*                     m_address;                  ///< Communication address, when 0 we use any address otherwise we bind a specific one.
        unsigned int                    m_port;                     ///< Communication port. When 0 is implicit port (assigned by the system) or a value for explicit port.
        unsigned int                    m_driverReceiveBufferSize;  ///< Driver receive buffer size (0 uses default buffer size). Used only if m_driver == null.
        unsigned int                    m_driverSendBufferSize;     ///< Driver send buffer size (0 uses default buffer size). Used only if m_driver == null.
        bool                            m_driverIsFullPackets;      ///< Used only for sockets drivers and LAN. Normally an internet packet is ~1500 bytes. With full packets you will enable big packets (64 KB or less) packets (which will fail on internet, but usually ok locally).
        bool                            m_driverIsCrossPlatform;    ///< True if we will need communicate across platforms (need to make sure we use common platform features).

        VersionType                     m_version;                  ///< Carriers with mismatching version numbers are not allowed to connect to each other. Default is 1.

        const char*                     m_securityData;             ///< Pointer to string with security data

        bool                            m_enableDisconnectDetection; ///< Enable/Disable disconnect detection. (should be set to false ONLY for debug purpose)
        unsigned int                    m_connectionTimeoutMS;      ///< Connection timeout in milliseconds
        float                           m_disconnectDetectionRttThreshold; ///< Rtt threshold in milliseconds, connection will be dropped once actual rtt is bigger than this value
        float                           m_disconnectDetectionPacketLossThreshold; ///< Packet loss percentage threshold (0.0..1.0, 1.0 is 100%), connection will be dropped once actual packet loss exceeds this value

        /**
         * When a disconnect condition is detected (packet loss, connection timeout, high RTT, etc.) all other connections will be evaluated
         * What we want to achieve is to disconnect connections in groups, as big as possible. To do so, we use a factor.This factor is percentage (0.00 to 1.00)
         * of the disconnect conditions to be used to determine if a connection is bad. Default factor 0.5.
         * Example: Let's say our connection timeout is 10 sec. We have N connections. We detected a disconnection from connection X reaching a 10 sec limit.
         * if the connectionEvaluationThreshold is let's say 0.5 (default) all connections that we have not heard for 10 * 0.5 = 5 sec we will be disconnected
         * on the spot.
         */
        float                           m_connectionEvaluationThreshold;

        // thread processing
        int  m_threadCpuID;         ///< -1 for no thread use, otherwise the number depends on the platform. \ref AZStd::thread_desc \ref AZ::JobManagerThreadDesc
        int  m_threadPriority;      ///< depends on the platform, value of -100000 means it will inherit calling thead priority.
        // NOTE: May we should group when m_threadUpdateTimeMS is less than 10 ms we switch to m_threadFastResponse = true mode and remove m_threadFastResponse flag.
        int  m_threadUpdateTimeMS;  ///< Thread update time in milliseconds [0,100]. This time in general should be higher than 10 milliseconds. Otherwise it will be more efficient to set m_threadFastResponse
        /**
         * This flag is used to instruct carrier thread to react instantaneously when a data needs to be send/received.
         *
         * By default this flag is false as we would like to process network data every m_threadUpdateTimeMS
         * otherwise to achieve this instant response time (0 latency) we will use more bandwidth because
         * messages will be grouped less efficiently (especially true when you have small messages).
         */
        bool m_threadInstantResponse;

        unsigned int m_recvPacketsLimit; ///< Maximum packets per second allowed to be received from an existing connection
        unsigned int m_maxConnections; ///< maximum number of connections

        unsigned int m_connectionRetryIntervalBase; ///< Base for expotential backoff of connection request retries (ie. if it's 30, will retry connection request with 30, 60, 120, 240 msec, ... delays)
        unsigned int m_connectionRetryIntervalMax; ///< Cap for interval between connection requests
        unsigned int m_sendBatchPacketCount;            ///< Number of packets queued to force send (rather than wait for m_maxSendRateMS expiration)
    };

    /**
     * Default carrier implementation
     */
    class DefaultCarrier
    {
    public:
        static Carrier* Create(const CarrierDesc& desc, IGridMate* gridMate);
        static void     Destroy(Carrier* carrier)           { delete carrier; }
    };

    /**
    * Carrier error codes
    */
    enum class CarrierErrorCode : int
    {
        EC_DRIVER = 0,  ///< Driver layer error.
        EC_SECURITY,    ///< Carrier layer Security error
    };

    /**
    * Driver error
    */
    struct DriverError
    {
        Driver::ErrorCodes m_errorCode; ///< Driver error code, including platform specific error codes.
        //TODO: Session will be closed only if this error is critical.
        //TODO: bool IsCriticalError() const { return true; }
    };

    /**
    * Security error
    */
    struct SecurityError
    {
        enum
        {
            EC_OK = 0,

            EC_UPDATE_TIMEOUT,                  ///< Carrier should be Updated/Ticked in time( the connection timeout value for now)
            EC_BUFFER_READ_OUT_OF_BOUND,        ///< Out of bounds buffer reads
            EC_CHANNEL_ID_OUT_OF_BOUND,         ///< Out of bounds channel id
            EC_MESSAGE_TYPE_NOT_SUPPORTED,      ///< Unsupported message type
            EC_SEQUENCE_NUMBER_OUT_OF_BOUND,    ///< Seq number is far from expected range
            EC_SEQUENCE_NUMBER_DUPLICATED,      ///< Duplicate seq number
            EC_PACKET_RATE_TOO_HIGH,            ///< Packet rate is too high
            EC_DATA_RATE_TOO_HIGH,              ///< Data rate is too high
            EC_INVALID_SOURCE_ADDRESS,          ///< Invalid source address
            EC_DATAGRAM_TOO_LARGE,              ///< datagram exceeds max size
            EC_BAD_PACKET,                      ///< datagram exceeds max size

            // EC_MAX must be last
            EC_MAX                              ///< Max # of types
        } m_errorCode;                          ///< Security error code
    };

    /**
    * Reasons for a disconnect callback to be called.
    */
    enum class CarrierDisconnectReason : AZ::u8
    {
        DISCONNECT_USER_REQUESTED = 0,          ///< The user requested to close the connection.
        DISCONNECT_BAD_CONNECTION,              ///< Traffic conditions are bad to maintain a connection.
        DISCONNECT_BAD_PACKETS,                 ///< We received invalid data packets.
        DISCONNECT_DRIVER_ERROR,
        DISCONNECT_HANDSHAKE_REJECTED,
        DISCONNECT_HANDSHAKE_TIMEOUT,
        /** A connection was initiated while the previous was never closed (properly).
        * As a result the connection will be closed on both sides to synchronize.
        * If you initiated this connection, you should make sure you closed the previous properly
        * and if so you can retry to connect, which will most likely succeed. Since both sides are in sync.
        */
        DISCONNECT_WAS_ALREADY_CONNECTED,
        DISCONNECT_SHUTTING_DOWN,               ///< Carrier is shutting down. You should not have connection at this time to begin with.
        DISCONNECT_DEBUG_DELETE_CONNECTION,

        DISCONNECT_VERSION_MISMATCH,            ///< Attempting to connect to a different application version.

        DISCONNECT_MAX,                         ///< Must be last for internal reasons
    };

    /**
     * Base class for carrier events
     */

    class CarrierEventsBase
    {
    public:
        virtual ~CarrierEventsBase() {}

        AZStd::string ReasonToString(CarrierDisconnectReason reason);
    };

    class CarrierEvents
        : public CarrierEventsBase
        , public GridMateEBusTraits
    {
    public:
        virtual void OnIncomingConnection(Carrier* carrier, ConnectionID id)
        {
            (void)carrier;
            (void)id;
        }

        virtual void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
        {
            (void)carrier;
            (void)id;
            (void)reason;
        }

        virtual void OnConnectionEstablished(Carrier* carrier, ConnectionID id)
        {
            (void)carrier;
            (void)id;
        }

        virtual void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
        {
            (void)carrier;
            (void)id;
            (void)reason;
        }

        /// Report all carrier and driver errors! id == InvalidConnectionID if the error is not connection related!
        virtual void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error)
        {
            (void)carrier;
            (void)id;
            (void)error;
        }
        virtual void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error)
        {
            (void)carrier;
            (void)id;
            (void)error;
        }

        /**
        * Notifies of data rate change
        *
        * The carrier has detected a peer rate change (likely from congestion). Listeners should
        * decrease/increase generation rate to match. This version sends the rate for the lowest
        * rate peer connection, but future versions will update per-peer and add a ConnectionID
        * param.
        *
        * carrier ptr to carrier
        * id connection with rate change
        * sendLimitBytesPerSec new send rate in _bytes_ per second
        */
        virtual void OnRateChange(Carrier* carrier, ConnectionID id, AZ::u32 sendLimitBytesPerSec)
        {
            (void)carrier;
            (void)id;
            (void)sendLimitBytesPerSec;
        };

        /**
         * Notifies of message arrival
         *
         * Note: as with all EBUS functions the callee must add thread safety if required
         *
         * carrier ptr to carrier
         * id connection with new message
         * channel channel receiving message
         */
        virtual void OnReceive(Carrier* carrier, ConnectionID id, unsigned char channel)
        {
            (void)carrier;
            (void)id;
            (void)channel;
        };
    };

    typedef AZ::EBus<CarrierEvents> CarrierEventBus;

    namespace Debug
    {
        class CarrierDrillerEvents
            : public CarrierEventsBase
            , public AZ::Debug::DrillerEBusTraits
        {
        public:
            virtual void OnIncomingConnection(Carrier* carrier, ConnectionID id) = 0;

            virtual void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) = 0;

            virtual void OnConnectionEstablished(Carrier* carrier, ConnectionID id) = 0;

            virtual void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) = 0;

            /// Report all carrier and driver errors! id == InvalidConnectionID if the error is not connection related!
            virtual void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) = 0;
            virtual void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) = 0;

            //////////////////////////////////////////////////////////////////////////
            // Executed from NETWORK thread

            // Driver
            /// SendTo
            /// ReceiveFrom
            /// Errors

            // Traffic control

            /// Called every second when you update last second statistics
            virtual void OnUpdateStatistics(const AZStd::string& address, const TrafficControl::Statistics& lastSecond, const TrafficControl::Statistics& lifeTime, const TrafficControl::Statistics& effectiveLastSecond, const TrafficControl::Statistics& effectiveLifeTime) = 0;

            // Simulator
            /// Enable/Disable
            /// Change Simulator parameters

            // Carrier
            virtual void OnConnectionStateChanged(Carrier* carrier, ConnectionID id, Carrier::ConnectionStates newState) = 0;

            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Executed from GAME/MAIN thread

            // Handshake low level (we drill the handshake on session level too)

            // Carrier - in addition to carrier events

            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus<CarrierDrillerEvents> CarrierDrillerBus;
    }
}

#endif // GM_CARRIER_H
