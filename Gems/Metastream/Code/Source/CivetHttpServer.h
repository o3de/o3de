/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "BaseHttpServer.h"
#include "CivetServer.h"

namespace Metastream
{
    class CivetHttpServer : public BaseHttpServer
    {
    public:
        CivetHttpServer(const DataCache* cache);
        virtual ~CivetHttpServer();

        virtual bool Start(const std::string& civetOptions) override;
        virtual void Stop() override;
    private:
        CivetHandler* m_handler;
        CivetWebSocketHandler* m_webSocketHandler;
        CivetServer* m_server;
    };
} // namespace Metastream
