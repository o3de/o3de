/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
namespace BarrierInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Barrier client that manages a connection with a Barrier server.
    class BarrierClient
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        static constexpr AZ::u32 DEFAULT_BARRIER_CONNECTION_PORT_NUMBER = 24800;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(BarrierClient, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] clientScreenName Name of the Barrier client screen this class implements
        //! \param[in] serverHostName Name of the Barrier server host this client connects to
        //! \param[in] connectionPort Port number over which to connect to the Barrier server
        BarrierClient(const char* clientScreenName,
                      const char* serverHostName,
                      AZ::u32 connectionPort = DEFAULT_BARRIER_CONNECTION_PORT_NUMBER);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~BarrierClient();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the Barrier client screen this class implements
        //! \return Name of the Barrier client screen this class implements
        const AZStd::string& GetClientScreenName() const { return m_clientScreenName; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the Barrier server host this client connects to
        //! \return Name of the Barrier server host this client connects to
        const AZStd::string& GetServerHostName() const { return m_serverHostName; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the socket the Barrier client is communicating over
        //! \return The socket the Barrier client is communicating over
        const AZSOCKET& GetSocket() const { return m_socket; }

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The client connection loop that runs in it's own thread
        void Run();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Try to connect to the Barrier server
        //! \return True if we're connected to the Barrier server, false otherwise
        bool ConnectToServer();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::string       m_clientScreenName;
        AZStd::string       m_serverHostName;
        AZ::u32             m_connectionPort;
        AZStd::thread       m_threadHandle;
        AZStd::atomic_bool  m_threadQuit;
        AZSOCKET            m_socket;
    };
} // namespace BarrierInput
