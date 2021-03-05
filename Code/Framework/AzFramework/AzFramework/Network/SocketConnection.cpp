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
