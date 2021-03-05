#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
