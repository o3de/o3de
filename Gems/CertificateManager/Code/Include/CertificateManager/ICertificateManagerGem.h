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

#include "IGem.h"

namespace CertificateManager
{
    class IDataSource;

    //CRYINTERFACE_DECLARE(ICertificateManagerGem, 0x37a4537a67f04608, 0xf8ab133065034f52);

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
