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