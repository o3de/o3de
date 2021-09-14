/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/StreamSecureSocketDriver.h>

#if AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL

#include <AzCore/Math/MathUtils.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

//////////////////////////////////////////////////////////////////////////
// StreamSecureSocketDriver
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    X509* CreateCertificateFromEncodedPEM(const char* encodedPem);
    EVP_PKEY* CreatePrivateKeyFromEncodedPEM(const char* encodedPem);
    void CreateCertificateChainFromEncodedPEM(const char* encodedPem, AZStd::vector<X509*>& certificateChain);

    static AZStd::atomic_int s_initializeOpenSSLCount;

    bool InitializeOpenSSL()
    {
        if (s_initializeOpenSSLCount.fetch_add(1) == 0)
        {
            SSL_library_init();
            SSL_load_error_strings();
            ERR_load_BIO_strings();
            OpenSSL_add_all_algorithms();
        }
        return true;
    }

    bool ShutdownOpenSSL()
    {
        if (s_initializeOpenSSLCount.fetch_sub(1) == 1)
        {
            ERR_remove_state(0);
            ERR_free_strings();
            EVP_cleanup();
            sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
            CRYPTO_cleanup_all_ex_data();
        }
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
// SecureContextHandle
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    struct SecureContextHandleImpl
        : public StreamSecureSocketDriver::SecureContextHandle
    {
        GM_CLASS_ALLOCATOR(SecureContextHandleImpl);

        SecureContextHandleImpl()
            : m_ctx(nullptr)
            , m_privateKey(nullptr)
            , m_certificate(nullptr)
        {
        }

        ~SecureContextHandleImpl() override
        {
            Teardown();
        }

        Driver::ResultCode Prepare(StreamSecureSocketDriver::StreamSecureSocketDriverDesc& desc)
        {
            if (!InitializeOpenSSL())
            {
                return Driver::EC_SECURE_CREATE;
            }
            m_ctx = SSL_CTX_new(TLSv1_2_method());
            if (m_ctx == nullptr)
            {
                return Driver::EC_SECURE_CREATE;
            }

            // Only support a single cipher suite in OpenSSL that supports:
            //
            //  ECDHE       Key exchange using ephemeral elliptic curve diffie-hellman.
            //  RSA         Authentication (public and private key) used to sign ECDHE parameters and can be checked against a CA.
            //  AES256      AES cipher for symmetric key encryption using a 256-bit key.
            //  GCM         Mode of operation for symmetric key encryption.
            //  SHA384      SHA-2 hashing algorithm.
            if (SSL_CTX_set_cipher_list(m_ctx, "ECDHE-RSA-AES256-GCM-SHA384") != 1)
            {
                return Driver::EC_SECURE_CREATE;
            }

            // Automatically generate parameters for elliptic-curve diffie-hellman (i.e. curve type and coefficients).
            SSL_CTX_set_ecdh_auto(m_ctx, 1);

            if (desc.m_privateKeyPEM || desc.m_certificatePEM)
            {
                if (desc.m_certificatePEM)
                {
                    m_certificate = CreateCertificateFromEncodedPEM(desc.m_certificatePEM);
                    if (m_certificate == nullptr || SSL_CTX_use_certificate(m_ctx, m_certificate) != 1)
                    {
                        return Driver::EC_SECURE_CERT;
                    }
                }
                else
                {
                    AZ_TracePrintf("GridMateSecure", "If a private key is provided, so must a corresponding certificate.\n");
                    return Driver::EC_SECURE_CONFIG;
                }

                if (desc.m_privateKeyPEM)
                {
                    m_privateKey = CreatePrivateKeyFromEncodedPEM(desc.m_privateKeyPEM);
                    if (m_privateKey == nullptr || SSL_CTX_use_PrivateKey(m_ctx, m_privateKey) != 1)
                    {
                        return Driver::EC_SECURE_PKEY;
                    }
                }
                else
                {
                    AZ_TracePrintf("GridMateSecure", "If a certificate is provided, so must a corresponding private key.\n");
                    return Driver::EC_SECURE_PKEY;
                }
            }
            
            // Determine if both client and server must be authenticated or only the server.
            // The default behavior only authenticates the server, and not the client.
            int verificationMode = SSL_VERIFY_PEER;
            if (desc.m_authenticateClient)
            {
                verificationMode = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
            }

            if (desc.m_certificateAuthorityPEM)
            {
                // SSL context should already have empty cert storage
                X509_STORE* caLocalStore = SSL_CTX_get_cert_store(m_ctx);
                if (caLocalStore == nullptr)
                {
                    return Driver::EC_SECURE_CA_CERT;
                }

                AZStd::vector<X509*> certificateChain;
                CreateCertificateChainFromEncodedPEM(desc.m_certificateAuthorityPEM, certificateChain);
                if (certificateChain.size() == 0)
                {
                    return Driver::EC_SECURE_CA_CERT;
                }

                for (auto certificate : certificateChain)
                {
                    X509_STORE_add_cert(caLocalStore, certificate);
                }
                SSL_CTX_set_verify(m_ctx, verificationMode, nullptr);
            }
            else
            {
                auto fnVerifyCertificate = [](int ok, X509_STORE_CTX* ctx) -> int
                {
                    // Called when a certificate has been received and needs to be verified (e.g.
                    // verify that it has been signed by the appropriate CA, has the correct
                    // hostname, etc).
                    (void)ok;
                    (void)ctx;
                    return 1;
                };
                SSL_CTX_set_verify(m_ctx, verificationMode, fnVerifyCertificate);
            }

            return Driver::EC_OK;
        }

        bool Teardown()
        {
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

            if (m_ctx)
            {
                // Calls to SSL_CTX_free() also free any attached X509_STORE objects.
                SSL_CTX_free(m_ctx);
                m_ctx = nullptr;
            }

            return ShutdownOpenSSL();
        }

        SSL_CTX* m_ctx; // main SSL context 
        EVP_PKEY* m_privateKey;
        X509* m_certificate;
    };
}

//////////////////////////////////////////////////////////////////////////
// StreamSecureSocketDriver
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    StreamSecureSocketDriver::StreamSecureSocketDriver(AZ::u32 maxConnections, AZ::u32 maxPacketSize, AZ::u32 inboundBufferSize, AZ::u32 outboundBufferSize)
        : StreamSocketDriver(maxConnections, maxPacketSize, inboundBufferSize, outboundBufferSize)
    {
    }

    StreamSecureSocketDriver::~StreamSecureSocketDriver()
    {
        m_handle.release();
        m_connectionFactory = [](AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize)
        {
            (void)inboundBufferSize;
            (void)outputBufferSize;
            AZ_TracePrintf("GridMateSecure", "Tried to create a new connection during shutdown.\n");
            return (SecureConnection*)nullptr;
        };
    }

    Driver::ResultCode StreamSecureSocketDriver::InitializeSecurity(AZ::s32 familyType, const char* address, AZ::u32 port, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize, StreamSecureSocketDriverDesc& desc)
    {
        Driver::ResultCode code = Initialize(familyType, address, port, false, receiveBufferSize, sendBufferSize);
        if (code != EC_OK)
        {
            return code;
        }
        SecureContextHandleImpl* pHandle = aznew SecureContextHandleImpl();
        code = pHandle->Prepare(desc);
        if (code != Driver::EC_OK)
        {
            delete pHandle;
            return code;
        }
        m_handle.reset(pHandle);
        m_connectionFactory = [this](AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize)
        {
            if (m_handle)
            {
                return aznew SecureConnection(inboundBufferSize, outputBufferSize, *m_handle.get());
            }
            return (SecureConnection*)nullptr;
        };
        return EC_OK;
    }
}

//////////////////////////////////////////////////////////////////////////
// SecureConnectionContext
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    struct SecureConnectionContextImpl
        : public StreamSecureSocketDriver::SecureConnectionContext
    {
        GM_CLASS_ALLOCATOR(SecureConnectionContextImpl);

        explicit SecureConnectionContextImpl(SSL_CTX* sslContext, AZ::u32 scratchSize)
            : m_ssl(nullptr)
            , m_bioIn(nullptr)
            , m_bioOut(nullptr)
            , m_sslCtx(sslContext)
            , m_scratch(nullptr)
            , m_scratchSize(scratchSize)
        {
        }

        ~SecureConnectionContextImpl() override
        {
            Teardown();
        }

        SecureConnectionContextImpl(const SecureConnectionContextImpl &) = delete;
        SecureConnectionContextImpl& operator =(const SecureConnectionContextImpl&) = delete;

        bool PrepareToAccept() override
        {
            if (Prepare())
            {
                SSL_set_accept_state(m_ssl);
                return true;
            }
            return false;
        }

        bool PrepareToConnect() override
        {
            if (Prepare())
            {
                SSL_set_connect_state(m_ssl);
                return true;
            }
            return false;
        }

        bool Prepare()
        {
            do
            {
                m_ssl = SSL_new(m_sslCtx);
                if (m_ssl == nullptr)
                {
                    AZ_Warning("GridMate", m_ssl == nullptr, "SSL_new() failed!");
                    return false;
                }

                m_bioIn = BIO_new(BIO_s_mem());
                if (m_bioIn == nullptr)
                {
                    AZ_Warning("GridMate", m_bioIn == nullptr, "BIO_new() for m_bioIn failed.");
                    break;
                }

                m_bioOut = BIO_new(BIO_s_mem());
                if (m_bioOut == nullptr)
                {
                    AZ_Warning("GridMate", m_bioOut == nullptr, "BIO_new() for m_bioOut failed.");
                    break;
                }

                m_scratch = static_cast<char*>(azmalloc(m_scratchSize));
                if (m_scratch == nullptr)
                {
                    AZ_Warning("GridMate", m_scratch == nullptr, "Could not allocate scratch buffer.");
                    break;
                }
            }
            while (false);

            // did everything successfully create and/or allocate?
            if (m_ssl && m_bioIn && m_bioOut && m_scratch)
            {
                BIO_set_mem_eof_return(m_bioIn, -1);
                BIO_set_mem_eof_return(m_bioOut, -1);
                SSL_set_bio(m_ssl, m_bioIn, m_bioOut);
                return true;
            }

            Teardown();
            return false;
        }

        void Teardown()
        {
            if (m_scratch != nullptr)
            {
                azfree(m_scratch);
                m_scratch = nullptr;
            }

            if (m_ssl != nullptr)
            {
                SSL_free(m_ssl);
                m_ssl = nullptr;
            }

            if (m_bioIn != nullptr)
            {
                BIO_free(m_bioIn);
                m_bioIn = nullptr;
            }

            if (m_bioOut != nullptr)
            {
                BIO_free(m_bioOut);
                m_bioOut = nullptr;
            }
        }

        SSL* m_ssl;                     // the SSL which represents a "connection"
        BIO* m_bioIn;                   // we use memory read BIO
        BIO* m_bioOut;                  // we use memory write BIO
        SSL_CTX* m_sslCtx;              // the parent context this SSL instance belongs
        char* m_scratch;                // a scratch buffer to temporarily read and write
        const AZ::u32 m_scratchSize;    // the size of the scratch buffer
    };
}

//////////////////////////////////////////////////////////////////////////
// StreamSecureSocketDriver::SecureConnection
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    SecureConnectionContextImpl* CastContext(AZStd::unique_ptr<StreamSecureSocketDriver::SecureConnectionContext>& ptr)
    {
        return static_cast<SecureConnectionContextImpl*>(ptr.get());
    }

    StreamSecureSocketDriver::SecureConnection::SecureConnection(AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize, StreamSecureSocketDriver::SecureContextHandle& handle)
        : Connection(inboundBufferSize, outputBufferSize)
        , m_inboundRawBuffer(inboundBufferSize)
    {
        SecureContextHandleImpl* refHandle = static_cast<SecureContextHandleImpl*>(&handle);
        m_context.reset(aznew SecureConnectionContextImpl(refHandle->m_ctx, AZ::GetMax(inboundBufferSize, outputBufferSize)));
    }

    StreamSecureSocketDriver::SecureConnection::~SecureConnection()
    {
        m_context.release();
    }

    bool StreamSecureSocketDriver::SecureConnection::SendPacket(const char* data, AZ::u32 dataSize)
    {
        const SecureConnectionContextImpl* ctx = CastContext(m_context);
        const AZ::u32 kMaxPacketSendSize = (1 << (sizeof(AZ::u16) * 8)) - 1;
        const AZ::s32 kPacketDelimeterSize = sizeof(decltype(Packet::m_size));

        if (nullptr == ctx || nullptr == ctx->m_ssl)
        {
            return false;
        }
        else if (!IsValidPacketDataSize(dataSize, kMaxPacketSendSize))
        {
            return false;
        }
        else if ((kPacketDelimeterSize + dataSize) > ctx->m_scratchSize)
        {
            AZ_TracePrintf("GridMate", "Failed to memory write the packet");
            return false;
        }

        AZ::u16 packetSize = static_cast<AZ::u16>(dataSize);
        packetSize = SocketOperations::HostToNetShort(packetSize);

        std::memcpy(ctx->m_scratch, &packetSize, kPacketDelimeterSize);
        std::memcpy(ctx->m_scratch + kPacketDelimeterSize, data, dataSize);

        AZ::s32 wrote = SSL_write(ctx->m_ssl, ctx->m_scratch, kPacketDelimeterSize + dataSize);
        if (wrote > 0)
        {
            AZ_Warning("GridMate", wrote == static_cast<AZ::s32>(kPacketDelimeterSize + dataSize), "SSL_write only wrote %d", wrote);
            return true;
        }
        else
        {
            // In this case a call to SSL_get_error with the return value of SSL_write() will yield SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE.
            if (SSL_get_error(ctx->m_ssl, wrote) == SSL_ERROR_WANT_READ || SSL_get_error(ctx->m_ssl, wrote) == SSL_ERROR_WANT_WRITE)
            {
                AZ_TracePrintf("GridMate", "Writing was blocked by an internal SSL process.\n");
                return true;
            }
        }
        return false;
    }

    bool StreamSecureSocketDriver::SecureConnection::OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            if (m_context->PrepareToAccept())
            {
                return true;
            }
            else
            {
                sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                return true;
            }
        }

        if (e.id != ConnectionEvents::CE_UPDATE)
        {
            return StreamSocketDriver::Connection::OnStateAccept(sm, e);
        }

        if (!m_socketErrors.empty())
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        return ProcessHandshake();
    }

    bool StreamSecureSocketDriver::SecureConnection::OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            if (m_context->PrepareToConnect())
            {
                return true;
            }
            else
            {
                sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                return true;
            }
        }

        if (e.id != ConnectionEvents::CE_UPDATE)
        {
            return StreamSocketDriver::Connection::OnStateConnect(sm, e);
        }

        if (!m_socketErrors.empty())
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        return ProcessHandshake();
    }

    Driver::ErrorCodes StreamSecureSocketDriver::SecureConnection::ReadBytes(void* buf, AZ::u32& num, bool& resume)
    {
        const SecureConnectionContextImpl* ctx = CastContext(m_context);

        resume = true;
        const AZ::u32 bytesToRead = AZ::GetMin(num, ctx->m_scratchSize);
        AZ::s32 ret = SSL_read(ctx->m_ssl, buf, bytesToRead);
        switch (SSL_get_error(ctx->m_ssl, ret))
        {
            case SSL_ERROR_NONE:
            {
                num = ret;
                return Driver::EC_OK;
            }
            case SSL_ERROR_ZERO_RETURN:
            {
                // end of data
                num = 0;
                resume = false;
                return Driver::EC_OK;
            }
            case SSL_ERROR_WANT_READ:
            {
                num = 0;
                return Driver::EC_OK;
            }
            case SSL_ERROR_WANT_WRITE:
            {
                num = 0;
                return Driver::EC_OK;
            }
            default:
            {
                num = 0;
                resume = false;
                break;
            }
        }
        return Driver::EC_RECEIVE;
    }

    bool StreamSecureSocketDriver::SecureConnection::WriteIntoRingBufferSafe(RingBuffer& inboundBuffer)
    {
        AZ::u32 inOutBytesSize = 0;
        char* bytesBuffer = inboundBuffer.ReserveForWrite(inOutBytesSize);
        if (bytesBuffer == nullptr || inOutBytesSize == 0)
        {
            AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
            return false;
        }

        bool resume = true;
        if (ReadBytes(bytesBuffer, inOutBytesSize, resume) != Driver::EC_OK)
        {
            StoreLastSocketError();
            m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        // should continue?
        if (!resume)
        {
            m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
            return true;
        }

        // nothing read in the buffer, thus nothing to store in inboundBuffer
        if (inOutBytesSize == 0)
        {
            return false;
        }

        bool bWroteToMarker = false;
        inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
        if (bWroteToMarker && inOutBytesSize > 0)
        {
            // attempt to fill out the other side of the ring buffer
            inOutBytesSize = 0;
            bytesBuffer = inboundBuffer.ReserveForWrite(inOutBytesSize);
            if (bytesBuffer == nullptr || inOutBytesSize == 0)
            {
                AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
                return false;
            }
            if (ReadBytes(bytesBuffer, inOutBytesSize, resume) != Driver::EC_OK)
            {
                StoreLastSocketError();
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                return true;
            }
            if (!resume)
            {
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                return true;
            }
            inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
        }
        return false;
    }

    bool StreamSecureSocketDriver::SecureConnection::OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id != ConnectionEvents::CE_UPDATE)
        {
            return StreamSocketDriver::Connection::OnStateEstablished(sm, e);
        }

        if (!m_socketErrors.empty())
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        bool didSwitch = ProcessNetwork();
        if (didSwitch)
        {
            return didSwitch;
        }

        const SecureConnectionContextImpl* ctx = CastContext(m_context);
        do
        {
            // anything to read from SSL connection (aka inbound traffic)?
            if (WriteIntoRingBufferSafe(m_inboundBuffer))
            {
                return true;
            }
        } 
        while (SSL_pending(ctx->m_ssl));

        // no state change, yet
        return false;
    }

    bool StreamSecureSocketDriver::SecureConnection::ProcessNetwork()
    {
        const SecureConnectionContextImpl* ctx = CastContext(m_context);

        // read from the socket first
        bool switchState = false;
        ProcessInbound(switchState, m_inboundRawBuffer);
        if (switchState)
        {
            return true;
        }

        // network traffic to process?
        if (m_inboundRawBuffer.GetSpaceToRead() > 0)
        {
            AZ::u32 bytesToRead = AZ::GetMin(ctx->m_scratchSize, m_inboundRawBuffer.GetSpaceToRead());
            if (m_inboundRawBuffer.Fetch(ctx->m_scratch, bytesToRead))
            {
                AZ::s32 wrote = BIO_write(ctx->m_bioIn, ctx->m_scratch, bytesToRead);
                if (wrote <= 0)
                {
                    if (!BIO_should_retry(ctx->m_bioIn))
                    {
                        StoreLastSocketError();
                        return false;
                    }
                }
            }
        }

        // anything written into the SSL to be sent out?
        size_t pending = BIO_ctrl_pending(ctx->m_bioOut);
        while (pending > 0 && m_outboundBuffer.GetSpaceToWrite() >= static_cast<AZ::u32>(pending))
        {
            AZ::s32 read = BIO_read(ctx->m_bioOut, ctx->m_scratch, ctx->m_scratchSize);
            if (read > 0)
            {
                m_outboundBuffer.Store(ctx->m_scratch, static_cast<AZ::u32>(read));
            }
            else if (read == 0)
            {
                // closed
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                return true;
            }
            else
            {
                if (BIO_should_retry(ctx->m_bioOut))
                {
                    // try again latter
                    break;
                }
                StoreLastSocketError();
                return false;
            }
            pending = BIO_ctrl_pending(ctx->m_bioOut);
        }

        // process any more out going network traffic
        switchState = false;
        ProcessOutbound(switchState, m_outboundBuffer);
        if (switchState)
        {
            return true;
        }

        // no state machine change
        return false;
    }

    bool StreamSecureSocketDriver::SecureConnection::ProcessHandshake()
    {
        if (ProcessNetwork())
        {
            return true;
        }

        const SecureConnectionContextImpl* ctx = CastContext(m_context);

        // is the handshake done?
        if (SSL_is_init_finished(ctx->m_ssl) == 1)
        {
            m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::ESTABLISHED));
            return true;
        }
        else
        {
            // update the SSL internals
            AZ::s32 hsRet = SSL_do_handshake(ctx->m_ssl);
            if (hsRet < 0)
            {
                AZ::s32 exRet = SSL_get_error(ctx->m_ssl, hsRet);
                if (exRet != SSL_ERROR_WANT_READ && exRet != SSL_ERROR_WANT_WRITE)
                {
                    // The TLS/SSL handshake was not successful because a fatal error occurred either at the protocol level or a connection failure occurred.
                    StoreLastSocketError();
                    m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                    return true;
                }
            }
        }

        // no state change, yet
        return false;
    }
}

#endif // AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL
