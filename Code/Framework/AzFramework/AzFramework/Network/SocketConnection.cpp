/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Environment.h>
#include <AzFramework/Network/SocketConnection.h>

namespace AzFramework
{
    static AZ::EnvironmentVariable<SocketConnection*> g_socketConnectionInstance;
    static const char* s_SocketConnectionName = "SocketConnection";

    SocketConnection* SocketConnection::GetInstance()
    {
        if (!g_socketConnectionInstance)
        {
            g_socketConnectionInstance = AZ::Environment::FindVariable<SocketConnection*>(s_SocketConnectionName);
        }

        return g_socketConnectionInstance ? (*g_socketConnectionInstance) : nullptr;
    }

    void SocketConnection::SetInstance(SocketConnection* instance)
    {
        if (!g_socketConnectionInstance)
        {
            g_socketConnectionInstance = AZ::Environment::CreateVariable<SocketConnection*>(s_SocketConnectionName);
            (*g_socketConnectionInstance) = nullptr;
        }

        if ((instance) && (g_socketConnectionInstance) && (*g_socketConnectionInstance))
        {
            AZ_Error("SocketConnection", false, "SocketConnection::SetInstance was called without first destroying the old instance and setting it to nullptr");
        }

        (*g_socketConnectionInstance) = instance;
    }
} // namespace AzFramework
