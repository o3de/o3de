/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef INCLUDE_FILEDATASOURCEBUS_H
#define INCLUDE_FILEDATASOURCEBUS_H

#include <AzCore/EBus/EBus.h>

// Consider moving this into it's own gem to act as an extension, and allow the certificate manager to remain completely data source agnostic.
namespace CertificateManager
{
    class IDataSource;

    class FileDataSourceCreationRequest 
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        // Bus Functions
        virtual void CreateFileDataSource() = 0;
        virtual void DestroyFileDataSource() = 0;
    };
    
    using FileDataSourceCreationBus = AZ::EBus<FileDataSourceCreationRequest>;
    
    class FileDataSourceConfigurationRequest
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; ///< Only one service may provide this interface
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;        
        
        // Bus Functions
        virtual void ConfigureDataSource(const char* keyPath, const char* certPath, const char* caPath) = 0;

        virtual void ConfigurePrivateKey(const char* path) = 0;
        virtual void ConfigureCertificate(const char* path) = 0;
        virtual void ConfigureCertificateAuthority(const char* path) = 0;
    };
    
    using FileDataSourceConfigurationBus = AZ::EBus<FileDataSourceConfigurationRequest>;
}
#endif
