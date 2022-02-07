/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SECURE_SOCKET_DRIVER_H
#define GM_SECURE_SOCKET_DRIVER_H

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/State/HSM.h>

#include <GridMate/Carrier/SocketDriver.h>

#define AZ_DebugSecureSocket(...)
#define AZ_DebugSecureSocketConnection(window, fmt, ...)

//#define AZ_DebugUseSocketDebugLog
//#define AZ_DebugSecureSocket AZ_TracePrintf
//#define AZ_DebugSecureSocketConnection(window, fmt, ...) \
//{\
//    AZStd::string line = AZStd::string::format(fmt, __VA_ARGS__);\
//    this->m_dbgLog += line;\
//}

#if AZ_TRAIT_GRIDMATE_SECURE_SOCKET_DRIVER_HOOK_ENABLED
struct ssl_st;
struct ssl_ctx_st;
struct dh_st;
struct bio_st;
struct x509_store_ctx_st;
struct evp_pkey_st;
struct x509_st;
struct x509_store_ct;

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct dh_st DH;
typedef struct bio_st BIO;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_st X509;

namespace GridMate
{
    static const int COOKIE_SECRET_LENGTH = 16; //128 bit key
    static const int MAX_COOKIE_LENGTH = 255; // largest length that will fit in one byte

    namespace ConnectionSecurity
    {
        bool IsHandshake(const char* data, AZ::u32 dataSize);
        bool IsClientHello(const char* data, AZ::u32 dataSize);
        bool IsChangeCipherSpec(const char* data, AZ::u32 dataSize);
        bool IsHelloVerifyRequest(const char* data, AZ::u32 dataSize);
        bool IsHelloRequestHandshake(const char* data, AZ::u32 dataSize);
        const char* TypeToString(const char* data, AZ::u32 dataSize);
    }

    struct SecureSocketDesc
    {
        SecureSocketDesc()
            : m_connectionTimeoutMS(5000)
            , m_authenticateClient(false)
            , m_maxDTLSConnectionsPerIP(~0u)
            , m_privateKeyPEM(nullptr)
            , m_certificatePEM(nullptr)
            , m_certificateAuthorityPEM(nullptr) {};

        AZ::u64     m_connectionTimeoutMS;
        bool        m_authenticateClient;          // Ensure that a client must be authenticated (the server is
                                                   // always authenticated). Only required to be set on the server!
        unsigned int m_maxDTLSConnectionsPerIP;    // Max number of DTLS connections that can be accepted per ip
        const char* m_privateKeyPEM;               // A base-64 encoded PEM format private key.
        const char* m_certificatePEM;              // A base-64 encoded PEM format certificate.
        const char* m_certificateAuthorityPEM;     // A base-64 encoded PEM format CA root certificate.
    };

    /**
    * A driver implementation that encrypts and decrypts data sent between the application
    * and the underlying socket. The driver depends on a socket being successfully created
    * and bound to a port so it derives from the existing SocketDriver implementation (this
    * approach also makes SecureSocketDriver fairly platform agnostic).
    *
    * In order to establish a secure channel between two peers a formal connection needs to be
    * created and a TLS handshake performed. During this handshake a cipher is agreed upon, a
    * shared symmetric key generated and peers authenticated.
    *
    * Connections are created when sending or receiving a packet from a peer for the first time
    * and removed when explicitly disconnected or times out.
    *
    * The driver API is stateless so a user needn't know about the internal connections to
    * remote peers. The user simply sends and receive datagrams as normal to endpoints on the network.
    * All user datagrams sent during the connection handshake are queued up and sent encrypted when
    * the connection has been successfully established.
    */
    class SecureSocketDriver
        : public SocketDriver
    {
    public:
        GM_CLASS_ALLOCATOR(SecureSocketDriver);

        typedef AZStd::intrusive_ptr<DriverAddress> AddrPtr;
        typedef AZStd::vector<char>                 Datagram;
        typedef AZStd::pair<Datagram, AddrPtr>      DatagramAddr;

        SecureSocketDriver(const SecureSocketDesc& desc, bool isFullPackets = false, bool crossPlatform = false, bool isHighPerformance = true);
        virtual ~SecureSocketDriver();
        static void apps_ssl_info_callback(const SSL *s, int where, int ret);

        AZ::u32 GetMaxSendSize() const override;

        Driver::ResultCode Initialize(AZ::s32 ft, const char* address, AZ::u32 port, bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize) override;
        void               Update() override;
        void               ProcessIncoming() override;
        void               ProcessOutgoing() override;
        Driver::ResultCode Receive(char* data, AZ::u32 maxDataSize, AddrPtr& from, ResultCode* resultCode) override;
        AZ::u32            Send(const AddrPtr& to, const char* data, AZ::u32 dataSize) override;

    protected:
        enum ConnectionState
        {
            CS_TOP,
                CS_ACTIVE,                              // Processing datagrams
                    CS_SEND_HELLO_REQUEST,              // S: Waiting for client to start TLS/DTLS handshake
                    CS_ACCEPT,                          // S: Performing TLS/DTLS handshake for an incoming connection.
                    CS_COOKIE_EXCHANGE,                 // C: Performing cookie verification
                    CS_CONNECT,                         // C: Performing TLS/DTLS handshake for an outgoing connection.
                    CS_ESTABLISHED,                     // Both: SSL handshake succeeded.
                CS_DISCONNECTED,                        // Both: Disconnected.
            CS_MAX
        };

        /**
        * Manage a single DTLS connection to a remote peer. It is ideally backed by two buffers
        * that will contain ciphertext and two queues that contain plaintext.
        *
        * There are two flows of data:
        *  - Plaintext is pulled from the out queue, encrypted, and ciphertext written to the out buffer.
        *  - Ciphertext is read from the in buffer, decrypted, and plaintext added to the in queue.
        *
        * In practice there is two buffers of ciphertext and only one queue that contains plaintext.
        *
        * As an optimization, the connection does not hold it's own plaintext in queue. Since the
        * user is going to be polling the single driver for plaintext datagrams it is better to
        * add all decrypted datagrams to a single, shared queue for the driver to pull from.
        *
        * Connections have their own timeout which is set during construction. The connection will
        * be disconnected on a timeout and no further communication will be possible.
        */
        class Connection
        {
        public:
            GM_CLASS_ALLOCATOR(Connection);
            Connection(const AddrPtr& addr, AZ::u32 bufferSize, AZStd::queue<DatagramAddr>* inQueue, AZ::u64 timeoutMS, int port);
            virtual ~Connection();

            bool    Initialize(SSL_CTX* sslContext, ConnectionState startState, AZ::u32 mtu);
            void    Shutdown();

            void    Update();
            void    AddDgram(const char* data, AZ::u32 dataSize);
            void    ProcessIncomingDTLSDgram(const char* data, AZ::u32 dataSize);
            AZ::u32 GetDTLSDgram(char* data, AZ::u32 dataSize);
            void    FlushOutgoingDTLSDgrams();
            bool    IsDisconnected() const;

            void ForceDTLSTimeout();
            bool CreateSSL(SSL_CTX* sslContext);
            bool DestroySSL();
            const SSL* GetSSL() { return m_ssl; }

        private:
            bool OnStateActive(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateSendHelloRequest(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateCookieExchange(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool HandleSSLError(AZ::s32 result);

            // Read a DTLS record (datagram) from a BIO buffer. Return true if records were read and stored
            // in outDgramList, or false if nothing was found.
            bool ReadDgramFromBuffer(BIO* buffer, AZStd::vector<SecureSocketDriver::Datagram>& outDgramQueue);

            // Queue outbound Datagrams from SSL BIO into m_outDTLSQueue
            int QueueDatagrams();

            enum ConnectionEvents
            {
                CE_UPDATE = 1,
                CE_STATEFUL_HANDSHAKE,
                CE_COOKIE_EXCHANGE_COMPLETED,
                CE_NEW_INCOMING_DGRAM,
                CE_NEW_OUTGOING_DGRAM,
            };

            bool m_isInitialized;
            TimeStamp m_creationTime;    // The time the connection reached the established state.
            AZ::u64 m_timeoutMS;
            AZStd::queue<Datagram>          m_outboundPlainQueue;   // Outbound plaintext datagrams from application
            BIO*                            m_outDTLSBuffer;        // Outbound DTLS serialization buffer ready for -> m_outDTLSQueue
            AZStd::queue<Datagram>          m_outDTLSQueue;         // Outbound DTLS datagrams ready for Socket Send
            BIO*                            m_inDTLSBuffer;         // Inbound DTLS decryption buffer ready for -> m_inboundPlaintextQueue
            AZStd::queue<DatagramAddr>*     m_inboundPlaintextQueue;// Inbound plaintext datagrams ready for application read
            AZ::HSM m_sm;
            SSL* m_ssl;
            SSL_CTX* m_sslContext;
            AddrPtr m_addr;
            AZ::u32 m_maxTempBufferSize;
            AZ::s32 m_sslError;
            AZ::u32 m_mtu;

            AZStd::chrono::system_clock::time_point m_nextHelloRequestResend;
            AZStd::chrono::milliseconds k_initialHelloRequestResendInterval = AZStd::chrono::milliseconds(100);
            AZStd::chrono::milliseconds m_helloRequestResendInterval;
            AZStd::chrono::system_clock::time_point m_nextHandshakeRetry;

        public:
            int m_dbgDgramsSent;
            int m_dbgDgramsReceived;
            int m_dbgPort;
#ifdef AZ_DebugUseSocketDebugLog
            AZStd::string m_dbgLog;
#endif
        };

        bool RotateCookieSecret(bool bForce = false);
        static int VerifyCertificate(int ok, X509_STORE_CTX* ctx);
        int GenerateCookie(AddrPtr endpoint, unsigned char* cookie, unsigned int* cookieLen);
        int VerifyCookie(AddrPtr endpoint, unsigned char* cookie, unsigned int cookieLen);

        void FlushSocketToConnectionBuffer();
        void UpdateConnections();
        void FlushConnectionBuffersToSocket();

        EVP_PKEY* m_privateKey;
        X509* m_certificate;
        SSL_CTX* m_sslContext;
        char* m_tempSocketWriteBuffer;
        char* m_tempSocketReadBuffer;

        struct GridMateSecret
        {
            TimeStamp m_lastSecretGenerationTime;
            unsigned char m_currentSecret[COOKIE_SECRET_LENGTH];
            unsigned char m_previousSecret[COOKIE_SECRET_LENGTH];
            bool m_isCurrentSecretValid;
            bool m_isPreviousSecretValid;

            GridMateSecret()
                : m_isCurrentSecretValid(false)
                , m_isPreviousSecretValid(false)
            { }
        } m_cookieSecret;

        AZ::u32 m_maxTempBufferSize;
        AZStd::queue<DatagramAddr> m_globalInQueue;
        AZStd::unordered_map<SocketDriverAddress, Connection*, SocketDriverAddress::Hasher> m_connections;
        AZStd::unordered_map<AZStd::string, int> m_ipToNumConnections;
        SecureSocketDesc m_desc;
        AZStd::chrono::system_clock::time_point m_lastTimerCheck;       ///Time last timers were checked
    };
}

#endif
#endif
