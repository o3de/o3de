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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AWSMetrics
{
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
