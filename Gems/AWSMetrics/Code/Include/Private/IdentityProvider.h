/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AWSMetrics
{
    constexpr const char* EngineVersionJsonKey = "O3DEVersion";

    //! Base class to be implemented by IdentityProvider to retrive an ID for identity.
    class IdentityProvider
    {
    public:
        IdentityProvider() = default;
        virtual ~IdentityProvider() = default;

        //! Retrieve the ID for identity.
        //! @return ID for identity.
        virtual AZStd::string GetIdentifier() const = 0;

        //! Factory method for creating the concrete identify provider.
        //! @return Pointer to the concrete identity provider.
        static AZStd::unique_ptr<IdentityProvider> CreateIdentityProvider();

    private:
        static AZStd::string GetEngineVersion();
    };
}
