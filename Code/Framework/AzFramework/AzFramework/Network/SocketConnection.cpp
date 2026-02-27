/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Environment.h>
#include <AzFramework/Network/SocketConnection.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZF_API, AzFramework::EngineConnectionEvents);

namespace AzFramework
{
    static AZ::EnvironmentVariable<SocketConnection*> g_socketConnectionInstance;
    static AZ::EnvironmentVariable<SocketConnection::KeepAliveCallback> g_keepAliveCallbackInstance;
    static const char* s_SocketConnectionName = "SocketConnection";
    static const char* s_KeepAliveVariableName = "SocketConnectionKeepAliveCallback";

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

    SocketConnection::KeepAliveCallback SocketConnection::GetKeepAliveCallback()
    {
        if (!g_keepAliveCallbackInstance)
        {
            g_keepAliveCallbackInstance = AZ::Environment::FindVariable<SocketConnection::KeepAliveCallback>(s_KeepAliveVariableName);
        }

        return g_keepAliveCallbackInstance ? (*g_keepAliveCallbackInstance) : nullptr;
    }

    void SocketConnection::SetKeepAliveCallback(SocketConnection::KeepAliveCallback callbackFn)
    {
        if (!g_keepAliveCallbackInstance)
        {
            g_keepAliveCallbackInstance = AZ::Environment::CreateVariable<SocketConnection::KeepAliveCallback>(s_KeepAliveVariableName);
            (*g_keepAliveCallbackInstance) = nullptr;
        }

        if ((callbackFn) && (g_keepAliveCallbackInstance) && (*g_keepAliveCallbackInstance))
        {
            AZ_Error(
                "SocketConnection",
                false,
                "SocketConnection::SetKeepAliveCallback was called without first destroying the old instance and setting it to nullptr");
        }

        (*g_keepAliveCallbackInstance) = callbackFn;
    }
} // namespace AzFramework
