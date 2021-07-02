/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Metastream_precompiled.h"
#include "CivetHttpServer.h"
#include <AzCore/base.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/std/containers/vector.h>

#include <sstream>

static const AZ::u16 kMetastreamDefaultServerPort = 8082;

using namespace Metastream;

class CivetHttpHandler : public CivetHandler
{
public:
    CivetHttpHandler(const CivetHttpServer* parent)
        : m_parent(parent)
    {
    }

    bool handleGet([[maybe_unused]] CivetServer* server, struct mg_connection* conn)
    {
        const mg_request_info* request = mg_get_request_info(conn);

        std::map<std::string, std::string> filters;
        if (request->query_string != nullptr)
        {
            filters = BaseHttpServer::TokenizeQuery(request->query_string);
        }
                
        HttpResponse response;

        auto table = filters.find("table");
        if (table != filters.end())
        {
            auto key = filters.find("key");
            if (key != filters.end())
            {
                std::vector<std::string> keyList = BaseHttpServer::SplitValueList(key->second, ',');
                response = m_parent->GetDataValues(table->second, keyList);
            }
            else
            {
                response = m_parent->GetDataKeys(table->second);
            }
        }
        else
        {
            response = m_parent->GetDataTables();
        }

        mg_printf(conn, BaseHttpServer::HttpStatus(response.code).c_str());
        mg_printf(conn, BaseHttpServer::SerializeHeaders(response.headers).c_str());
        mg_printf(conn, response.body.c_str());
        return true;
    }

private:
    const CivetHttpServer* m_parent;
};

class CivetWSHandler : public CivetWebSocketHandler
{
public:
    CivetWSHandler(const CivetHttpServer* parent)
        : m_parent(parent)
    {
    }

    bool handleConnection([[maybe_unused]] CivetServer *server, [[maybe_unused]] const struct mg_connection *conn) override
    {
        return true;
    }

    void handleReadyState([[maybe_unused]] CivetServer *server, [[maybe_unused]] struct mg_connection *conn) override
    {
    }

    bool handleData([[maybe_unused]] CivetServer *server, struct mg_connection *conn, int bits, char *data, size_t data_len) override
    {
        // RFC for websockets: https://tools.ietf.org/html/rfc6455
        // bits represents the websocket frame flags

        // we check if this is the final fragment (FIN)
        if (bits & 0x80) {
            bits &= 0x7f; // extract only the opcode
            switch (bits) {
            case WEBSOCKET_OPCODE_CONTINUATION:
                break;
            case WEBSOCKET_OPCODE_TEXT:
            {
                std::map<std::string, std::string> filters;
                if (data != nullptr)
                {
                    filters = BaseHttpServer::TokenizeQuery(std::string(data, data_len).c_str());
                }

                HttpResponse response;

                auto table = filters.find("table");
                if (table != filters.end())
                {
                    auto key = filters.find("key");
                    if (key != filters.end())
                    {
                        std::vector<std::string> keyList = BaseHttpServer::SplitValueList(key->second, ',');
                        response = m_parent->GetDataValues(table->second, keyList);
                    }
                    else
                    {
                        response = m_parent->GetDataKeys(table->second);
                    }
                }
                else
                {
                    response = m_parent->GetDataTables();
                }
                std::string payload(response.body);
                mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, payload.c_str(), payload.size() + 1);
                break;
            }
            case WEBSOCKET_OPCODE_BINARY:
                break;
            case WEBSOCKET_OPCODE_CONNECTION_CLOSE:
                /* If client initiated close, respond with close message in acknowledgment */
                mg_websocket_write(conn, WEBSOCKET_OPCODE_CONNECTION_CLOSE, "", 0);
                return 0; /* time to close the connection */
                break;
            case WEBSOCKET_OPCODE_PING:
                /* client sent PING, respond with PONG */
                mg_websocket_write(conn, WEBSOCKET_OPCODE_PONG, "", 0);
                break;
            case WEBSOCKET_OPCODE_PONG:
                /* received PONG to our PING, no action */
                break;
            default:
                AZ_Error("Metastream", false, "Unknown flags: %02x\n", bits);
                break;
            }
        }

        return true;
    }

    virtual void handleClose([[maybe_unused]] CivetServer *server, [[maybe_unused]] const struct mg_connection *conn) override
    {
    }

private:
    const CivetHttpServer* m_parent;
};

CivetHttpServer::CivetHttpServer(const DataCache* cache) :
    BaseHttpServer(cache),
    m_server(nullptr)
{
    m_handler = new CivetHttpHandler(this);
    m_webSocketHandler = new CivetWSHandler(this);
}

CivetHttpServer::~CivetHttpServer()
{
    Stop();
    delete m_handler;
    delete m_webSocketHandler;
}

bool CivetHttpServer::Start(const std::string& civetOptions)
{
    // default options
    std::vector<std::string> options{ "enable_directory_listing", "no" };

    // ignore options
    std::vector<std::string> ignoreOptions{ "enable_directory_listing", "cgi_interpreter", "run_as_user", "put_delete_auth_file" };
        
    AZStd::vector<std::string> parsedOptions;
    AZStd::tokenize<std::string>(civetOptions, ";", parsedOptions);

    for (const auto & i : parsedOptions)
    {
        AZStd::vector<std::string> kvp;
        AZStd::tokenize<std::string>(i, "=", kvp);

        if (kvp.size() == 2)    // there must be both a key and a value. (Key=offset 0, Value=1)
        {
            // make sure its not an ignored option
            if ( std::find(ignoreOptions.cbegin(), ignoreOptions.cend(), kvp[0]) == ignoreOptions.cend() )
            {
                // replace escape sequences with correct characters
                std::string realOption(StrReplace(kvp[1], "$semi", ";"));
                realOption = StrReplace(realOption, "$equ", "=");

                options.push_back(kvp[0]);
                options.push_back( realOption );
            }
        }
    }

    // check to see if "listening_ports" is present, if not set to default value.
    if (std::find(options.cbegin(), options.cend(), "listening_ports") == options.cend())
    {
        options.push_back("listening_ports");
        options.push_back(std::to_string(kMetastreamDefaultServerPort));
    }

    // Note: the 3rd party software, Civetweb, uses exceptions.
    // Using try/catch to handle failure gracefully without having to modify Civetweb.
    try
    {
        // Create and start the server
        m_server = new CivetServer(options);
    }
    catch (CivetException)
    {
        // Failed to create/start server
        return false;
    }
    

    // Add a handler for all requests
    m_server->addHandler("/data", m_handler);
    m_server->addWebSocketHandler("/ws", m_webSocketHandler);

    return true;
}

void CivetHttpServer::Stop()
{
    if (m_server)
    {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

