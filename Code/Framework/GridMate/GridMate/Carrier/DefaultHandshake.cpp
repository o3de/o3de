/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/DefaultHandshake.h>
#include <GridMate/Serialize/UtilityMarshal.h>

using namespace GridMate;

//=========================================================================
// DefaultHandshake
// [11/5/2010]
//=========================================================================
DefaultHandshake::DefaultHandshake(unsigned int timeOut, VersionType version)
{
    m_handshakeTimeOutMS = timeOut;
    m_version = version;
}

//=========================================================================
// ~DefaultHandshake
// [11/5/2010]
//=========================================================================
DefaultHandshake::~DefaultHandshake()
{
}

//=========================================================================
// GetWelcomeData
// [11/5/2010]
//=========================================================================
void
DefaultHandshake::OnInitiate(ConnectionID id, WriteBuffer& wb)
{
    (void)id;
    wb.Write(m_version);
}

//=========================================================================
// OnReceiveRequest
// [11/5/2010]
//=========================================================================
HandshakeErrorCode
DefaultHandshake::OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb)
{
    (void)id;
    OnInitiate(id, wb); // send back the version string

    VersionType version;
    if (rb.Read(version) && version == m_version)
    {
        return HandshakeErrorCode::OK;
    }
    else
    {
        return HandshakeErrorCode::VERSION_MISMATCH;
    }
}

//=========================================================================
// OnConfirmRequest
// [2/10/2011]
//=========================================================================
bool
DefaultHandshake::OnConfirmRequest(ConnectionID id, ReadBuffer& rb)
{
    return OnReceiveAck(id, rb);
}

//=========================================================================
// OnReceiveAck
// [2/2/2011]
//=========================================================================
bool
DefaultHandshake::OnReceiveAck(ConnectionID id, ReadBuffer& rb)
{
    (void)id;
    (void)rb;
    return true;
}

//=========================================================================
// OnConfirmAck
// [2/10/2011]
//=========================================================================
bool
DefaultHandshake::OnConfirmAck(ConnectionID id, ReadBuffer& rb)
{
    return OnReceiveAck(id, rb);
}

//=========================================================================
// OnNewConnection
// [11/5/2010]
//=========================================================================
bool
DefaultHandshake::OnNewConnection(const string& address)
{
    (void)address;
    return true; /// We don't have a ban list yet
}

//=========================================================================
// OnDisconnect
// [11/5/2010]
//=========================================================================
void
DefaultHandshake::OnDisconnect(ConnectionID id)
{
    (void)id;
}
