/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/string/memorytoascii.h>
#include <GridMate/Carrier/SecureSocketDriver.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>

#include <GridMate_Traits_Platform.h>

//#define PRINT_IPADDRESS
//#define DEBUG_CERTIFICATE_CHAIN_ENCODE 1

#if AZ_TRAIT_GRIDMATE_SECURE_SOCKET_DRIVER_HOOK_ENABLED
#include <openssl/ossl_typ.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

namespace GridMate
{
    namespace
    {
        const AZ::u32 kSSLContextDriverPtrArg = 0;
        const AZ::u32 kDTLSSecretExpirationTime = 1000 * 30; // 30 Seconds
        const AZ::u32 kSSLHandshakeAttempts = 10;
    }

    namespace Internal
    {
        const char *SafeGetAddress(const AZStd::string& addr)
        {
        #ifdef PRINT_IPADDRESS
            return addr.c_str();
        #else
            // can't display real addresses, so lets just display a ...|0
            (void)addr;
            return "x.x.x.x|x";
        #endif
        }
    }

    namespace ConnectionSecurity
    {
        const AZ::u8 kExpectedCookieSize = 20;

        // Unpacking
        // 
        template<typename TOut, size_t TBitsOffset>
        AZ_INLINE static TOut UnpackByte(ReadBuffer& input)
        {
            if (input.IsValid())
            {
                AZ::u8 byte;
                input.Read(byte);
                TOut value = static_cast<TOut>(byte) << TBitsOffset;
                return value;
            }
            AZ_TracePrintf("GridMateSecure", "Read buffer unpacked too many bytes.");
            return 0;
        }

        AZ_INLINE static void UnpackNetwork1ToHost1(ReadBuffer& input, AZ::u8& value)
        {
            value = UnpackByte<AZ::u8, 0>(input);
        }

        AZ_INLINE static void UnpackNetwork2ToHost2(ReadBuffer& input, AZ::u16& value)
        {
            if (input.IsValid())
            {
                input.Read(value);
            }
        }

        AZ_INLINE static void UnpackNetwork3ToHost4(ReadBuffer& input, AZ::u32& value)
        {
            value = UnpackByte<AZ::u32, 16>(input)
                  | UnpackByte<AZ::u32, 8 >(input)
                  | UnpackByte<AZ::u32, 0 >(input);
        }

        AZ_INLINE static void UnpackNetwork6ToHost8(ReadBuffer& input, AZ::u64& value)
        {
            value = UnpackByte<AZ::u64, 40>(input)
                  | UnpackByte<AZ::u64, 32>(input)
                  | UnpackByte<AZ::u64, 24>(input)
                  | UnpackByte<AZ::u64, 16>(input)
                  | UnpackByte<AZ::u64, 8 >(input)
                  | UnpackByte<AZ::u64, 0 >(input);
        }

        AZ_INLINE static void UnpackOpaque(ReadBuffer& input, uint8_t* bytes, size_t bytesSize)
        {
            if (input.IsValid())
            {
                input.ReadRaw(bytes, bytesSize);
            }
        }

        AZ_INLINE static void UnpackRange(ReadBuffer& input, AZ::u8 minBytes, AZ::u8 maxBytes, AZ::u8* output, AZ::u8& outputSize)
        {
            if (input.IsValid())
            {
                outputSize = 0;
                UnpackNetwork1ToHost1(input, outputSize);
                if (outputSize < minBytes || outputSize > maxBytes)
                {
                    AZ_TracePrintf("GridMate", "Unpack out of range");
                }
                if (outputSize > 0)
                {
                    input.ReadRaw(output, outputSize);
                }
            }
        }

        // Packing
        //
        template<typename TInput, size_t TBytePos>
        AZ_INLINE static AZ::u8 PackByte(TInput input)
        {
            return static_cast<AZ::u8>(input >> (8 * TBytePos));
        }

        AZ_INLINE static void PackHost4Network3(AZ::u32 value, WriteBuffer& writeBuffer)
        {
            writeBuffer.Write(PackByte<AZ::u32, 2>(value));
            writeBuffer.Write(PackByte<AZ::u32, 1>(value));
            writeBuffer.Write(PackByte<AZ::u32, 0>(value));
        }

        AZ_INLINE static void PackHost8Network6(AZ::u64 value, WriteBuffer& writeBuffer)
        {
            writeBuffer.Write(PackByte<AZ::u64, 5>(value));
            writeBuffer.Write(PackByte<AZ::u64, 4>(value));
            writeBuffer.Write(PackByte<AZ::u64, 3>(value));
            writeBuffer.Write(PackByte<AZ::u64, 2>(value));
            writeBuffer.Write(PackByte<AZ::u64, 1>(value));
            writeBuffer.Write(PackByte<AZ::u64, 0>(value));
        }

        // Structures
        //
        struct RecordHeader                 // 13 bytes = DTLS1_RT_HEADER_LENGTH
        {
            const AZ::u32 kExpectedSize;
                                             // offset  size
            AZ::u8  m_type;                  // [ 0]    1
            AZ::u16 m_version;               // [ 1]    2
            AZ::u16 m_epoch;                 // [ 3]    2
            AZ::u64 m_sequenceNumber;        // [ 5]    6
            AZ::u16 m_length;                // [11]    2
            
            RecordHeader()
                : kExpectedSize(13)
            {
            }
            RecordHeader& operator= (const RecordHeader& other) = delete;

            bool Unpack(ReadBuffer& readBuffer)
            {
                UnpackNetwork1ToHost1(readBuffer, m_type);
                UnpackNetwork2ToHost2(readBuffer, m_version);
                UnpackNetwork2ToHost2(readBuffer, m_epoch);
                UnpackNetwork6ToHost8(readBuffer, m_sequenceNumber);
                UnpackNetwork2ToHost2(readBuffer, m_length);
                return readBuffer.IsValid();
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                writeBuffer.Write(m_type);
                writeBuffer.Write(m_version);
                writeBuffer.Write(m_epoch);
                PackHost8Network6(m_sequenceNumber, writeBuffer);
                writeBuffer.Write(m_length);
                return writeBuffer.Size() == kExpectedSize;
            }
        };

        struct HandshakeHeader              // 12 bytes = DTLS1_HM_HEADER_LENGTH
            : public RecordHeader
        {
            const AZ::u32 kExpectedSize;
                                             // offset  size
            AZ::u8  m_hsType;                // [13]    1
            AZ::u32 m_hsLength;              // [14]    3
            AZ::u16 m_hsSequence;            // [17]    2
            AZ::u32 m_hsFragmentOffset;      // [19]    3
            AZ::u32 m_hsFragmentLength;      // [22]    3

            HandshakeHeader()
                : kExpectedSize(12)
            {
            }
            
            bool Unpack(ReadBuffer& readBuffer)
            {
                if (!RecordHeader::Unpack(readBuffer))
                {
                    return false;
                }
                UnpackHandshake(readBuffer);
                return readBuffer.IsValid();
            }

            bool UnpackHandshake(ReadBuffer& readBuffer)
            {
                UnpackNetwork1ToHost1(readBuffer, m_hsType);
                UnpackNetwork3ToHost4(readBuffer, m_hsLength);
                UnpackNetwork2ToHost2(readBuffer, m_hsSequence);
                UnpackNetwork3ToHost4(readBuffer, m_hsFragmentOffset);
                UnpackNetwork3ToHost4(readBuffer, m_hsFragmentLength);
                return readBuffer.IsValid();
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                if (!RecordHeader::Pack(writeBuffer))
                {
                    return false;
                }
                writeBuffer.Write(m_hsType);
                PackHost4Network3(m_hsLength, writeBuffer);
                writeBuffer.Write(m_hsSequence);
                PackHost4Network3(m_hsFragmentOffset, writeBuffer);
                PackHost4Network3(m_hsFragmentLength, writeBuffer);
                return writeBuffer.Size() == kExpectedSize + RecordHeader::kExpectedSize;
            }
        };

        struct ClientHello               // 56 + other stuff the client sent... the headers and cookie are the only things considered
            : public HandshakeHeader
        {
            const AZ::u32 kBaseExpectedSize;

            AZ::u16 m_clientVersion;              // [25] 2
            AZ::u8 m_randomBytes[32];             // [27] 32
            AZ::u8 m_sessionSize;                 // [29] 1 (should be zero value)
            AZ::u8 m_sessionId[32];               // [__] 0 (Client Hello should not have any session data)
            AZ::u8 m_cookieSize;                  // [30] 1 (normally the value is kExpectedCookieSize)
            AZ::u8 m_cookie[MAX_COOKIE_LENGTH];   // [31] 0 up to 255

            ClientHello()
                : kBaseExpectedSize(sizeof(m_clientVersion) + sizeof(m_randomBytes) + sizeof(m_sessionSize) + 0 + sizeof(m_cookieSize))
            {
            }

            bool Unpack(ReadBuffer& readBuffer)
            {
                if (!HandshakeHeader::Unpack(readBuffer))
                {
                    return false;
                }

                memset(m_sessionId, 0, sizeof(m_sessionId));
                memset(m_randomBytes, 0, sizeof(m_randomBytes));
                memset(m_cookie, 0, sizeof(m_cookie));

                UnpackNetwork2ToHost2(readBuffer, m_clientVersion);
                UnpackOpaque(readBuffer, &m_randomBytes[0], sizeof(m_randomBytes));
                UnpackRange(readBuffer, 0, sizeof(m_sessionId), &m_sessionId[0], m_sessionSize);
                UnpackRange(readBuffer, 0, static_cast<AZ::u8>(MAX_COOKIE_LENGTH), &m_cookie[0], m_cookieSize);
                return readBuffer.IsValid() && readBuffer.Read() == (HandshakeHeader::kExpectedSize + RecordHeader::kExpectedSize + kBaseExpectedSize + m_cookieSize);
            }
        };

        struct HelloVerifyRequest                   // 25 = 3 + sizeof cookie (kExpectedCookieSize)
            : public HandshakeHeader
        {
            const AZ::u32 kFragmentLength;

            AZ::u16 m_serverVersion;                 // [26] 2
            AZ::u8 m_cookieSize;                     // [27] 1
            AZ::u8 m_cookie[MAX_COOKIE_LENGTH];      // [29] up to 255

            HelloVerifyRequest() 
                // The server_version field has the same syntax as in TLS.However, in order to avoid the requirement to do 
                // version negotiation in the initial handshake, DTLS 1.2 server implementations SHOULD use DTLS version 1.0 
                // regardless of the version of TLS that is expected to be negotiated.
                : kFragmentLength(sizeof(m_serverVersion) + sizeof(m_cookieSize) + kExpectedCookieSize)
                , m_serverVersion(DTLS1_VERSION)
                , m_cookieSize(kExpectedCookieSize)
            {
                RecordHeader::m_type = SSL3_RT_HANDSHAKE;
                RecordHeader::m_version = DTLS1_VERSION;
                RecordHeader::m_epoch = 0;
                RecordHeader::m_sequenceNumber = 0;
                RecordHeader::m_length = static_cast<AZ::u16>(DTLS1_HM_HEADER_LENGTH + kFragmentLength);

                HandshakeHeader::m_hsLength = kFragmentLength;
                HandshakeHeader::m_hsType = DTLS1_MT_HELLO_VERIFY_REQUEST;
                HandshakeHeader::m_hsSequence = 0;
                HandshakeHeader::m_hsFragmentOffset = 0;
                HandshakeHeader::m_hsFragmentLength = kFragmentLength;
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                if (!HandshakeHeader::Pack(writeBuffer))
                {
                    return false;
                }
                writeBuffer.Write(m_serverVersion);
                writeBuffer.Write(static_cast<AZ::u8>(m_cookieSize));
                writeBuffer.WriteRaw(&m_cookie[0], m_cookieSize);
                return writeBuffer.Size() == (HandshakeHeader::kExpectedSize + RecordHeader::kExpectedSize + kFragmentLength);
            }
        };

        struct HelloRequest          // 0 (headers only)
            : public HandshakeHeader
        {
            HelloRequest()
            {
                RecordHeader::m_type = SSL3_RT_HANDSHAKE;
                RecordHeader::m_version = DTLS1_VERSION;
                RecordHeader::m_epoch = 0;
                RecordHeader::m_sequenceNumber = 0;
                RecordHeader::m_length = static_cast<AZ::u16>(DTLS1_HM_HEADER_LENGTH);

                HandshakeHeader::m_hsLength = 0;
                HandshakeHeader::m_hsType = SSL3_MT_HELLO_REQUEST;
                HandshakeHeader::m_hsSequence = 0;
                HandshakeHeader::m_hsFragmentOffset = 0;
                HandshakeHeader::m_hsFragmentLength = 0;
            }

            bool Pack(WriteBuffer& writeBuffer) const
            {
                return HandshakeHeader::Pack(writeBuffer);
            }
        };

        // Constants
        //
        const AZ::u32 kMaxPacketSize = sizeof(HelloVerifyRequest); // largest to be written to client

        // Functions
        //
        bool IsHandshake(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
            return dataSize >= (DTLS1_RT_HEADER_LENGTH + DTLS1_HM_HEADER_LENGTH)
                && bytes[0] == SSL3_RT_HANDSHAKE
                && bytes[1] == DTLS1_VERSION_MAJOR;
        }
        bool IsChangeCipherSpec(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
            return dataSize == 14
                && bytes[0] == SSL3_RT_CHANGE_CIPHER_SPEC
                && bytes[1] == DTLS1_VERSION_MAJOR;
        }

        bool IsClientHello(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
            return IsHandshake(data, dataSize)
                && bytes[DTLS1_RT_HEADER_LENGTH] == SSL3_MT_CLIENT_HELLO;
        }

        bool IsHelloVerifyRequest(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
            return IsHandshake(data, dataSize)
                && bytes[DTLS1_RT_HEADER_LENGTH] == DTLS1_MT_HELLO_VERIFY_REQUEST;
        }

        bool IsHelloRequestHandshake(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
            return IsHandshake(data, dataSize)
                && dataSize == DTLS1_RT_HEADER_LENGTH + DTLS1_HM_HEADER_LENGTH
                && bytes[DTLS1_RT_HEADER_LENGTH] == SSL3_MT_HELLO_REQUEST
                && bytes[DTLS1_RT_HEADER_LENGTH + 1] == 0
                && bytes[DTLS1_RT_HEADER_LENGTH + 2] == 0
                && bytes[DTLS1_RT_HEADER_LENGTH + 3] == 0;
        }

        const char* TypeToString(const char* data, AZ::u32 dataSize)
        {
            const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);

            if(!IsHandshake(data, dataSize) && !IsChangeCipherSpec(data, dataSize))
            {
                switch(bytes[0])
                {
                case SSL3_RT_APPLICATION_DATA:  return "AppData";
                case DTLS1_RT_HEARTBEAT:        return "HeartBeat";
                case SSL3_RT_ALERT:             return "Alert";
                default:                        return "Unknown";
                }
            }

            switch (bytes[DTLS1_RT_HEADER_LENGTH])
            {
            case SSL3_MT_HELLO_REQUEST:                 return "HelloRequest";
            case SSL3_MT_CLIENT_HELLO | SSL3_MT_CCS:    return IsChangeCipherSpec(data, dataSize) ?
                                                                "ChangeCipherSpec" : "ClientHello";
            case SSL3_MT_SERVER_HELLO:                  return "ServerHello";
            case SSL3_MT_NEWSESSION_TICKET:             return "NewSessionTicket";
            case SSL3_MT_CERTIFICATE:                   return "Certificate";
            case SSL3_MT_SERVER_KEY_EXCHANGE:           return "ServerKeyExch";
            case SSL3_MT_CERTIFICATE_REQUEST:           return "CertRequest";
            case SSL3_MT_SERVER_DONE:                   return "ServerDone";
            case SSL3_MT_CERTIFICATE_VERIFY:            return "CertVerify";
            case SSL3_MT_CLIENT_KEY_EXCHANGE:           return "ClientKeyExch";
            case SSL3_MT_FINISHED:                      return "Finished";
            case SSL3_MT_CERTIFICATE_STATUS:            return "CertStatus";
            case DTLS1_MT_HELLO_VERIFY_REQUEST:         return "HelloVerifyReq";
            default:
                //AZ_TracePrintf("GridMate", "%s\n", AZStd::MemoryToASCII::ToString(data, dataSize, 256).c_str());
                return "Unknown Handshake/CCS";
            }
        }

        enum class NextAction
        {
            Error,
            VerifyCookie,
            SendHelloVerifyRequest
        };


        NextAction DetermineHandshakeState(char* ptr, AZ::u32 bytesReceived)
        {
            if (!IsClientHello(ptr, bytesReceived))
            {
                return NextAction::Error;
            }

            ReadBuffer buffer(EndianType::BigEndian, ptr, bytesReceived);
            ClientHello clientHello;
            if (!clientHello.Unpack(buffer))
            {
                return NextAction::Error;
            }

            if (clientHello.m_version != DTLS1_VERSION &&
                clientHello.m_version != DTLS1_2_VERSION)
            {
                return NextAction::Error;
            }
            if (clientHello.m_length > bytesReceived)
            {
                return NextAction::Error;
            }

            if (clientHello.m_hsType == SSL3_MT_CLIENT_HELLO)
            {
                // RFC-6347
                // The first message each side transmits in each handshake always has message_seq = 0.  Whenever each new message is generated, the
                // message_seq value is incremented by one.Note that in the case of a re-handshake, this implies that the HelloRequest will have message_seq
                // = 0 and the ServerHello will have message_seq = 1.  When a message is retransmitted, the same message_seq value is used.

                if (clientHello.m_hsSequence == 0)
                {
                    // is the ClientHello(0)
                    //   1. send back HelloVerifyRequest
                    return NextAction::SendHelloVerifyRequest;
                }
                else if (clientHello.m_hsSequence == 1)
                {
                    // is the ClientHello(1)
                    //   1. check for cookie
                    // if all good then:
                    //   1. Send back HelloRequest to restart the HelloClient sequence
                    //   2. Prepare a new connection for the remote address
                    //   3. process the DTLS sequence for ssl_accept latter on during CS_ACCEPT
                    if (clientHello.m_cookieSize == kExpectedCookieSize)
                    {
                        return NextAction::VerifyCookie;
                    }
                }
            }
            return NextAction::Error;
        }
    }

    X509* CreateCertificateFromEncodedPEM(const char* encodedPem)
    {
        BIO* tempBio = BIO_new(BIO_s_mem());
        BIO_puts(tempBio, encodedPem);
        X509* certificate = PEM_read_bio_X509(tempBio, nullptr, nullptr, nullptr);
        BIO_free(tempBio);
        return certificate;
    }

    EVP_PKEY* CreatePrivateKeyFromEncodedPEM(const char* encodedPem)
    {
        BIO* tempBio = BIO_new(BIO_s_mem());
        BIO_puts(tempBio, encodedPem);
        EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(tempBio, nullptr, nullptr, nullptr);
        BIO_free(tempBio);
        return privateKey;
    }


#if DEBUG_CERTIFICATE_CHAIN_ENCODE
    AZStd::string X509NameToString(X509_NAME *subj_or_issuer)
    {
        BIO * bio_out = BIO_new(BIO_s_mem());
        X509_NAME_print(bio_out, subj_or_issuer, 0);
        BUF_MEM *bio_buf;
        BIO_get_mem_ptr(bio_out, &bio_buf);
        AZStd::string issuer = string(bio_buf->data, bio_buf->length);
        BIO_free(bio_out);
        return issuer;
    }

    AZStd::string X509IntegerToString(const ASN1_INTEGER *bs)
    {
        AZStd::string ashex;

        for (int i = 0; i < bs->length; i++)
        {
            ashex += AZStd::string::format("%02x", bs->data[i]);
        }

        return ashex;
    }

    AZStd::string X509DateToString(const ASN1_TIME *time)
    {
        int year=0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
        const char* str = (const char*)time->data;
        size_t i = 0;

        if( ((time->type == V_ASN1_UTCTIME) && (time->length < 13)) ||
            ((time->type == V_ASN1_GENERALIZEDTIME) && (time->length < 17)) )
        {
            return "??";
        }

        if (time->type == V_ASN1_UTCTIME)
        {
            year = (str[i++] - '0') * 10;
            year += (str[i++] - '0');
            year += (year < 70 ? 2000 : 1900);
        }
        else if (time->type == V_ASN1_GENERALIZEDTIME)
        {
            year = (str[i++] - '0') * 1000;
            year += (str[i++] - '0') * 100;
            year += (str[i++] - '0') * 10;
            year += (str[i++] - '0');
        }

        month = (str[i++] - '0') * 10;
        month += (str[i++] - '0') - 1; // -1 since January is 0 not 1.
        day = (str[i++] - '0') * 10;
        day += (str[i++] - '0');
        hour = (str[i++] - '0') * 10;
        hour += (str[i++] - '0');
        minute = (str[i++] - '0') * 10;
        minute += (str[i++] - '0');
        second = (str[i++] - '0') * 10;
        second += (str[i++] - '0');

        return AZStd::string::format("%02d-%02d-%4d %02d:%02d:%02d", month, day, year, hour, minute, second);
    }
#endif

    void CreateCertificateChainFromEncodedPEM(const char* encodedPem, AZStd::vector<X509*>& certificateChain)
    {
        const char startCertHeader[] = "-----BEGIN CERTIFICATE-----";
        const char endCertHeader[] = "-----END CERTIFICATE-----";
        const size_t endCertHeaderLen = strlen(endCertHeader);

        size_t offset = 0;
        AZStd::string encodedPemStr(encodedPem);
        while (true)
        {
            size_t beginStartIdx = encodedPemStr.find(startCertHeader, offset);
            size_t endStartIdx = encodedPemStr.find(endCertHeader, offset);

            if (beginStartIdx == AZStd::string::npos ||
                endStartIdx == AZStd::string::npos ||
                beginStartIdx >= endStartIdx)
            {
                break;
            }

            size_t certLen = (endStartIdx - beginStartIdx) + endCertHeaderLen;
            AZStd::string encodedPemCertStr = encodedPemStr.substr(beginStartIdx, certLen);
            X509* newCertificate = CreateCertificateFromEncodedPEM(encodedPemCertStr.c_str());
            if (newCertificate == nullptr)
            {
                AZ_Warning("GridMateSecure", false, "Could not create certificate from PEM data!\n");
                break;
            }

#if DEBUG_CERTIFICATE_CHAIN_ENCODE
            // We don't want this in client logs normally; it's left here purely debugging purposes
            AZ_Printf("GridMateSecure", "Certinfo: Issuer:\"%s\" Serial:%s Not Before:%s Not After:%s\n",
                X509NameToString(X509_get_issuer_name(newCertificate)).c_str(),
                X509IntegerToString(X509_get_serialNumber(newCertificate)).c_str(),
                X509DateToString(X509_get0_notBefore(newCertificate)).c_str(),
                X509DateToString(X509_get0_notAfter(newCertificate)).c_str()
                );
#endif

            certificateChain.push_back(newCertificate);

            offset = (endStartIdx + endCertHeaderLen);
        }
    }

    SecureSocketDriver::Connection::Connection(const AddrPtr& addr, AZ::u32 bufferSize, AZStd::queue<DatagramAddr>* inQueue, AZ::u64 timeoutMS, int port)
        : m_isInitialized(false)
        , m_timeoutMS(timeoutMS)
        , m_inDTLSBuffer(nullptr)
        , m_outDTLSBuffer(nullptr)
        , m_inboundPlaintextQueue(inQueue)
        , m_ssl(nullptr)
        , m_sslContext(nullptr)
        , m_addr(addr)
        , m_maxTempBufferSize(bufferSize)
        , m_sslError(SSL_ERROR_NONE)
        , m_mtu(576) // RFC-791
        , m_dbgDgramsSent(0)
        , m_dbgDgramsReceived(0)
        , m_dbgPort((port&0xFF)<<8 | (port&0xFF00)>>8)
    {
    }

    SecureSocketDriver::Connection::~Connection()
    {
#ifdef AZ_DebugUseSocketDebugLog
        if (m_sm.GetCurrentState() != CS_ESTABLISHED)
        {
            AZ_TracePrintf("GridMate", "DTOR Disconnected LOG: %s\n", m_dbgLog.c_str());
        }
#endif
        Shutdown();
    }

    bool SecureSocketDriver::Connection::Initialize(SSL_CTX* sslContext, ConnectionState startState, AZ::u32 mtu)
    {
        AZ_Assert(!m_isInitialized, "SecureSocket connection object is already initialized!");
        AZ_Assert(startState == ConnectionState::CS_SEND_HELLO_REQUEST || startState == ConnectionState::CS_COOKIE_EXCHANGE, "SecureSocket connection object must be initialized to CS_SEND_HELLO_REQUEST or CS_COOKIE_EXCHANGE!");

        m_isInitialized = true;

        m_sslContext = sslContext;
        m_mtu = mtu;

        ///////////Shared states///////////
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_TOP), AZ::HSM::StateHandler(&AZ::DummyStateHandler<true>), AZ::HSM::InvalidStateId, CS_ACTIVE);
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ACTIVE), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateActive), CS_TOP, AZ::HSM::StateId(startState));
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ESTABLISHED), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateEstablished), CS_ACTIVE);
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_DISCONNECTED), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateDisconnected), CS_TOP);
        ///////////Server states///////////
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_SEND_HELLO_REQUEST), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateSendHelloRequest), CS_ACTIVE);
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_ACCEPT), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateAccept), CS_ACTIVE);
        ///////////Client states///////////
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_COOKIE_EXCHANGE), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateCookieExchange), CS_ACTIVE);
        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(CS_CONNECT), AZ::HSM::StateHandler(this, &SecureSocketDriver::Connection::OnStateConnect), CS_ACTIVE);

        m_sm.Start();

        return true;
    }

    void SecureSocketDriver::Connection::Shutdown()
    {
        DestroySSL();
        m_isInitialized = false;
    }

    int SecureSocketDriver::Connection::QueueDatagrams()
    {
        AZStd::vector<SecureSocketDriver::Datagram> dgramList;
        int dgrams = 0;

        if (ReadDgramFromBuffer(m_outDTLSBuffer, dgramList))
        {
            for (const auto& dgram : dgramList)
            {
                ++dgrams;
                m_outDTLSQueue.push(dgram);
                //DEBUG high
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | [%08x] RawSend %s size %d to %s\n",
                    AZStd::chrono::system_clock::now().time_since_epoch().count(), this,
                    ConnectionSecurity::TypeToString(dgram.data(), dgram.size()), dgram.size(),
                    Internal::SafeGetAddress(m_addr->ToString()));
            }
        }

        if (dgrams)
        {
            AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | [%08x] Server Handshake sent %d tot %d. State '%s'\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, dgrams, m_outDTLSQueue.size(), SSL_state_string_long(m_ssl));
        }

        return dgrams;
    }

    bool SecureSocketDriver::Connection::OnStateActive(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        switch (event.id)
        {
            case AZ::HSM::EnterEventId:
            {
                m_creationTime = AZStd::chrono::system_clock::now();
                CreateSSL(m_sslContext);
                return true;
            }
            case CE_UPDATE:
            {
                if (!m_addr->IsBoundToCarrierConnection()) //If the Carrier never bound or bound and unbounded
                {
                    const auto now = AZStd::chrono::system_clock::now();
                    // If connection is not bound any time after the handshake period, disconnect.
                    if (AZStd::chrono::milliseconds(now - m_creationTime).count() > static_cast<typename AZStd::chrono::milliseconds::rep>(m_timeoutMS))
                    {
                        AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | [%08x] :%d Connection unbound from %s. DgramsSent=%d, DgramsReceived=%d timeout %llu\n", now.time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived, m_timeoutMS);
                        sm.Transition(CS_DISCONNECTED);
                        return true;
                    }
                }
                return false;
            }
        }
        return false;
    }

    bool SecureSocketDriver::Connection::OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        switch (event.id)
        {
            case AZ::HSM::EnterEventId:
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | \tS [%08x] :%d Accepting a new connection from %s. DgramsSent=%d, DgramsReceived=%d\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);
                SSL_accept(m_ssl);
                return true;
            }
            case AZ::HSM::ExitEventId:
                return true;
            case CE_NEW_INCOMING_DGRAM:
            case CE_UPDATE:
            {
        bool changedState = false;

        int result = SSL_accept(m_ssl);
        if (result == 1)
        {
            sm.Transition(ConnectionState::CS_ESTABLISHED);
            changedState = true;
        }
        else if (result <= 0)
        {
                    changedState = HandleSSLError(result);
        }

                QueueDatagrams();
                return changedState;
            }
            default: break;
        }

        return false;
    }

    bool SecureSocketDriver::Connection::OnStateSendHelloRequest(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;

        const auto SendHello = [&](const AZStd::chrono::system_clock::time_point &now)
        {
            (void)now;
            char buffer[ConnectionSecurity::kMaxPacketSize];
            WriteBufferStaticInPlace writer(EndianType::BigEndian, buffer, sizeof(buffer));
            ConnectionSecurity::HelloRequest helloRequest;
            if (!helloRequest.Pack(writer))
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | S [%08x] :%d Failed to Pack HelloRequest!\n", now.time_since_epoch().count(), this, m_dbgPort);
                return;
            }

            m_outDTLSQueue.emplace(&buffer[0], &buffer[0] + writer.Size());
            m_dbgDgramsSent++;

        };

        const AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();

        switch (event.id)
        {
            case AZ::HSM::EnterEventId:
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | \tS [%08x] :%d Waiting for stateful handshake from %s. DgramsSent=%d, DgramsReceived=%d\n", now.time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);

                m_helloRequestResendInterval = k_initialHelloRequestResendInterval;
                m_nextHelloRequestResend = now + m_helloRequestResendInterval;
                SendHello(now);
                break;
            }
            case CE_STATEFUL_HANDSHAKE:
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | \tS [%08x] :%d Starting SSL portion of handshake with %s. DgramsSent=%d, DgramsReceived=%d\n", now.time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);
                sm.Transition(CS_ACCEPT);
                return true;
            case CE_UPDATE:
            {
                //Enter this state when re-handshaking or an initial move from cookie to handshake
        if (now > m_nextHelloRequestResend)
        {
            m_nextHelloRequestResend = now + m_helloRequestResendInterval;
            m_helloRequestResendInterval *= 2; // exponential backoff
                    m_helloRequestResendInterval = AZStd::GetMin(m_helloRequestResendInterval, AZStd::chrono::milliseconds(1000));
                    SendHello(now);
            }
                break;
            }
            default: break;
        }
        return false;
    }

    bool SecureSocketDriver::Connection::OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "%p Connecting to %s.DgramsSent=%d, DgramsReceived=%d\n", this, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);

                SSL_connect(m_ssl);
                QueueDatagrams();
                m_nextHandshakeRetry = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_timeoutMS / kSSLHandshakeAttempts);
                return true;
    }
        case AZ::HSM::ExitEventId:
            return true;
            case CE_NEW_INCOMING_DGRAM:
            {
        bool changedState = false;
            int result = SSL_connect(m_ssl);
            if (result == 1)
            {
                sm.Transition(ConnectionState::CS_ESTABLISHED);
                changedState = true;
            }
            else if (result <= 0)
            {
                    changedState = HandleSSLError(result);
                }

                QueueDatagrams();
                return changedState;
            }
            case CE_UPDATE:
            {
                if (m_nextHandshakeRetry <= AZStd::chrono::system_clock::now())
                {
                    AZ_DebugSecureSocketConnection("GridMateSecure", ":%d CS_HANDSHAKE_RETRY to %s.DgramsSent=%d, DgramsReceived=%d outQueue %d State %s\n", m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived, m_outDTLSQueue.size(), SSL_state_string_long(m_ssl));
                    m_nextHandshakeRetry = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_timeoutMS / kSSLHandshakeAttempts);
                    ForceDTLSTimeout();
                    QueueDatagrams();
                }
                break;
            }
            default: break;
        }

        return false;
    }

    bool SecureSocketDriver::Connection::OnStateCookieExchange(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        switch (event.id)
        {
            case AZ::HSM::EnterEventId:
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | [%08x] :%d Exchanging cookie with %s. DgramsSent=%d, DgramsReceived=%d\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);

                SSL_connect(m_ssl);
                QueueDatagrams();
                m_nextHandshakeRetry = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_timeoutMS / kSSLHandshakeAttempts);
                break;
            case CE_COOKIE_EXCHANGE_COMPLETED:
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | C [%08x] :%d Cookie exchange completed. Starting NEW SSL portion of handshake with %s. DgramsSent=%d, DgramsReceived=%d\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);
                //We have to restart SSL for the non-cookie handshake
                DestroySSL();
                CreateSSL(m_sslContext);
                sm.Transition(CS_CONNECT);
                return true;
        }
            case CE_NEW_INCOMING_DGRAM:
            {
                bool changedState = false;
                int result = SSL_connect(m_ssl);
                if (result <= 0)
                {
                    changedState = HandleSSLError(result);
                }
                QueueDatagrams();
                return changedState;
    }
            case CE_UPDATE:
    {
                if (m_nextHandshakeRetry <= AZStd::chrono::system_clock::now())
        {
                    AZ_DebugSecureSocketConnection("GridMateSecure", "CS_HANDSHAKE_RETRY to %s.DgramsSent=%d, DgramsReceived=%d\n", Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);

                    m_nextHandshakeRetry = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_timeoutMS / kSSLHandshakeAttempts);
                    ForceDTLSTimeout();
                    QueueDatagrams();
                }
                break;
            }
            default: break;
        }

        return false;
    }

    bool SecureSocketDriver::Connection::OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
        {
                /*static AZStd::atomic_int count = 0;
                ++count;
                AZ_TracePrintf("GridMateSecure", "OnStateEstablished %d\n", count);*/
                /*AZ_DebugSecureSocketConnection("GridMateSecure", "%lld | [%08x] :%d Successfully established connection to %s. DgramsSent=%d, DgramsReceived=%d\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);*/
            return true;
        }
        case AZ::HSM::ExitEventId:
            return true;
            case CE_NEW_INCOMING_DGRAM:
    {
                while (BIO_pending(m_inDTLSBuffer)) //TODO ratelimit
        {
                    Datagram newReceived;
                    newReceived.resize_no_construct(m_mtu);
                    AZ::s32 bytesRead = SSL_read(m_ssl, newReceived.data(), m_mtu);
            if (bytesRead <= 0)
            {
                        if(HandleSSLError(bytesRead))
                {
                    return true;
                }
                break;
            }
                    newReceived.resize_no_construct(bytesRead);
                    m_inboundPlaintextQueue->emplace(AZStd::move(newReceived), m_addr);
                }
                return false;
        }
            case CE_NEW_OUTGOING_DGRAM:
            {
                for (; !m_outboundPlainQueue.empty(); m_outboundPlainQueue.pop())   //TODO rate limit
        {
                    const Datagram& plainDgram = m_outboundPlainQueue.front();
            AZ::s32 bytesWritten = SSL_write(m_ssl, plainDgram.data(), static_cast<int>(plainDgram.size()));
            if (bytesWritten <= 0)
            {
                        if(HandleSSLError(bytesWritten))
                {
                    return true;
                }
                break;
            }

                    QueueDatagrams();
                }
                return false;
            }
            default: break;
        }

        return false;
    }

    bool SecureSocketDriver::Connection::OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& event)
    {
        (void)sm;
        (void)event;
        switch (event.id)
        {
        case AZ::HSM::EnterEventId:
            AZ_DebugSecureSocketConnection("GridMateSecure", "[%08x] :%d Secure connection to %s terminated. DgramsSent=%d, DgramsReceived=%d\n", this, m_dbgPort, Internal::SafeGetAddress(m_addr->ToString()), m_dbgDgramsSent, m_dbgDgramsReceived);
#ifdef AZ_DebugUseSocketDebugLog
            AZ_TracePrintf("GridMate", "Disconnected LOG: %s\n", m_dbgLog.c_str());
#endif
            return true;
        }
        return false;
    }

    bool SecureSocketDriver::Connection::HandleSSLError(AZ::s32 result)
    {
        AZ::s32 sslError = SSL_get_error(m_ssl, result);
        if (sslError != SSL_ERROR_WANT_READ && sslError != SSL_ERROR_WANT_WRITE)
        {
            m_sslError = sslError;
            if (m_sslError == SSL_ERROR_SSL)
        {
            static const AZ::u32 BUFFER_SIZE = 256;
            char buffer[BUFFER_SIZE];
                ERR_error_string_n(ERR_get_error(), buffer, BUFFER_SIZE);
                AZ_DebugSecureSocketConnection("GridMateSecure", "[%08x] Connection error occurred on %s with SSL error %s.\n", this, Internal::SafeGetAddress(m_addr->ToString()), buffer);
            }
            else
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "[%08x] Connection error occurred on %s with SSL error %d.\n", this, Internal::SafeGetAddress(m_addr->ToString()), m_sslError);
            }
            m_sm.Transition(CS_DISCONNECTED);
            return true;
        }

        return false;
        }

    void SecureSocketDriver::Connection::ForceDTLSTimeout()
    {
        timeval next_timeout;
        next_timeout.tv_sec = 0;
        next_timeout.tv_usec = 1;    //Add just enough to not detect as NULL
        BIO_ctrl(m_inDTLSBuffer, BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT, 0, &next_timeout);

        // we are unable to set SSLs DTLS next_timeout because the ssl_st structure
        // is now opaque, so this may fail if there is no active DTLS timer
        DTLSv1_handle_timeout(m_ssl);
    }

    bool SecureSocketDriver::Connection::CreateSSL(SSL_CTX* sslContext)
    {
        AZ_Assert(m_isInitialized, "Initialize SecureSocketDriver::Connection first!");
        AZ_Assert(m_ssl == nullptr, "This connection already has an SSL context! Make sure the previous one is destroyed first!");
        m_ssl = SSL_new(sslContext);
        if (m_ssl == nullptr)
        {
            AZ_Warning("GridMateSecure", false, "Failed to create ssl object for %s!", Internal::SafeGetAddress( m_addr->ToString()));
            return false;
        }

        // Set the internal DTLS MTU for use when fragmenting DTLS handshake datagrams only,
        // and not the application datagrams (i.e. internally generated datagrams). Datagrams
        // passed into the SecureSocketDriver are expected to already be smaller them GetMaxSendSize().
        // This is particularly relevant when sending certificates in the handshake which will
        // likely be larger than the MTU.
        SSL_set_mtu(m_ssl, m_mtu);

        m_inDTLSBuffer = BIO_new(BIO_s_mem());
        if (m_inDTLSBuffer == nullptr)
        {
            AZ_Warning("GridMateSecure", false, "Failed to instantiate m_inDTLSBuffer for %s!", Internal::SafeGetAddress(m_addr->ToString()));
            SSL_free(m_ssl);
            m_ssl = nullptr;

            return false;
        }

        m_outDTLSBuffer = BIO_new(BIO_s_mem());
        if (m_outDTLSBuffer == nullptr)
        {
            AZ_Warning("GridMateSecure", false, "Failed to instantiate m_outDTLSBuffer for %s!", Internal::SafeGetAddress(m_addr->ToString()));

            SSL_free(m_ssl);
            m_ssl = nullptr;

            BIO_free(m_inDTLSBuffer);
            m_inDTLSBuffer = nullptr;

            return false;
        }

        BIO_set_mem_eof_return(m_inDTLSBuffer, -1);
        BIO_set_mem_eof_return(m_outDTLSBuffer, -1);

        SSL_set_bio(m_ssl, m_inDTLSBuffer, m_outDTLSBuffer);

        return true;
    }

    bool SecureSocketDriver::Connection::DestroySSL()
    {
        if (m_ssl)
        {
            // Calls to SSL_free() also free any attached BIO objects.
            SSL_free(m_ssl);
            m_ssl = nullptr;
            m_inDTLSBuffer = nullptr;
            m_outDTLSBuffer = nullptr;
        }

        return true;
    }

    bool SecureSocketDriver::Connection::ReadDgramFromBuffer(BIO* bio, AZStd::vector<SecureSocketDriver::Datagram>& outDgramList)
    {
        //NOTE: It is expected that this BIO buffer has been filled up with multiple DTLS records
        //      created by SSL functions: SSL_write(), SSL_accept(), or SSL_connect().

        // Drain the BIO buffer and store the contents in a temporary buffer for
        // deserialization. This is necessary because the BIO object doesn't provide
        // a way to directly access it's memory.
        bool isSuccess = true;
        int availableEncryptedBytes = BIO_pending(bio);
        if (availableEncryptedBytes > 0)
        {
            AZStd::vector<char> tempBuffer;
            tempBuffer.resize_no_construct(availableEncryptedBytes);
            int bytesRead = BIO_read(bio, tempBuffer.data(), availableEncryptedBytes);
            if (bytesRead != availableEncryptedBytes)
            {
                AZ_Assert(false, "We did not extract the expected number of bytes from OpenSSL (expected=%d, read=%d)", availableEncryptedBytes, bytesRead);
                isSuccess = false;
            }
            else
            {
        // Multiple DTLS records (datagrams) may have been written to the BIO buffer so
        // each one must be extracted and stored as a separate Datagram object in the driver.
        static const size_t lengthOffsetInDTLSRecord = DTLS1_RT_HEADER_LENGTH - sizeof(AZ::u16);
        size_t recordStart = 0, recordEnd = (lengthOffsetInDTLSRecord + sizeof(AZ::u16));
                        while (recordEnd < tempBuffer.size())
        {
                    AZ::u16 recordLength = *reinterpret_cast<AZ::u16*>(tempBuffer.data() + recordEnd - sizeof(AZ::u16));

            // The fields in a DTLS record are stored in big-endian format so perform
            // an endian swap on little-endian machines.
            AZStd::endian_swap<AZ::u16>(recordLength);
            recordEnd += recordLength;
                            if (recordEnd > tempBuffer.size())
            {
                break;
            }

                    outDgramList.emplace_back(tempBuffer.data() + recordStart, tempBuffer.data() + recordEnd);

            recordStart = recordEnd;
            recordEnd += (lengthOffsetInDTLSRecord + sizeof(AZ::u16));
        }

        // If a deserialization error occurred (i.e. not all bytes in the buffer were read) all datagrams
        // after the malformation are discarded. It's assumed that the BIO buffer only contained complete
        // DTLS record datagrams.
                bool isComplete = (recordStart == tempBuffer.size());
                (void)isComplete;
                AZ_Assert(isComplete, "Malformed DTLS record found, dropping remaining records in the buffer (%d bytes lost).\n", (tempBuffer.size() - recordStart));
            }
        }
        return isSuccess;
    }

    void SecureSocketDriver::Connection::Update()
    {
        if (!m_sm.IsDispatching())
        {
            m_sm.Dispatch(ConnectionEvents::CE_UPDATE);
        }
    }

    void SecureSocketDriver::Connection::AddDgram(const char* data, AZ::u32 dataSize)
    {
        m_outboundPlainQueue.push(Datagram(data, data + dataSize));
    }

    void SecureSocketDriver::Connection::ProcessIncomingDTLSDgram(const char* data, AZ::u32 dataSize)
    {
        using namespace ConnectionSecurity;
        using namespace AZStd::chrono;
        bool keepDgram = false;

        switch (m_sm.GetCurrentState())
        {
            case CS_SEND_HELLO_REQUEST:    //Server
            {
                if (IsClientHello(data, dataSize))
                {
                    // We are only interested in new ClientHellos at this point
                    ReadBuffer reader(EndianType::BigEndian, data, dataSize);
                    ClientHello clientHello;
                    clientHello.Unpack(reader);
                    if (clientHello.m_hsSequence == 0)  //Received the start of a new handshake
                    {
                        m_sm.Dispatch(CE_STATEFUL_HANDSHAKE);
                        keepDgram = true;
                    }
                }
                break;
            }
            case CS_COOKIE_EXCHANGE:
            {
                if (IsHelloRequestHandshake(data, dataSize))
                {
                    m_sm.Dispatch(CE_COOKIE_EXCHANGE_COMPLETED);    //transition and discard datagram
                }
                else
                {
                    keepDgram = true;
                }
                break;
            }
            case CS_CONNECT:
            {
                keepDgram = !IsHelloRequestHandshake(data, dataSize);
                break;
                }
            default:
                keepDgram = true;
                break;
            }

        if (keepDgram)
            {
            const auto now = system_clock::now();

            //Received handshake while established (but still before timeout)
            if (m_sm.GetCurrentState() == CS_ESTABLISHED
                && IsHandshake(data, dataSize)
                && now >= m_creationTime
                && milliseconds(now - m_creationTime).count() <= static_cast<typename AZStd::chrono::milliseconds::rep>(m_timeoutMS))
            {
                //Resend Finished to close handshake
                Datagram newReceived;
                newReceived.resize_no_construct(m_mtu);

                size_t bytesRead = SSL_get_finished(m_ssl, newReceived.data(), m_mtu);
                if (bytesRead > 0)
                {
                    newReceived.resize_no_construct(bytesRead);
                    m_outDTLSQueue.push(newReceived);
                }

                QueueDatagrams();
            }

                BIO_write(m_inDTLSBuffer, data, dataSize);
            m_sm.Dispatch(CE_NEW_INCOMING_DGRAM);
        }

    }

    AZ::u32 SecureSocketDriver::Connection::GetDTLSDgram(char* data, AZ::u32 dataSize)
    {
        AZ::u32 dgramSize = 0;
        if (!m_outDTLSQueue.empty())
        {
            const Datagram& dgram = m_outDTLSQueue.front();
            if (dgram.size() <= dataSize)
            {
                memcpy(data, dgram.data(), dgram.size());
                dgramSize = static_cast<AZ::u32>(dgram.size());
            }
            else
            {
                AZ_DebugSecureSocketConnection("GridMateSecure", "[%08x] Dropped datagram of %d bytes.\n", this, dgram.size());
            }

            m_outDTLSQueue.pop();
        }

        return dgramSize;
    }

    void SecureSocketDriver::Connection::FlushOutgoingDTLSDgrams()
    {
        if (!m_outboundPlainQueue.empty())
        {
            m_sm.Dispatch(CE_NEW_OUTGOING_DGRAM);
        }
    }

    bool SecureSocketDriver::Connection::IsDisconnected() const
    {
        return m_sm.GetCurrentState() == CS_DISCONNECTED;
    }

    SecureSocketDriver::SecureSocketDriver(const SecureSocketDesc& desc, bool isFullPackets, bool crossPlatform, bool highPerformance)
        : SocketDriver(isFullPackets, crossPlatform, highPerformance)
        , m_privateKey(nullptr)
        , m_certificate(nullptr)
        , m_sslContext(nullptr)
        , m_tempSocketWriteBuffer(nullptr)
        , m_tempSocketReadBuffer(nullptr)
        , m_maxTempBufferSize(10 * 1024)
        , m_desc(desc)
        , m_lastTimerCheck(AZStd::chrono::system_clock::now())
    {
        m_tempSocketWriteBuffer = static_cast<char*>(azmalloc(m_maxTempBufferSize));
        m_tempSocketReadBuffer = static_cast<char*>(azmalloc(m_maxTempBufferSize));
        AZ_Warning("GridMateSecure", m_desc.m_connectionTimeoutMS / kSSLHandshakeAttempts <= 1000, "Capping SecureSocketDriver connection timeout at 1 second.");
    }

    SecureSocketDriver::~SecureSocketDriver()
    {
        for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
        {
            delete addrConn.second;
        }

        if (m_tempSocketWriteBuffer)
        {
            azfree(m_tempSocketWriteBuffer);
            m_tempSocketWriteBuffer = nullptr;
        }

        if (m_tempSocketReadBuffer)
        {
            azfree(m_tempSocketReadBuffer);
            m_tempSocketReadBuffer = nullptr;
        }

        if (m_certificate)
        {
            X509_free(m_certificate);
            m_certificate = nullptr;
        }

        if (m_privateKey)
        {
            EVP_PKEY_free(m_privateKey);
            m_privateKey = nullptr;
        }

        if (m_sslContext)
        {
            // Calls to SSL_CTX_free() also free any attached X509_STORE objects.
            SSL_CTX_free(m_sslContext);
            m_sslContext = nullptr;
        }
    }

    AZ::u32 SecureSocketDriver::GetMaxSendSize() const
    {
        // This is the size of the DTLS header when sending application data. The DTLS
        // header during handshake is larger but isn't relevant here since the user can
        // only send data as application data and never during the handshake.
        static const AZ::u32 sDTLSHeader = DTLS1_RT_HEADER_LENGTH;

        // An additional overhead, as a result of encrypting the data (padding, etc), needs
        // to be calculated and added.
        //NOTE: This value was determined by looking at different sized DTLS datagrams, but
        //      needs some more thought put into it based on the cipher type and mode of operation
        //      (i.e. AES and GCM).
        static const AZ::u32 sCipherOverhead = 30;

        return SocketDriverCommon::GetMaxSendSize() - sDTLSHeader - sCipherOverhead;
    }
    void SecureSocketDriver::apps_ssl_info_callback(const SSL *s, int loc, int ret)
    {
        const char *str;
        int w = loc& ~SSL_ST_MASK;

        if (w & SSL_ST_CONNECT) str = "SSL_connect()";
        else if (w & SSL_ST_ACCEPT) str = "SSL_accept()";
        else str = "undefined";

        if (loc & SSL_CB_LOOP)
        {
            //AZ_TracePrintf("GridMate", "%s:%s\n", str, SSL_state_string_long(s));
        }
        else if (loc & SSL_CB_ALERT)
        {
            /*str = (loc & SSL_CB_READ) ? "read" : "write";
            AZ_TracePrintf("GridMate", "SSL3 alert %s:%s:%s\n",
                str,
                SSL_alert_type_string_long(ret),
                SSL_alert_desc_string_long(ret))*/
        }
        else if (loc & SSL_CB_EXIT)
        {
            if (SSL_get_error(s, ret) == SSL_ERROR_WANT_READ || SSL_get_error(s, ret) == SSL_ERROR_WANT_WRITE)
            {
                //Don't spam non-blocking read/write updates
                return;
            }
            char errorTextBuffer[256] = {0};
            ERR_error_string_n( SSL_get_error(s, ret), errorTextBuffer, sizeof(errorTextBuffer) );

            if (ret == 0)
            {
                AZ_Printf("GridMateSecure", "[%08x] %s: failed in %s : %s\n",s, str, SSL_state_string_long(s), errorTextBuffer);
            }
            else if (ret < 0)
            {
                AZ_Printf("GridMateSecure", "[%08x] %s: error in %s : %s\n", s, str, SSL_state_string_long(s), errorTextBuffer);
            }
        }
    }

    void apps_ssl_msg_callback(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void * /*arg*/)
    {
        const char* ctype;
        switch (content_type)
        {
        case SSL3_RT_CHANGE_CIPHER_SPEC:
            ctype = "cipher_spec";
            break;
        case SSL3_RT_ALERT:
            ctype = "alert";
            break;
        case SSL3_RT_HANDSHAKE:
            ctype = "handshake";
            break;
        case SSL3_RT_HEADER:
            ctype = "RecordHeaderOnly";       //Tx/Rx header only see SSL_CTX_set_msg_callback
            break;
        case SSL3_RT_APPLICATION_DATA:
            ctype = "AppData";
            break;
        default:
            ctype = "unkn_type";
        }

        AZ_Printf("GridMateSecure", "[%08x] : %s %04xv %s(%d) buf %p len %d\n", ssl, write_p ? "Rx" : "Tx", version, ctype, content_type, buf, len);
        AZ_Printf("GridMate", "%s\n", AZStd::MemoryToASCII::ToString(buf, len, 256).c_str() );
    }


    Driver::ResultCode SecureSocketDriver::Initialize(AZ::s32 ft, const char* address, AZ::u32 port, bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize)
    {
        if (m_desc.m_privateKeyPEM != nullptr && m_desc.m_certificatePEM == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "If a private key is provided, so must a corresponding certificate.\n");
            return EC_SECURE_CONFIG;
        }

        if (m_desc.m_certificatePEM != nullptr && m_desc.m_privateKeyPEM == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "If a certificate is provided, so must a corresponding private key.\n");
            return EC_SECURE_CONFIG;
        }

        ResultCode result = SocketDriver::Initialize(ft, address, port, isBroadcast, receiveBufferSize, sendBufferSize);
        if (result != EC_OK)
        {
            return result;
        }

        SSL_library_init();

        ERR_load_crypto_strings();
        ERR_load_BIO_strings();
        ERR_load_SSL_strings();
        SSL_load_error_strings();

        m_sslContext = SSL_CTX_new(DTLSv1_2_method());
        if (m_sslContext == nullptr)
        {
            return EC_SECURE_CREATE;
        }

        // Disable automatic MTU discovery so it can be set explicitly in SecureSocketDriver::Connection.
        SSL_CTX_set_options(m_sslContext, SSL_OP_NO_QUERY_MTU);

        //Detailed SSL Debugging
        {
            //SSL_CTX_set_info_callback(m_sslContext, &apps_ssl_info_callback);
            //SSL_CTX_set_msg_callback(m_sslContext, &apps_ssl_msg_callback);     //Per-protocol message logging
        }

        // Only support a single cipher suite in OpenSSL that supports:
        //
        //  ECDHE       Key exchange using ephemeral elliptic curve diffie-hellman.
        //  RSA         Authentication (public and private key) used to sign ECDHE parameters and can be checked against a CA.
        //  AES256      AES cipher for symmetric key encryption using a 256-bit key.
        //  GCM         Mode of operation for symmetric key encryption.
        //  SHA384      SHA-2 hashing algorithm.
        if (SSL_CTX_set_cipher_list(m_sslContext, "ECDHE-RSA-AES256-GCM-SHA384") != 1)
        {
            return EC_SECURE_CREATE;
        }

        // Automatically generate parameters for elliptic-curve diffie-hellman (i.e. curve type and coefficients).
        SSL_CTX_set_ecdh_auto(m_sslContext, 1);

        if (m_desc.m_certificatePEM)
        {
            m_certificate = CreateCertificateFromEncodedPEM(m_desc.m_certificatePEM);
            if (m_certificate == nullptr || SSL_CTX_use_certificate(m_sslContext, m_certificate) != 1)
            {
                return EC_SECURE_CERT;
            }
        }

        if (m_desc.m_privateKeyPEM)
        {
            m_privateKey = CreatePrivateKeyFromEncodedPEM(m_desc.m_privateKeyPEM);
            if (m_privateKey == nullptr || SSL_CTX_use_PrivateKey(m_sslContext, m_privateKey) != 1)
            {
                return EC_SECURE_PKEY;
            }
        }

        // Determine if both client and server must be authenticated or only the server.
        // The default behavior only authenticates the server, and not the client.
        int verificationMode = SSL_VERIFY_PEER;
        if (m_desc.m_authenticateClient)
        {
            verificationMode = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        }

        if (m_desc.m_certificateAuthorityPEM)
        {
            // SSL context should have empty cert storage
            X509_STORE* caLocalStore = SSL_CTX_get_cert_store(m_sslContext);
            if (caLocalStore == nullptr)
            {
                return EC_SECURE_CA_CERT;
            }

            AZStd::vector<X509*> certificateChain;
            CreateCertificateChainFromEncodedPEM(m_desc.m_certificateAuthorityPEM, certificateChain);
            if (certificateChain.size() == 0)
            {
                return EC_SECURE_CA_CERT;
            }

            for (auto certificate : certificateChain)
            {
                X509_STORE_add_cert(caLocalStore, certificate);
            }
            SSL_CTX_set_verify(m_sslContext, verificationMode, nullptr);
        }
        else
        {
            AZ_TracePrintf("GridMateSecure", "No certificateAuthorityPEM set, using NULL verifier.\n");
            SSL_CTX_set_verify(m_sslContext, verificationMode, SecureSocketDriver::VerifyCertificate);
        }

        if (!SSL_CTX_set_ex_data(m_sslContext, kSSLContextDriverPtrArg, this))
        {
            AZ_TracePrintf("GridMateSecure", "Failed to set driver for ssl context\n");
            return EC_SECURE_CREATE;
        }

        //Generate the initial key
        RotateCookieSecret(true);

        return EC_OK;
    }

    void SecureSocketDriver::Update()
    {
        using namespace AZStd::chrono;
        static const auto k_timerResolutionMS = milliseconds(15);  //Use 15ms resolution from OpenSSL
        const auto now = system_clock::now();

        if (now > m_lastTimerCheck && milliseconds(now - m_lastTimerCheck) >= k_timerResolutionMS)
        {
            m_lastTimerCheck = now;
        UpdateConnections();
    }
    }

    void SecureSocketDriver::ProcessIncoming()
    {
        FlushSocketToConnectionBuffer();
    }

    void SecureSocketDriver::ProcessOutgoing()
    {
        FlushConnectionBuffersToSocket();
    }

    AZ::u32 SecureSocketDriver::Receive(char* data, AZ::u32 maxDataSize, AddrPtr& from, ResultCode* resultCode)
    {
        if (m_globalInQueue.empty())
        {
            if (resultCode)
            {
                *resultCode = EC_OK;
            }
            return 0;
        }

        const Datagram& datagram = m_globalInQueue.front().first;
        const AddrPtr& fromAddr = m_globalInQueue.front().second;
        if (datagram.size() <= maxDataSize)
        {
            memcpy(data, datagram.data(), datagram.size());
            if (resultCode)
            {
                *resultCode = EC_OK;
            }
            from = fromAddr;
            m_globalInQueue.pop();
            return static_cast<AZ::u32>(datagram.size());
        }

        AZ_DebugSecureSocket("GridMateSecure", "Dropped datagram of %d bytes from %s.\n", datagram.size(), Internal::SafeGetAddress(fromAddr->ToString()));
        if (resultCode)
        {
            *resultCode = EC_RECEIVE;
        }
        m_globalInQueue.pop();
        return 0;
    }

    bool SecureSocketDriver::RotateCookieSecret(bool force /*= false*/)
    {
        TimeStamp currentTime = AZStd::chrono::system_clock::now();

        // Get the last time since we updated our secret.
        AZ::u64 durationSinceLastMS = AZStd::chrono::milliseconds(currentTime - m_cookieSecret.m_lastSecretGenerationTime).count();

        if (force || durationSinceLastMS > kDTLSSecretExpirationTime)
        {
            m_cookieSecret.m_lastSecretGenerationTime = currentTime;
            //Should we copy the old key to keep it just in case there's a handshake with the old secret?
            if (durationSinceLastMS < 2 * kDTLSSecretExpirationTime)
            {
                memcpy(m_cookieSecret.m_previousSecret, m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_previousSecret));
                m_cookieSecret.m_isPreviousSecretValid = true;
            }
            else
            {
                memset(m_cookieSecret.m_previousSecret, 0, sizeof(m_cookieSecret.m_previousSecret));
                m_cookieSecret.m_isPreviousSecretValid = false;
            }
            int cookieGeneration = RAND_bytes(m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_currentSecret));
            m_cookieSecret.m_isCurrentSecretValid = true;
            AZ_Verify(cookieGeneration == 1, "Failed to generate the cookie");
            return cookieGeneration == 1;
        }
        return true;
    }

    Driver::ResultCode SecureSocketDriver::Send(const AddrPtr& to, const char* data, AZ::u32 dataSize)
    {
        SocketDriverAddress* connKey = static_cast<SocketDriverAddress*>(to.get());
        Connection* connection = nullptr;
        auto connItr = m_connections.find(*connKey);
        if (connItr == m_connections.end())
        {
            connection = aznew Connection(to, m_maxTempBufferSize, &m_globalInQueue, m_desc.m_connectionTimeoutMS, m_port);
            if (connection->Initialize(m_sslContext, ConnectionState::CS_COOKIE_EXCHANGE, SecureSocketDriver::GetMaxSendSize()))
            {
                ++m_ipToNumConnections[connKey->GetIP()];
                m_connections[*connKey] = connection;
            }
            else
            {
                AZ_Warning("GridMate", false, "Failed to initialize secure outbound connection object for %s.\n", Internal::SafeGetAddress(to->ToString()));
                delete connection;
                connection = nullptr;
                return EC_SEND;
            }
        }
        else
        {
            connection = connItr->second;
        }

        connection->AddDgram(data, dataSize);

        return EC_OK;
    }

    int SecureSocketDriver::VerifyCertificate(int ok, X509_STORE_CTX* ctx)
    {
        //This is the NULL verifier. It should only be used when no PEM is set and will accept all certificates.

        // Called when a certificate has been received and needs to be verified (e.g.
        // verify that it has been signed by the appropriate CA, has the correct
        // hostname, etc).

        (void)ok;
        (void)ctx;
        return 1;
    }


    int SecureSocketDriver::GenerateCookie(AddrPtr endpoint, unsigned char* cookie, unsigned int* cookieLen)
    {
        const unsigned int cookieMaxLen = *cookieLen;
        (void)cookieMaxLen;

        *cookieLen = 0;

        if (cookie == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "Cookie is nullptr, it needs to be allocated");
            return 0;
        }

        if (!RotateCookieSecret())
        {
            AZ_TracePrintf("GridMateSecure", "Failed to rotate secret\n");
            return 0;
        }

        // Calculate HMAC of buffer using the secret and peer address
        AZStd::string addrStr = endpoint->ToAddress();
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha1(), m_cookieSecret.m_currentSecret, sizeof(m_cookieSecret.m_currentSecret),
            reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.size(),
            result, &resultLen);

        if (resultLen > MAX_COOKIE_LENGTH)
        {
            AZ_TracePrintf("GridMateSecure", "Insufficient cookie buffer: %u > %u\n", resultLen, cookieMaxLen);
            return 0;
        }

        memcpy(cookie, result, resultLen);
        *cookieLen = resultLen;
        return 1;
    }

    int SecureSocketDriver::VerifyCookie(AddrPtr endpoint, unsigned char* cookie, unsigned int cookieLen)
    {
        if (cookie == nullptr)
        {
            AZ_TracePrintf("GridMateSecure", "Cookie is nullptr");
            return 0;
        }

        if (!m_cookieSecret.m_isCurrentSecretValid)
        {
            AZ_TracePrintf("GridMateSecure", "Secret not initialized, can't verify cookie\n");
            return 0;
        }

        // Calculate HMAC of buffer using the secret and peer address
        AZStd::string addrStr = endpoint->ToAddress();
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha1(), m_cookieSecret.m_currentSecret, COOKIE_SECRET_LENGTH,
            reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.length(),
            result, &resultLen);

        if (cookieLen == resultLen && memcmp(result, cookie, resultLen) == 0)
        {
            return 1;
        }

        //This part was added to check for older handshakes, it only allows checks for 1 secret that's maximum 2* max life of key
        if (m_cookieSecret.m_isPreviousSecretValid)
        {
            /* Calculate HMAC of buffer using the secret */
            HMAC(EVP_sha1(), m_cookieSecret.m_previousSecret, COOKIE_SECRET_LENGTH,
                reinterpret_cast<const unsigned char*>(addrStr.c_str()), addrStr.length(),
                result, &resultLen);

            if (cookieLen == resultLen && memcmp(result, cookie, resultLen) == 0)
            {
                return 1;
            }
        }

        AZ_TracePrintf("GridMate", "Failed to validate the cookie for %s\n", Internal::SafeGetAddress(addrStr) );
        return 0;
    }

    void SecureSocketDriver::FlushSocketToConnectionBuffer()
    {
        while (true)
        {
            AddrPtr from;
            ResultCode result;
            AZ::u32 bytesReceived = SocketDriver::Receive(m_tempSocketReadBuffer, m_maxTempBufferSize, from, &result);
            if (result != EC_OK || bytesReceived == 0)
            {
                break;
            }

            const char* type = ConnectionSecurity::TypeToString(m_tempSocketReadBuffer, bytesReceived);
            (void)type;
#ifdef AZ_DebugUseSocketDebugLog
            bool handshake = false;
            if (ConnectionSecurity::IsHandshake(m_tempSocketReadBuffer, bytesReceived)
                || ConnectionSecurity::IsChangeCipherSpec(m_tempSocketReadBuffer, bytesReceived))
            {
                handshake = true;
            }
#endif

            Connection* connection = nullptr;
            SocketDriverAddress* connKey = static_cast<SocketDriverAddress*>(from.get());
            const auto itConnection = m_connections.find(*connKey);
            if (itConnection != m_connections.end())
            {
                connection = itConnection->second;
                connection->m_dbgDgramsReceived++;
                AZ_DebugSecureSocket("GridMateSecure", "%lld | [%08x] RawRecv %s size %d connection exists\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, type, bytesReceived);
#ifdef AZ_DebugUseSocketDebugLog
                if (handshake)
                {
                    AZStd::string line = AZStd::string::format("%lld | [%08x] RawRecv %s size %d connection exists\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, type, bytesReceived);
                    connection->m_dbgLog += line;
            }
#endif
            }
            else    //Stateless server or client received spurious datagram
            {
                auto numConnIt = m_ipToNumConnections.insert_key(from->GetIP());
                if (static_cast<AZ::s64>(numConnIt.first->second) >= m_desc.m_maxDTLSConnectionsPerIP) // cut off number of connections accepted per ip
                {
                    AZ_DebugSecureSocket("GridMateSecure", "Maximum connections per IP exceeded!");
                    continue;
                }

                const auto nextAction = ConnectionSecurity::DetermineHandshakeState(m_tempSocketReadBuffer, bytesReceived);
                if (nextAction == ConnectionSecurity::NextAction::SendHelloVerifyRequest)
                {
                    AZ_DebugSecureSocket("GridMateSecure", "%lld | \tS [%08x] RawRecv %s size %d SendHelloVerify\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, type, bytesReceived);
                    ConnectionSecurity::HelloVerifyRequest helloVerifyRequest;
                    unsigned int cookieLen = 0;
                    if (CanSend())
                    {
                    if (GenerateCookie(from, helloVerifyRequest.m_cookie, &cookieLen) == 1)
                    {
                        helloVerifyRequest.m_cookieSize = static_cast<AZ::u8>(cookieLen);

                        WriteBufferStatic<ConnectionSecurity::kMaxPacketSize> writeBuffer(EndianType::BigEndian);
                        if (helloVerifyRequest.Pack(writeBuffer))
                        {
                            SocketDriver::Send(from, writeBuffer.Get(), static_cast<unsigned int>(writeBuffer.Size()));
                        }
                        else
                        {
                                AZ_DebugSecureSocket("GridMateSecure", "Failed to pack HelloVerifyRequest!\n");
                            }
                        }
                        else
                        {
                            AZ_DebugSecureSocket("GridMateSecure", "Failed to generate HelloVerifyRequest!\n");
                        }
                    }
                    else
                    {
                        AZ_DebugSecureSocket("GridMateSecure", "No buffer space to send HelloVerifyRequest!\n");
                }
                }
                else if (nextAction == ConnectionSecurity::NextAction::VerifyCookie)
                {
                    AZ_DebugSecureSocket("GridMateSecure", "%lld | \tS [%08x] RawRecv %s size %d VerifyCookie\n", AZStd::chrono::system_clock::now().time_since_epoch().count(), this, type, bytesReceived);
                    ReadBuffer readBuffer(EndianType::BigEndian, m_tempSocketReadBuffer, bytesReceived);
                    ConnectionSecurity::ClientHello clientHello;
                    if (clientHello.Unpack(readBuffer))
                    {
                        if (VerifyCookie(from, clientHello.m_cookie, clientHello.m_cookieSize) != 1)
                        {
                            AZ_DebugSecureSocket("GridMateSecure", "ClientHello cookie failed verification!");
                        }
                        else
                        {
                            Connection* newConnection = aznew Connection(from, m_maxTempBufferSize, &m_globalInQueue, m_desc.m_connectionTimeoutMS, m_port);
                            if (newConnection->Initialize(m_sslContext, ConnectionState::CS_SEND_HELLO_REQUEST, SecureSocketDriver::GetMaxSendSize()))
                            {
                                ++(numConnIt.first->second);
                                m_connections[*connKey] = newConnection;
                                connection = newConnection;

                            }
                            else
                            {
                                AZ_Warning("GridMate", false, "Failed to initialize secure connection object for %s.\n", Internal::SafeGetAddress( from->ToString() ));
                                delete newConnection;
                            }
                        }
                    }
                    else
                    {
                        AZ_DebugSecureSocket("GridMate", "Failed to unpack clientHello(cookie) for %s.\n", Internal::SafeGetAddress(from->ToString()) );
                }
                }
            }

            if (connection)
            {
                connection->ProcessIncomingDTLSDgram(m_tempSocketReadBuffer, bytesReceived);
            }
        }
    }

    void SecureSocketDriver::UpdateConnections()
    {
        for (auto itr = m_connections.begin(); itr != m_connections.end(); )
        {
            AZStd::pair<SocketDriverAddress, Connection*>& addrConn = *itr;
            addrConn.second->Update();
            if (addrConn.second->IsDisconnected())
            {
                --m_ipToNumConnections[addrConn.first.GetIP()];
                delete addrConn.second;
                itr = m_connections.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }

    void SecureSocketDriver::FlushConnectionBuffersToSocket()
    {
        for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
        {
            Connection* connection = addrConn.second;
            connection->FlushOutgoingDTLSDgrams();
            while (CanSend())
            {
                AZ::s32 bytesRead = connection->GetDTLSDgram(m_tempSocketWriteBuffer, m_maxTempBufferSize);
                if (bytesRead <= 0)
                {
                    break;
                }

                DriverAddress* addr = static_cast<DriverAddress*>(&addrConn.first);
                SocketDriver::Send(addr, m_tempSocketWriteBuffer, bytesRead);
                connection->m_dbgDgramsSent++;

                AZ_DebugSecureSocket("GridMateSecure", "%lld | [%08x] RawSend %s size %d to %s\n",
                    AZStd::chrono::system_clock::now().time_since_epoch().count(), this,
                    ConnectionSecurity::TypeToString(m_tempSocketWriteBuffer, bytesRead),
                    bytesRead, Internal::SafeGetAddress(addr->ToString()));
            }
        }
    }
}
#endif
