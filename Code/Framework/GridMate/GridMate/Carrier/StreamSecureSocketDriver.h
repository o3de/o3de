/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
#include <GridMate_Traits_Platform.h>

#if AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL

#include <GridMate/Carrier/StreamSocketDriver.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/functional.h>

namespace GridMate
{
    /**
     * Handles TLS for TCP stream socket transportation
     */
    class StreamSecureSocketDriver
        : public StreamSocketDriver
    {
    public:
        struct StreamSecureSocketDriverDesc
        {
            StreamSecureSocketDriverDesc()
                : m_privateKeyPEM(nullptr)
                , m_certificatePEM(nullptr)
                , m_certificateAuthorityPEM(nullptr)
                , m_authenticateClient(false)
            {
            };

            const char* m_privateKeyPEM;               // A base-64 encoded PEM format private key.
            const char* m_certificatePEM;              // A base-64 encoded PEM format certificate.
            const char* m_certificateAuthorityPEM;     // A base-64 encoded PEM format CA root certificate.
            bool        m_authenticateClient;          // Ensure that a client must be authenticated (the server is always authenticated). Only required to be set on the server!
        };

        GM_CLASS_ALLOCATOR(StreamSecureSocketDriver);

        StreamSecureSocketDriver(AZ::u32 maxConnections = 32, AZ::u32 maxPacketSize = 1024 * 64, AZ::u32 inboundBufferSize = 1024 * 64, AZ::u32 outboundBufferSize = 1024 * 64);
        ~StreamSecureSocketDriver() override;

        // Security operation(s)
        ResultCode InitializeSecurity(AZ::s32 familyType, const char* address, AZ::u32 port, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize, StreamSecureSocketDriverDesc& desc);

    public:
        struct SecureContextHandle
        {
            virtual ~SecureContextHandle() {}
        };
        struct SecureConnectionContext
        {
            virtual ~SecureConnectionContext() {}
            virtual bool PrepareToAccept() = 0;
            virtual bool PrepareToConnect() = 0;
        };

    protected:
        class SecureConnection
            : public Connection
        {
        public:
            GM_CLASS_ALLOCATOR(SecureConnection);

            SecureConnection(AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize, SecureContextHandle& handle);
            ~SecureConnection() override;

            bool SendPacket(const char* data, AZ::u32 dataSize) override;

        protected:
            bool OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& e) override;
            bool OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& e) override;
            bool OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& e) override;

            bool ProcessNetwork();
            bool ProcessHandshake();

        private:
            Driver::ErrorCodes ReadBytes(void* buf, AZ::u32& num, bool& resume);
            bool WriteIntoRingBufferSafe(RingBuffer& inboundBuffer);

            AZStd::unique_ptr<SecureConnectionContext> m_context;
            RingBuffer m_inboundRawBuffer;
        };
        
        // for secure connections
        AZStd::unique_ptr<SecureContextHandle> m_handle;
    };
}

#endif // AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL
