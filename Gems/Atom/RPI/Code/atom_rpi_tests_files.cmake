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
    Tests/Buffer/BufferTests.cpp
    Tests/Common/AssetManagerTestFixture.cpp
    Tests/Common/AssetManagerTestFixture.h
    Tests/Common/AssetSystemStub.cpp
    Tests/Common/AssetSystemStub.h
    Tests/Common/ErrorMessageFinder.cpp
    Tests/Common/ErrorMessageFinder.h
    Tests/Common/ErrorMessageFinderTests.cpp
    Tests/Common/JsonTestUtils.cpp
    Tests/Common/JsonTestUtils.h
    Tests/Common/RPITestFixture.cpp
    Tests/Common/RPITestFixture.h
    Tests/Common/SerializeTester.h
    Tests/Common/TestUtils.h
    Tests/Common/TestFeatureProcessors.h
    Tests/Common/RHI/Factory.cpp
    Tests/Common/RHI/Factory.h
    Tests/Common/RHI/Stubs.cpp
    Tests/Common/RHI/Stubs.h
    Tests/Image/StreamingImageTests.cpp
    Tests/Material/LuaMaterialFunctorTests.cpp
    Tests/Material/MaterialTypeAssetTests.cpp
    Tests/Material/MaterialTypeSourceDataTests.cpp
    Tests/Material/MaterialAssetTestUtils.cpp
    Tests/Material/MaterialAssetTestUtils.h
    Tests/Material/MaterialPropertySerializerTests.cpp
    Tests/Material/MaterialAssetTests.cpp
    Tests/Material/MaterialSourceDataTests.cpp
    Tests/Material/MaterialFunctorTests.cpp
    Tests/Material/MaterialFunctorSourceDataSerializerTests.cpp
    Tests/Material/MaterialPropertyValueSourceDataTests.cpp
    Tests/Material/MaterialTests.cpp
    Tests/Model/ModelTests.cpp
    Tests/Pass/PassTests.cpp
    Tests/Shader/ShaderTests.cpp
    Tests/ShaderResourceGroup/ShaderResourceGroupAssetTests.cpp
    Tests/ShaderResourceGroup/ShaderResourceGroupBufferTests.cpp
    Tests/ShaderResourceGroup/ShaderResourceGroupConstantBufferTests.cpp
    Tests/ShaderResourceGroup/ShaderResourceGroupImageTests.cpp
    Tests/ShaderResourceGroup/ShaderResourceGroupGeneralTests.cpp
    Tests/System/FeatureProcessorFactoryTests.cpp
    Tests/System/GpuQueryTests.cpp
    Tests/System/RenderPipelineTests.cpp
    Tests/System/SceneTests.cpp
    Tests/System/ViewTests.cpp
)
