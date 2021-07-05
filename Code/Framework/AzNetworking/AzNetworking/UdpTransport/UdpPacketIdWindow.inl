/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline UdpPacketIdWindow::~UdpPacketIdWindow()
    {
        Reset();
    }

    inline SequenceId UdpPacketIdWindow::GetHeadSequenceId() const
    {
        return m_headSequenceId;
    }

    inline SequenceRolloverCount UdpPacketIdWindow::GetSequenceRolloverCount() const
    {
        return m_sequenceRolloverCount;
    }

    inline const UdpPacketIdWindow::PacketAckContainer& UdpPacketIdWindow::GetPacketAckContainer() const
    {
        return m_ackWindow;
    }
}
