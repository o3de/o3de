/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Socket/AzSocket_fwd.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Synergy client that manages a connection with a Synergy server.
    class SynergyClient
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SynergyClient, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] clientScreenName Name of the Synergy client screen this class implements
        //! \param[in] serverHostName Name of the Synergy server host this client connects to
        SynergyClient(const char* clientScreenName, const char* serverHostName);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SynergyClient();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the synergy client screen this class implements
        //! \return Name of the synergy client screen this class implements
        const AZStd::string& GetClientScreenName() const { return m_clientScreenName; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the synergy server host this client connects to
        //! \return Name of the synergy server host this client connects to
        const AZStd::string& GetServerHostName() const { return m_serverHostName; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the socket the Synergy client is communicating over
        //! \return The socket the Synergy client is communicating over
        const AZSOCKET& GetSocket() const { return m_socket; }

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The client connection loop that runs in it's own thread
        void Run();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Try to connect to the Synergy server
        //! \return True if we're connected to the Synergy server, false otherwise
        bool ConnectToServer();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::string       m_clientScreenName;
        AZStd::string       m_serverHostName;
        AZStd::thread       m_threadHandle;
        AZStd::atomic_bool  m_threadQuit;
        AZSOCKET            m_socket;
    };
} // namespace SynergyInput
