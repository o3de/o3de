/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SESSION_LANSESSIONSERVICEUTILS_H
#define GM_SESSION_LANSESSIONSERVICEUTILS_H

namespace GridMate
{
    /**
    * Session params for default PC implementation.
    */
    struct LANSessionParams
        : public SessionParams
    {
        LANSessionParams()
            : m_port (0) // by default session can't be found (searched for)
        {}

        string m_address; /// empty to accept any address otherwise you can provide a specific bind address.
        /**
         * Use 0 if you don't want you session to be searchable (default)
         * Port on which we will register the LAN session (it will be used for session communication and should be different than the game/carrier one).
         */
        int m_port;
    };

    struct LANSearchParams
        : public SearchParams
    {
        LANSearchParams()
            : m_familyType(0)
            , m_serverPort(-1)
            , m_listenPort(0)
            , m_broadcastFrequencyMs(1000)
        {}

        int     m_familyType;                   ///< Socket driver specific, by default 0.
        string  m_serverAddress;                ///< Address of the server, we empty we create a broadcast address.
        int     m_serverPort;                   ///< Server port (must be provided).
        string  m_listenAddress;                ///< Address to bind for listening. By default is empty, which means we are listening to any address.
        int     m_listenPort;                   ///< Search listen port, if not set we will use ephimeral port.
        unsigned int m_broadcastFrequencyMs;    ///< Time in MS between search broadcasts.
    };

    /*
    * Used to return search results.
    */
    struct LANSearchInfo
        : public SearchInfo
    {
        string m_serverIP; ///< server ID as we see it
        AZ::u16 m_serverPort; ///< server port for the session
    };
}

#endif
