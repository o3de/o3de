/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CertificateManager
{
    class IDataSource;

    class CertificateManagerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////        

        //Retrieve Certificate Authority trust chain from configured data source of choice
        virtual bool HasCertificateAuthority() const = 0;
        virtual char* RetrieveCertificateAuthority() = 0;

        //Retrieve Private Key from configured data source of choice
        virtual bool HasPrivateKey() const = 0;
        virtual char* RetrievePrivateKey() = 0;

        //Retrieve public key from certificate from configured data source of choice
        virtual bool HasPublicKey() const = 0;
        virtual char* RetrievePublicKey() = 0;        
    };

    using CertificateManagerRequestsBus = AZ::EBus<CertificateManagerRequests>;
} // namespace CertificateManager
