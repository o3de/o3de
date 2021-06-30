/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Authentication/AuthenticationProviderInterface.h>

namespace AWSClientAuth
{  
    AuthenticationTokens AuthenticationProviderInterface::GetAuthenticationTokens()
    {
        return m_authenticationTokens;
    }

    void AuthenticationProviderInterface::SignOut()
    {
        m_authenticationTokens = AuthenticationTokens();
    }

   
} // namespace AWSClientAuth
