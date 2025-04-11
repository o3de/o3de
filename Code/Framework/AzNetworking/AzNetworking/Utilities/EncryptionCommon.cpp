/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/EncryptionCommon.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h> // For AZ_MAX_PATH_LEN
#include <AzCore/Console/Console.h>
#include <AzCore/Console/ILogger.h>

#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

#if defined(OPENSSL_THREADS)
//  thread support enabled
#elif AZ_TRAIT_USE_OPENSSL
#   error OpenSSL threading support is not enabled
#endif

namespace AzNetworking
{
    AZ_CVAR(AZ::CVarFixedString, net_SslExternalCertificateFile, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The filename of the EXTERNAL (server to client) certificate chain in PEM format");
    AZ_CVAR(AZ::CVarFixedString, net_SslExternalPrivateKeyFile,  "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The filename of the EXTERNAL (server to client) private key file in PEM format");
    AZ_CVAR(AZ::CVarFixedString, net_SslExternalContextPassword, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::IsInvisible, "The password required for the EXTERNAL (server to client) private certificate");

    AZ_CVAR(AZ::CVarFixedString, net_SslInternalCertificateFile, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The filename of the INTERNAL (server to server only) certificate chain in PEM format");
    AZ_CVAR(AZ::CVarFixedString, net_SslInternalPrivateKeyFile,  "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The filename of the INTERNAL (server to server only) private key file in PEM format");
    AZ_CVAR(AZ::CVarFixedString, net_SslInternalContextPassword, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::IsInvisible, "The password required for the INTERNAL (server to server only) private certificate");

    AZ_CVAR(AZ::CVarFixedString, net_SslCertCiphers, "ECDHE-RSA-AES256-GCM-SHA384", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The cipher suite to use when using cert based key exchange");

    AZ_CVAR(int32_t,    net_SslMaxCertDepth,                3, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The maximum depth allowed for cert chaining validation");
    AZ_CVAR(AZ::TimeMs, net_RotateCookieTimer, AZ::TimeMs{50}, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Number of milliseconds to wait before generating a new DTLS cookie for handshaking");
    AZ_CVAR(bool,       net_SslEnablePinning,            true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "If enabled, the public certificates on the local and remote endpoints will be compared to ensure they match exactly");
    AZ_CVAR(bool,       net_SslValidateExpiry,           true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "If enabled, expiration dates on the certificate will be checked for validity");
    AZ_CVAR(bool,       net_SslAllowSelfSigned,          true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "If enabled, self-signed certs will not cause a validation error if they are otherwise considered trusted");

    void PrintSslErrorStack()
    {
#if AZ_TRAIT_USE_OPENSSL

        #if OPENSSL_VERSION_NUMBER >= 0x30000000L
            const char *func = nullptr;
            const int32_t errorCode = static_cast<int32_t>(ERR_get_error_all(nullptr, nullptr, &func, nullptr, nullptr));
        #else
            const int32_t errorCode = static_cast<int32_t>(ERR_get_error());
        #endif 
        const int32_t systemError = GetLastNetworkError();

        switch (errorCode)
        {
            case SSL_ERROR_NONE:
                AZLOG_ERROR("%X - SSL_ERROR_NONE: last system error is (%d:%s)", errorCode, systemError, GetNetworkErrorDesc(systemError));
                break;
            case SSL_ERROR_ZERO_RETURN:
                AZLOG_ERROR("%X - SSL_ERROR_ZERO_RETURN: connection has been closed", errorCode);
                break;
            case SSL_ERROR_WANT_READ:
                AZLOG_ERROR("%X - SSL_ERROR_WANT_READ: socket is non-blocking, read buffer is empty", errorCode);
                break;
            case SSL_ERROR_WANT_WRITE:
                AZLOG_ERROR("%X - SSL_ERROR_WANT_WRITE: socket is non-blocking, write buffer is full", errorCode);
                break;
            case SSL_ERROR_WANT_CONNECT:
                AZLOG_ERROR("%X - SSL_ERROR_WANT_CONNECT: socket is non-blocking, connect failed and should be retried", errorCode);
                break;
            case SSL_ERROR_WANT_ACCEPT:
                AZLOG_ERROR("%X - SSL_ERROR_WANT_ACCEPT: socket is non-blocking, accept failed and should be retried", errorCode);
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                AZLOG_ERROR("%X - SSL_ERROR_WANT_X509_LOOKUP: operation did not complete, SSL_CTX_set_client_cert_cb() has asked to be called again, operation should be retried", errorCode);
                break;
            case SSL_ERROR_SYSCALL:
                AZLOG_ERROR("%X - SSL_ERROR_SYSCALL: system error, check errno (%d:%s)", errorCode, systemError, GetNetworkErrorDesc(systemError));
                break;
            case SSL_ERROR_SSL:
                #if OPENSSL_VERSION_NUMBER >= 0x30000000L
                    AZLOG_ERROR("%X - SSL_ERROR_SSL: lib %s, func %s, reason %s", errorCode, ERR_lib_error_string(errorCode), func, ERR_reason_error_string(errorCode));
                #else
                    AZLOG_ERROR("%X - SSL_ERROR_SSL: lib %s, func %s, reason %s", errorCode, ERR_lib_error_string(errorCode), ERR_func_error_string(errorCode), ERR_reason_error_string(errorCode));
                #endif
                break;
            default:
                #if OPENSSL_VERSION_NUMBER >= 0x30000000L
                    AZLOG_ERROR("%X - Unknown error code: lib %s, func %s, reason %s", errorCode, ERR_lib_error_string(errorCode), func, ERR_reason_error_string(errorCode));
                #else
                    AZLOG_ERROR("%X - Unknown error code: lib %s, func %s, reason %s", errorCode, ERR_lib_error_string(errorCode), ERR_func_error_string(errorCode), ERR_reason_error_string(errorCode));
                #endif
                break;
        }
#endif
    }

#if AZ_TRAIT_USE_OPENSSL
    static const uint32_t MaxCookieHistory = 8;
    static bool g_encryptionInitialized = false;
    static int32_t g_azNetworkingTrustDataIndex = 0;
    static AZ::TimeMs g_lastCookieTimestamp = AZ::Time::ZeroTimeMs;
    static uint64_t g_validCookieArray[MaxCookieHistory];
    static uint32_t g_cookieReplaceIndex = 0;

    static void GetCertificatePaths(TrustZone trustZone, AZStd::string& certificatePath, AZStd::string& privateKeyPath)
    {
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            AZ::CVarFixedString certificateFile = "";
            AZ::CVarFixedString privateKeyFile = "";
            if (trustZone == TrustZone::ExternalClientToServer)
            {
                console->GetCvarValue("net_SslExternalCertificateFile", certificateFile);
                console->GetCvarValue("net_SslExternalPrivateKeyFile", privateKeyFile);
            }
            else
            {
                console->GetCvarValue("net_SslInternalCertificateFile", certificateFile);
                console->GetCvarValue("net_SslInternalPrivateKeyFile", privateKeyFile);
            }

            // Check asset directory when provided file paths are not valid
            AZStd::string assetDir = "";
            if (!AZ::IO::SystemFile::Exists(certificateFile.c_str()) &&
                !AZ::IO::SystemFile::Exists(privateKeyFile.c_str()) &&
                AZ::IO::FileIOBase::GetInstance() != nullptr)
            {
                char buffer[AZ_MAX_PATH_LEN];
                AZ::IO::FileIOBase::GetInstance()->ResolvePath("@products@/", buffer, sizeof(buffer));
                assetDir = AZStd::string(buffer);
            }

            certificatePath = assetDir + certificateFile.c_str();
            privateKeyPath = assetDir + privateKeyFile.c_str();
        }
    }

    static bool ValidatePinnedCertificate(X509* remoteCert, TrustZone trustZone)
    {
        // Get the remote certificate
        AZStd::vector<uint8_t> remoteCertificate;
        {
            if (remoteCert == nullptr)
            {
                AZLOG_ERROR("Failed to retrieve the remote certificate, pinned certificate validation failed");
                PrintSslErrorStack();
                return false;
            }

            const int32_t remoteCertLength = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(remoteCert), nullptr);
            if (remoteCertLength <= 0)
            {
                AZLOG_ERROR("Failed to retrieve the remote certificate, pinned certificate validation failed");
                PrintSslErrorStack();
                return false;
            }

            remoteCertificate.resize(remoteCertLength);
            uint8_t* remoteCertificateData = remoteCertificate.data();
            i2d_X509_PUBKEY(X509_get_X509_PUBKEY(remoteCert), &remoteCertificateData);
        }

        // Load our local copy of the certificate (what we expect the remote endpoint to present)
        AZStd::vector<uint8_t> localCertificate;
        {
            AZStd::string certificatePath;
            AZStd::string unusedPrivateKeyPath;
            GetCertificatePaths(trustZone, certificatePath, unusedPrivateKeyPath);

            AZ::IO::FileIOStream stream(certificatePath.c_str(), AZ::IO::OpenMode::ModeRead);
            const AZ::IO::SizeType publicCertLength = stream.GetLength();
            if (publicCertLength <= 0)
            {
                AZLOG_ERROR("Local public certificate is not valid, returned %d bytes", static_cast<int32_t>(publicCertLength));
                return false;
            }

            AZStd::vector<uint8_t> publicCertData;
            publicCertData.resize(publicCertLength);
            AZ::IO::SizeType bytesRead = stream.Read(publicCertLength, publicCertData.data());
            publicCertData.resize(bytesRead);
            stream.Close();

            const uint8_t* publicCertBytes = publicCertData.data();

            BIO* certBio = BIO_new(BIO_s_mem());
            BIO_write(certBio, publicCertBytes, static_cast<int32_t>(publicCertLength));

            X509* localCert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
            if (localCert == nullptr)
            {
                BIO_free(certBio);
                AZLOG_ERROR("Failed to convert public cert to X509, pinned certificate validation failed");
                PrintSslErrorStack();
                return false;
            }

            const int32_t localCertLength = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(localCert), nullptr);
            if (localCertLength <= 0)
            {
                BIO_free(certBio);
                X509_free(localCert);
                AZLOG_ERROR("Failed to retrieve the local certificate, pinned certificate validation failed");
                PrintSslErrorStack();
                return false;
            }

            localCertificate.resize(localCertLength);
            uint8_t* localCertificateData = localCertificate.data();
            i2d_X509_PUBKEY(X509_get_X509_PUBKEY(localCert), &localCertificateData);

            BIO_free(certBio);
            X509_free(localCert);
        }

        // Validate the certificates match
        {
            const int32_t localSize = static_cast<int32_t>(localCertificate.size());
            const int32_t remoteSize = static_cast<int32_t>(remoteCertificate.size());
            if (localSize != remoteSize)
            {
                AZLOG_ERROR("Validation failed, certs are different sizes; local %d bytes, remote %d bytes", localSize, remoteSize);
                return false;
            }

            if (memcmp(localCertificate.data(), remoteCertificate.data(), localSize) != 0)
            {
                AZLOG_ERROR("Validation failed, certificate content mismatch");
                return false;
            }
        }

        return true;
    }

    // Validates a certificate chain, returns OpenSslResultSuccess on success, OpenSslResultFailure on fail
    static int32_t ValidateCertificateCallback(int32_t preverifyOk, X509_STORE_CTX* context)
    {
        int32_t result = preverifyOk; // Start by assigning pre-verification to our result

        X509* err_cert = X509_STORE_CTX_get_current_cert(context);

        if (result != OpenSslResultSuccess)
        {
            int32_t error = X509_STORE_CTX_get_error(context);

            AZLOG_WARN("OpenSSL preverification failed with (%d: %s)", error, X509_verify_cert_error_string(error));

            switch (error)
            {
            case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                if (net_SslAllowSelfSigned)
                {
                    AZLOG_WARN("net_SslAllowSelfSigned is *enabled*, clearing X509 certificate validation failure");
                    X509_STORE_CTX_set_error(context, X509_V_OK);
                    result = OpenSslResultSuccess;
                }
                break;
            }
        }

        // Catch a too long certificate chain. The depth limit set using SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so that
        // whenever the "depth>verify_depth" condition is met, we have violated the limit and want to log this error condition.
        // We must do it here, because the CHAIN_TOO_LONG error would not be found explicitly; only errors introduced by cutting off the
        // additional certificates would be logged.
        const int32_t depth = X509_STORE_CTX_get_error_depth(context);
        if (depth > net_SslMaxCertDepth)
        {
            result = OpenSslResultFailure;
            X509_STORE_CTX_set_error(context, X509_V_ERR_CERT_CHAIN_TOO_LONG);
        }

        // Validate certificate before and after times
        if (net_SslValidateExpiry)
        {
            const ASN1_TIME* notBeforeTime = X509_get_notBefore(err_cert);
            const int32_t beforeTimeResult = X509_cmp_current_time(notBeforeTime);

            if (beforeTimeResult >= 0)
            {
                AZLOG_WARN("OpenSSL preverification failed, certificate is not yet valid (before time result %d)", beforeTimeResult);
                X509_STORE_CTX_set_error(context, X509_V_ERR_CERT_NOT_YET_VALID);
                result = OpenSslResultFailure;
            }

            const ASN1_TIME* notAfterTime = X509_get_notAfter(err_cert);
            const int32_t afterTimeResult = X509_cmp_current_time(notAfterTime);

            if (afterTimeResult <= 0)
            {
                AZLOG_WARN("OpenSSL preverification failed, certificate has expired (after time result %d)", afterTimeResult);
                X509_STORE_CTX_set_error(context, X509_V_ERR_CERT_HAS_EXPIRED);
                result = OpenSslResultFailure;
            }
        }

        if (net_SslEnablePinning)
        {
            SSL* sslSocket = reinterpret_cast<SSL*>(X509_STORE_CTX_get_ex_data(context, SSL_get_ex_data_X509_STORE_CTX_idx()));
            SSL_CTX* sslContext = SSL_get_SSL_CTX(sslSocket);
            TrustZone trustZone = static_cast<TrustZone>(reinterpret_cast<uintptr_t>(SSL_CTX_get_ex_data(sslContext, g_azNetworkingTrustDataIndex)));
            if (!ValidatePinnedCertificate(err_cert, trustZone))
            {
                X509_STORE_CTX_set_error(context, X509_V_ERR_UNSPECIFIED);
                result = OpenSslResultFailure;
            }
        }

        AZLOG_INFO("Certificate validation %s", (result == OpenSslResultSuccess) ? "passed" : "failed");

        return result;
    }

    // Generates a cookie, returns OpenSslResultSuccess on success, OpenSslResultFailure on fail
    static int32_t GenerateCookieCallback([[maybe_unused]] SSL* sslSocket, uint8_t* cookieData, uint32_t* cookieLength)
    {
        const AZ::TimeMs currentTime = AZ::GetElapsedTimeMs();

        if ((currentTime - g_lastCookieTimestamp) > net_RotateCookieTimer)
        {
            uint64_t newCookie = CryptoRand64();

            g_cookieReplaceIndex = (g_cookieReplaceIndex + 1) % MaxCookieHistory;
            g_validCookieArray[g_cookieReplaceIndex] = newCookie;
            g_lastCookieTimestamp = currentTime;
        }

        if (cookieLength != nullptr)
        {
            *cookieLength = sizeof(g_validCookieArray[g_cookieReplaceIndex]);
        }

        if (cookieData != nullptr)
        {
            memcpy(cookieData, reinterpret_cast<uint8_t*>(&g_validCookieArray[g_cookieReplaceIndex]), sizeof(g_validCookieArray[g_cookieReplaceIndex]));
        }

        return OpenSslResultSuccess;
    }

    // Verifies a cookie, returns OpenSslResultSuccess on success, OpenSslResultFailure on fail
    static int32_t VerifyCookieCallback([[maybe_unused]] SSL* sslSocket, const uint8_t* cookieData, uint32_t cookieLength)
    {
        if (cookieLength != sizeof(uint64_t))
        {
            // This should be logged somehow, but since this is part of DOS prevention I don't think I should *ACTUALLY* log here..
            return OpenSslResultFailure;
        }

        uint64_t cookie = *reinterpret_cast<const uint64_t*>(cookieData);

        for (uint32_t i = 0; i < MaxCookieHistory; ++i)
        {
            if (g_validCookieArray[i] == cookie)
            {
                return OpenSslResultSuccess;
            }
        }

        // This should be logged somehow, but since this is part of DOS prevention I don't think I should *ACTUALLY* log here..
        return OpenSslResultFailure;
    }
#endif

    bool EncryptionLayerInit()
    {
#if AZ_TRAIT_USE_OPENSSL
        if (!g_encryptionInitialized)
        {
            SSL_library_init();
            SSL_load_error_strings();
            #if !(OPENSSL_VERSION_NUMBER >= 0x30000000L)
                ERR_load_BIO_strings();
            #endif
            OpenSSL_add_all_algorithms();
            g_azNetworkingTrustDataIndex = SSL_get_ex_new_index(0, const_cast<void*>(reinterpret_cast<const void*>("AzNetworking TrustZone data index")), nullptr, nullptr, nullptr);

            g_encryptionInitialized = true;
        }
#endif
        return true;
    }

    bool EncryptionLayerShutdown()
    {
#if AZ_TRAIT_USE_OPENSSL
        if (g_encryptionInitialized)
        {
            ERR_free_strings();
            EVP_cleanup();
            CRYPTO_cleanup_all_ex_data();

            g_encryptionInitialized = false;
        }
#endif
        return true;
    }

#if AZ_TRAIT_USE_OPENSSL
    //! @struct ScopedSslContextFree
    //! @brief helper structure that manages RAII teardown of the SSL context if an error condition is encountered during creation
    //! If the destructor is hit and the context is non-null, this means an error has occurred and we are bailing from the create operation
    //! To release the context with no error, call ReleaseSslContextWithoutFree()
    struct ScopedSslContextFree
    {
        ScopedSslContextFree(SSL_CTX* sslContext) : m_sslContext(sslContext) {}

        ~ScopedSslContextFree()
        {
            if (m_sslContext != nullptr)
            {
                PrintSslErrorStack();
                SSL_CTX_free(m_sslContext);
            }
        }

        void ReleaseSslContextWithoutFree()
        {
            m_sslContext = nullptr;
        }

        SSL_CTX* m_sslContext = nullptr;
    };
#endif

    SSL_CTX* CreateSslContext([[maybe_unused]] SslContextType contextType, [[maybe_unused]] TrustZone trustZone)
    {
#if AZ_TRAIT_USE_OPENSSL
        if (!g_encryptionInitialized)
        {
            AZLOG_ERROR("SSL library has not been initialized for the current dll");
            return nullptr;
        }

        SSL_CTX* context = nullptr;

        switch (contextType)
        {
        case SslContextType::TlsGeneric:
            context = SSL_CTX_new(TLS_method());
            break;
        case SslContextType::TlsClient:
            context = SSL_CTX_new(TLS_client_method());
            break;
        case SslContextType::TlsServer:
            context = SSL_CTX_new(TLS_server_method());
            break;
        case SslContextType::DtlsGeneric:
            context = SSL_CTX_new(DTLS_method());
            break;
        case SslContextType::DtlsClient:
            context = SSL_CTX_new(DTLS_client_method());
            break;
        case SslContextType::DtlsServer:
            context = SSL_CTX_new(DTLS_server_method());
            break;
        }

        if (context == nullptr)
        {
            PrintSslErrorStack();
            return nullptr;
        }

        ScopedSslContextFree scopedFree(context);

        void* trustLevelPtr = reinterpret_cast<void*>(static_cast<uintptr_t>(trustZone));
        if (SSL_CTX_set_ex_data(context, g_azNetworkingTrustDataIndex, trustLevelPtr) != OpenSslResultSuccess)
        {
            AZLOG_ERROR("Failed to store the trust level on created SSL context");
            return nullptr;
        }

        // Enable automatic retries for sends if renegotiation is required, makes our code simpler
        SSL_CTX_set_mode(context, SSL_MODE_AUTO_RETRY);

        AZStd::string certificatePath;
        AZStd::string privateKeyPath;
        GetCertificatePaths(trustZone, certificatePath, privateKeyPath);

        // Returns 1 on success, and so returns.. not 1 on error? actual error is in error stack
        if (SSL_CTX_use_certificate_chain_file(context, certificatePath.c_str()) != OpenSslResultSuccess)
        {
            AZLOG_ERROR("Failed to load the certificate chain file");
            return nullptr;
        }

        // If we're accepting connections, set up password to access the private certificate
        // @KB TODO: plaintext passwords are bad...
        if ((contextType == SslContextType::TlsGeneric)
         || (contextType == SslContextType::TlsServer)
         || (contextType == SslContextType::DtlsGeneric)
         || (contextType == SslContextType::DtlsServer))
        {
            const AZ::CVarFixedString contextPassword = (trustZone == TrustZone::ExternalClientToServer) ? net_SslExternalContextPassword : net_SslInternalContextPassword;

            SSL_CTX_set_default_passwd_cb(context, nullptr);
            SSL_CTX_set_default_passwd_cb_userdata(context, (void*)contextPassword.c_str());

            if (SSL_CTX_use_PrivateKey_file(context, privateKeyPath.c_str(), SSL_FILETYPE_PEM) != OpenSslResultSuccess)
            {
                AZLOG_ERROR("Failed to load private certificate");
                return nullptr;
            }
        }

        // Validate the clients certificate only on handshake
        SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, ValidateCertificateCallback);

        // Only support a single cipher suite in OpenSSL that supports:
        //
        //  ECDHE       Primary key exchange using ephemeral elliptic curve diffie-hellman.
        //  RSA         Authentication (public and private key) used to sign ECDHE parameters and can be checked against a CA.
        //  AES256      AES cipher for symmetric key encryption using a 256-bit key.
        //  GCM         Mode of operation for symmetric key encryption.
        //  SHA384      SHA-2 hashing algorithm.
        const AZ::CVarFixedString ciphers = net_SslCertCiphers;
        if (SSL_CTX_set_cipher_list(context, ciphers.c_str()) != OpenSslResultSuccess)
        {
            AZLOG_ERROR("Failed to set supported ciphers");
            return nullptr;
        }

        // DTLS cookie callbacks for UDP based spoof DOS mitigation
        SSL_CTX_set_cookie_generate_cb(context, GenerateCookieCallback);
        SSL_CTX_set_cookie_verify_cb(context, VerifyCookieCallback);

        // Automatically generate parameters for elliptic-curve diffie-hellman (i.e. curve type and coefficients).

        // note that the below generates a warning depending on your version of openssl - on some, these functions
        // are deprecated and do nothing, causing a warning

        AZ_PUSH_DISABLE_WARNING(, "-Wunused-value", "-Wunused-value")
        SSL_CTX_set_ecdh_auto(context, 1);
        AZ_POP_DISABLE_WARNING

        scopedFree.ReleaseSslContextWithoutFree();
        return context;
#else
        return nullptr;
#endif
    }

    void FreeSslContext([[maybe_unused]] SSL_CTX*& context)
    {
#if AZ_TRAIT_USE_OPENSSL
        if (context == nullptr)
        {
            return;
        }

        SSL_CTX_free(context);
        context = nullptr;
#endif
    }

    SSL* CreateSslForAccept([[maybe_unused]] SocketFd socketFd, [[maybe_unused]] SSL_CTX* context)
    {
#if AZ_TRAIT_USE_OPENSSL
        SSL* socket = SSL_new(context);
        if (socket == nullptr)
        {
            AZLOG_ERROR("SSL_new failed, could not create SSL socket wrapper instance");
            PrintSslErrorStack();
            return nullptr;
        }
        if (SSL_set_fd(socket, static_cast<int32_t>(socketFd)) != OpenSslResultSuccess)
        {
            AZLOG_ERROR("SSL_set_fd failed, could not bind SSL socket wrapper to socket");
            PrintSslErrorStack();
            Close(socket);
            return nullptr;
        }

        // This socket should be configured to accept connections
        SSL_set_accept_state(socket);
        return socket;
#else
        return nullptr;
#endif
    }

    SSL* CreateSslForConnect([[maybe_unused]] SocketFd socketFd, [[maybe_unused]] SSL_CTX* context)
    {
#if AZ_TRAIT_USE_OPENSSL
        SSL* socket = SSL_new(context);
        if (socket == nullptr)
        {
            AZLOG_ERROR("SSL_new failed, could not create SSL socket wrapper instance");
            PrintSslErrorStack();
            return nullptr;
        }
        if (SSL_set_fd(socket, static_cast<int32_t>(socketFd)) != OpenSslResultSuccess)
        {
            AZLOG_ERROR("SSL_set_fd failed, could not bind SSL socket wrapper to socket");
            PrintSslErrorStack();
            Close(socket);
            return nullptr;
        }

        // This socket should be configured to initiate connections
        SSL_set_connect_state(socket);
        return socket;
#else
        return nullptr;
#endif
    }

    void Close([[maybe_unused]] SSL*& sslSocket)
    {
#if AZ_TRAIT_USE_OPENSSL
        if (sslSocket == nullptr)
        {
            return;
        }

        // SSL_shutdown can do very bad things if the SSL context is in a bad state
        // Further, the documentation around safely using SSL_shutdown is extremely confusing and doesn't provide a functional example
        // We never terminate encryption on a connection to send further data in plain-text, we explicitly close TCP sockets and UDP virtual
        // connections are deleted Therefore just removing the call to SSL_shutdown for now
        // SSL_shutdown(sslSocket);
        SSL_free(sslSocket);
        sslSocket = nullptr;
#endif
    }

    bool SslErrorIsWouldBlock(int32_t errorCode)
    {
        return (errorCode == SSL_ERROR_WANT_READ)
            || (errorCode == SSL_ERROR_WANT_WRITE);
    }

    uint32_t CryptoRand32()
    {
#if AZ_TRAIT_USE_OPENSSL
        uint8_t buffer[sizeof(uint32_t)];
        RAND_bytes(buffer, sizeof(buffer));
        return *reinterpret_cast<uint32_t*>(buffer);
#else
        return 0;
#endif
    }

    uint64_t CryptoRand64()
    {
#if AZ_TRAIT_USE_OPENSSL
        uint8_t buffer[sizeof(uint64_t)];
        RAND_bytes(buffer, sizeof(buffer));
        return *reinterpret_cast<uint64_t*>(buffer);
#else
        return 0;
#endif
    }
} // namespace AzNetworking
