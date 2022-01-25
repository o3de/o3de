/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/PlatformIncl.h>

#include "Source/DataSource/FileDataSource.h"

namespace CertificateManager
{
    static bool ReadFileIntoString(const char* filename, AZStd::vector<char>& outBuffer)
    {
        AZStd::string certificatePath = "@products@/certificates/";
        certificatePath.append(filename);

        AZ::IO::FileIOBase* fileBase = AZ::IO::FileIOBase::GetInstance();

        if (!fileBase->Exists(certificatePath.c_str()))
        {
            AZ_Error("FileDataSource",false,"File(%s) does not exist.\n", certificatePath.c_str());
            return false;
        }

        AZ::IO::HandleType fileHandle;

        AZ::IO::Result fileResult = fileBase->Open(certificatePath.c_str(),AZ::IO::OpenMode::ModeRead,fileHandle);

        if (!fileResult)
        {
            AZ_Error("FileDataSource",false,"Failed to open file with result %i\n", fileResult.GetResultCode());
            return false;
        }

        AZ::u64 fileSize = 0;
        fileBase->Size(fileHandle,fileSize);

        if (fileSize == 0)
        {
            AZ_Error("FileDataSource",false,"Given empty file(%s) as certificate file.\n",certificatePath.c_str());
            return false;
        }

        outBuffer.resize(fileSize + 1);
        fileResult = fileBase->Read(fileHandle,outBuffer.data(),fileSize);

        if (!fileResult)
        {
            AZ_Error("FileDataSource",false,"Failed to read from file(%s) with result code(%i).\n",certificatePath.c_str(),fileResult.GetResultCode());
            return false;
        }

        return true;
    }

    FileDataSource::FileDataSource()
        : m_privateKeyPEM(nullptr)
        , m_certificatePEM(nullptr)
        , m_certificateAuthorityCertPEM(nullptr)
    {
        FileDataSourceConfigurationBus::Handler::BusConnect();
    }

    FileDataSource::~FileDataSource()
    {
        FileDataSourceConfigurationBus::Handler::BusDisconnect();

        azfree(m_privateKeyPEM);
        azfree(m_certificatePEM);
        azfree(m_certificateAuthorityCertPEM);
    }

    void FileDataSource::ConfigureDataSource(const char* keyPath, const char* certPath, const char* caPath)
    {
        ConfigurePrivateKey(keyPath);
        ConfigureCertificate(certPath);
        ConfigureCertificateAuthority(caPath);
    }

    void FileDataSource::ConfigurePrivateKey(const char* path)
    {
        if (m_privateKeyPEM != nullptr)
        {
            azfree(m_privateKeyPEM);
            m_privateKeyPEM = nullptr;
        }

        if (path != nullptr)
        {
            LoadGenericFile(path,m_privateKeyPEM);
        }
    }

    void FileDataSource::ConfigureCertificate(const char* path)
    {
        if (m_certificatePEM != nullptr)
        {
            azfree(m_certificatePEM);
            m_certificatePEM = nullptr;
        }

        if (path != nullptr)
        {
            LoadGenericFile(path,m_certificatePEM);
        }
    }

    void FileDataSource::ConfigureCertificateAuthority(const char* path)
    {
        if (m_certificateAuthorityCertPEM != nullptr)
        {
            azfree(m_certificateAuthorityCertPEM);
            m_certificateAuthorityCertPEM = nullptr;
        }

        if (path != nullptr)
        {
            LoadGenericFile(path,m_certificateAuthorityCertPEM);
        }
    }

    bool FileDataSource::HasCertificateAuthority() const
    {
        return m_certificateAuthorityCertPEM != nullptr;
    }

    char* FileDataSource::RetrieveCertificateAuthority()
    {
        return m_certificateAuthorityCertPEM;
    }

    bool FileDataSource::HasPublicKey() const
    {
        return m_certificatePEM != nullptr;
    }

    char* FileDataSource::RetrievePublicKey()
    {
        return m_certificatePEM;
    }

    bool FileDataSource::HasPrivateKey() const
    {
        return m_privateKeyPEM != nullptr;
    }

    char* FileDataSource::RetrievePrivateKey()
    {
        return m_privateKeyPEM;
    }

    void FileDataSource::LoadGenericFile(const char* filename, char* &destination)
    {
        if (filename)
        {
            AZStd::vector<char> contents;
            if (ReadFileIntoString(filename, contents))
            {
                if (destination != nullptr)
                {
                    azfree(destination);
                    destination = nullptr;
                }
                destination = reinterpret_cast<char*>(azmalloc(contents.size()));
                if (destination == nullptr)
                {
                    AZ_Error("CertificateManager", false, "Invalid destination for file input");
                    return;
                }
                memcpy(destination, contents.data(), contents.size());
            }
            else
            {
                AZ_Warning("CertificateManager", false, "Failed to read authentication file '%s'.", filename);
            }
        }
    }
} //namespace CertificateManager
