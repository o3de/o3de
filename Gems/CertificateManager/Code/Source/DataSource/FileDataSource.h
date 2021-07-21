/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef INCLUDE_FILEDATASOURCE_HEADER
#define INCLUDE_FILEDATASOURCE_HEADER

#include <AzCore/Memory/SystemAllocator.h>
#include <CertificateManager/DataSource/IDataSource.h>

#include <CertificateManager/DataSource/FileDataSourceBus.h>

namespace CertificateManager
{
    class FileDataSource 
        : public IDataSource
        , public FileDataSourceConfigurationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(FileDataSource,AZ::SystemAllocator,0);        

        FileDataSource();
        ~FileDataSource();

        // IDataSource
        bool HasCertificateAuthority() const override;
        char* RetrieveCertificateAuthority() override;

        bool HasPrivateKey() const override;
        char* RetrievePrivateKey() override;

        bool HasPublicKey() const override;
        char* RetrievePublicKey() override;        
        
        // FileDataSourceConfigurationBus
        void ConfigureDataSource(const char* keyPath, const char* certPath, const char* caPath) override;

        void ConfigurePrivateKey(const char* path) override;
        void ConfigureCertificate(const char* path) override;
        void ConfigureCertificateAuthority(const char* path) override;

    private:
        void LoadGenericFile(const char* filename, char* &destination);

        char* m_privateKeyPEM;
        char* m_certificatePEM;
        char* m_certificateAuthorityCertPEM;
    };
} //namespace CertificateManager

#endif
