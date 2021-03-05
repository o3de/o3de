/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include <CryCommon/ISystem.h>

namespace AZ
{
    /*!
     * Signal LY to create an ICryFont
     */
    class CryFontCreationRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual bool CreateCryFont([[maybe_unused]] SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams) {return false;} //! return false to fall back to default CryFont initialization
    };
    using CryFontCreationRequestBus = AZ::EBus<CryFontCreationRequests>;

}
