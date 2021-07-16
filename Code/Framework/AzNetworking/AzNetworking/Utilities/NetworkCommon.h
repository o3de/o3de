/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>
#include <AzNetworking/ConnectionLayer/SequenceGenerator.h>

namespace AzNetworking
{
    AZ_TYPE_SAFE_INTEGRAL(SequenceRolloverCount, uint16_t);
    static constexpr SequenceRolloverCount InvalidSequenceRolloverCount = SequenceRolloverCount{uint16_t(0xFFFF)};

    AZ_TYPE_SAFE_INTEGRAL(PacketId, uint32_t);
    static constexpr PacketId InvalidPacketId = PacketId{0xFFFFFFFF};

    //! Helper type to contain socket file descriptors.
    AZ_TYPE_SAFE_INTEGRAL(SocketFd, int32_t);
    static constexpr SocketFd InvalidSocketFd = SocketFd{-1};

    //! Constants for wrapping os specific socket call results.
    static const int32_t SocketOpResultSuccess      =  0;
    static const int32_t SocketOpResultError        = -1;
    static const int32_t SocketOpResultDisconnected = -2;
    static const int32_t SocketOpResultErrorNotOpen = -3;
    static const int32_t SocketOpResultErrorNoSsl   = -4;

    //! Returns a valid disconnect reason if the provided socket result requires a disconnect.
    DisconnectReason GetDisconnectReasonForSocketResult(int32_t socketResult);

    //! Helper method that creates an PacketId from a SequenceRolloverCount and SequenceId.
    //! @param rolloverCount the number of rollovers detected for this sequenced variable
    //! @param sequenceId    sequence value to build the PacketId with
    //! @return PacketId
    PacketId MakePacketId(SequenceRolloverCount rolloverCount, SequenceId sequenceId);

    //! Helper method that creates a SequenceId from an PacketId.
    //! @param packetId packet id used in the conversion process
    //! @return SequenceId
    SequenceId ToSequenceId(PacketId packetId);

    //! Helper method that extracts the rollover count from an PacketId.
    //! @param packetId packet id used in the conversion process
    //! @return SequenceRolloverCount
    SequenceRolloverCount ToRolloverCount(PacketId packetId);

    //! Initializes the network layer, required on some platforms.
    //! Should be called once at program initialization, prior to performing any network operations
    //! @return boolean true on success
    bool SocketLayerInit();

    //! Shuts down the network layer, required on some platforms.
    //! Should be called once at program halt, after stopping all network resources
    //! @return boolean true on success
    bool SocketLayerShutdown();

    //! Sets appropriate socket options to make the input socket non-blocking.
    //! @param socketFd identifier of socket to set to non-blocking mode
    //! @return boolean true on success
    bool SetSocketNonBlocking(SocketFd socketFd);

    //! Disables Tcp flow control for the provided socket.
    //! @param socketFd identifier of the socket to disable flow control for
    //! @return boolean true on success
    bool SetSocketNoDelay(SocketFd socketFd);

    //! Changes network socket receive buffer size.
    //! @param socketFd identifier of the socket to change the receive buffer size of
    //! @param sendSize requested send buffer size
    //! @param recvSize requested receive buffer size
    //! @return boolean true on success
    bool SetSocketBufferSizes(SocketFd socketFd, int32_t sendSize, int32_t recvSize);

    //! Closes the provided socket.
    //! @param socketFd identifier of socket to close
    void CloseSocket(SocketFd socketFd);

    //! Returns the global error code from the last performed network operation, value is platform specific.
    //! @return platform specific error result for the last performed network operation
    int32_t GetLastNetworkError();

    //! Returns true if the platform specific error code maps to a 'would block' error.
    //! @return true if the platform specific error code maps to a 'would block' error
    bool ErrorIsWouldBlock(int32_t errorCode);

    //! Returns true if the platform specific error code maps to a 'forcibly closed' error.
    //! @param ignoreError out value to indicate if the error should be ignored
    //! @return true if the platform specific error code maps to a 'forcibly closed' error
    bool ErrorIsForciblyClosed(int32_t errorCode, bool& ignoreError);

    //! Returns a string description for the provided platform specific network error code.
    //! @param errorCode platform specific error code to return the string result for
    //! @return string description for the provided error code
    const char *GetNetworkErrorDesc(int32_t errorCode);

    //! Generates a string label suitable for container, doesn't allocate or use format strings.
    //! @param value the integral index to generate a string label for
    //! @return string label for the provided index
    template <AZStd::size_t MAX_VALUE>
    constexpr auto GenerateIndexLabel(AZStd::size_t value);
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AzNetworking::SequenceRolloverCount);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AzNetworking::PacketId);
AZ_TYPE_SAFE_INTEGRAL_CVARBINDING(AZ::TimeMs);

#include <AzNetworking/Utilities/NetworkCommon.inl>
