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
    Tests/RHITestFixture.h
    Tests/AllocatorTests.cpp
    Tests/BufferTests.cpp
    Tests/DrawPacketTests.cpp
    Tests/FrameGraphTests.cpp
    Tests/FrameSchedulerTests.cpp
    Tests/HashingTests.cpp
    Tests/ImageTests.cpp
    Tests/IndirectBufferTests.cpp
    Tests/InputStreamLayoutBuilderTests.cpp
    Tests/NameIdReflectionMapTests.cpp
    Tests/PipelineStateTests.cpp
    Tests/QueryTests.cpp
    Tests/RenderAttachmentLayoutBuilderTests.cpp
    Tests/ShaderResourceGroupTests.cpp
    Tests/UtilsTests.cpp
    Tests/Buffer.h
    Tests/Buffer.cpp
    Tests/Device.h
    Tests/Device.cpp
    Tests/Image.h
    Tests/Image.cpp
    Tests/Factory.h
    Tests/Factory.cpp
    Tests/FrameGraph.h
    Tests/FrameGraph.cpp
    Tests/IndirectBuffer.h
    Tests/IndirectBuffer.cpp
    Tests/PipelineState.h
    Tests/PipelineState.cpp
    Tests/Query.h
    Tests/Query.cpp
    Tests/Scope.h
    Tests/Scope.cpp
    Tests/ShaderResourceGroup.h
    Tests/ShaderResourceGroup.cpp
    Tests/TransientAttachmentPool.h
    Tests/TransientAttachmentPool.cpp
    Tests/ThreadTester.h
    Tests/ThreadTester.cpp
    Tests/ImagePropertyTests.cpp
    Tests/BufferPropertyTests.cpp
    Tests/IntervalMapTests.cpp
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    Tests/Buffer.h
    Tests/Buffer.cpp
)
