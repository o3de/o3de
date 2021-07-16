/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IdentityProvider.h>

namespace AWSMetrics
{
    //! Implements the default Client ID provider to create a default identifier for each client.
    class DefaultClientIdProvider
        : public IdentityProvider
    {
    public:
        DefaultClientIdProvider(const AZStd::string& engineVersion = "");
        DefaultClientIdProvider() = delete;
        ~DefaultClientIdProvider() = default;

        // IdentityProviderInterface overrides
        AZStd::string GetIdentifier() const override;

    private:
        AZStd::string m_clientId;
    };
}
