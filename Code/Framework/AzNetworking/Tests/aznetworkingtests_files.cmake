#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Main.cpp
    ConnectionLayer/ConnectionMetricsTests.cpp
    ConnectionLayer/SequenceGeneratorTests.cpp
    DataStructures/FixedSizeBitsetTests.cpp
    DataStructures/FixedSizeBitsetViewTests.cpp
    DataStructures/FixedSizeVectorBitsetTests.cpp
    DataStructures/RingBufferBitsetTests.cpp
    DataStructures/TimeoutQueueTests.cpp
    Serialization/DeltaSerializerTests.cpp
    Serialization/HashSerializerTests.cpp
    Serialization/NetworkInputSerializerTests.cpp
    Serialization/NetworkOutputSerializerTests.cpp
    Serialization/TrackChangedSerializerTests.cpp
    TcpTransport/TcpTransportTests.cpp
    UdpTransport/UdpTransportTests.cpp
    Utilities/CidrAddressTests.cpp
    Utilities/IpAddressTests.cpp
    Utilities/NetworkCommonTests.cpp
    Utilities/QuantizedValuesTests.cpp
)
