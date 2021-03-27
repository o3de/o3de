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
    testdata/config_broken_badplatform/AssetProcessorPlatformConfig.ini
    testdata/config_broken_noplatform/AssetProcessorPlatformConfig.ini
    testdata/config_broken_noscans/AssetProcessorPlatformConfig.ini
    testdata/config_broken_recognizers/AssetProcessorPlatformConfig.ini
    testdata/config_regular/AssetProcessorPlatformConfig.ini
    testdata/config_regular_platform_scanfolder/AssetProcessorPlatformConfig.ini
    testdata/EmptyDummyProject/AssetProcessorGamePlatformConfig.ini
    testdata/DummyProject/AssetProcessorGamePlatformConfig.ini
    native/tests/AssetProcessorTest.h
    native/tests/AssetProcessorTest.cpp
    native/tests/BaseAssetProcessorTest.h
    native/tests/assetdatabase/AssetDatabaseTest.cpp
    native/tests/resourcecompiler/RCBuilderTest.cpp
    native/tests/resourcecompiler/RCBuilderTest.h
    native/tests/resourcecompiler/RCControllerTest.cpp
    native/tests/resourcecompiler/RCControllerTest.h
    native/tests/resourcecompiler/RCJobTest.cpp
    native/tests/resourcecompiler/RCJobTest.h
    native/tests/assetBuilderSDK/assetBuilderSDKTest.h
    native/tests/assetBuilderSDK/assetBuilderSDKTest.cpp
    native/tests/assetBuilderSDK/SerializationDependenciesTests.cpp
    native/tests/assetmanager/AssetProcessorManagerTest.cpp
    native/tests/assetmanager/AssetProcessorManagerTest.h
    native/tests/utilities/assetUtilsTest.cpp
    native/tests/platformconfiguration/platformconfigurationtests.cpp
    native/tests/platformconfiguration/platformconfigurationtests.h
    native/tests/utilities/JobModelTest.cpp
    native/tests/utilities/JobModelTest.h
    native/tests/AssetCatalog/AssetCatalogUnitTests.cpp
    native/tests/assetscanner/AssetScannerTests.h
    native/tests/assetscanner/AssetScannerTests.cpp
    native/tests/BuilderConfiguration/BuilderConfigurationTests.cpp
    native/tests/FileProcessor/FileProcessorTests.h
    native/tests/FileProcessor/FileProcessorTests.cpp
    native/tests/FileStateCache/FileStateCacheTests.h
    native/tests/FileStateCache/FileStateCacheTests.cpp
    native/tests/InternalBuilders/SettingsRegistryBuilderTests.cpp
    native/tests/MissingDependencyScannerTests.cpp
    native/tests/SourceFileRelocatorTests.cpp
    native/tests/AssetProcessorMessagesTests.cpp
    native/unittests/AssetProcessingStateDataUnitTests.cpp
    native/unittests/AssetProcessingStateDataUnitTests.h
    native/unittests/AssetProcessorManagerUnitTests.cpp
    native/unittests/AssetProcessorManagerUnitTests.h
    native/unittests/AssetProcessorServerUnitTests.cpp
    native/unittests/AssetProcessorServerUnitTests.h
    native/unittests/AssetScannerUnitTests.cpp
    native/unittests/AssetScannerUnitTests.h
    native/unittests/ConnectionUnitTests.cpp
    native/unittests/ConnectionUnitTests.h
    native/unittests/ConnectionManagerUnitTests.cpp
    native/unittests/ConnectionManagerUnitTests.h
    native/unittests/FileWatcherUnitTests.cpp
    native/unittests/FileWatcherUnitTests.h
    native/unittests/PlatformConfigurationUnitTests.cpp
    native/unittests/PlatformConfigurationUnitTests.h
    native/unittests/RCcontrollerUnitTests.cpp
    native/unittests/RCcontrollerUnitTests.h
    native/unittests/ShaderCompilerUnitTests.cpp
    native/unittests/ShaderCompilerUnitTests.h
    native/unittests/UnitTestRunner.cpp
    native/unittests/UnitTestRunner.h
    native/unittests/UtilitiesUnitTests.cpp
    native/unittests/UtilitiesUnitTests.h
    native/unittests/AssetRequestHandlerUnitTests.cpp
    native/unittests/AssetRequestHandlerUnitTests.h
    native/unittests/MockConnectionHandler.h
    native/unittests/MockApplicationManager.cpp
    native/unittests/MockApplicationManager.h
    native/unittests/BuilderSDKUnitTests.cpp
    native/utilities/UnitTestShaderCompilerServer.cpp
    native/utilities/UnitTestShaderCompilerServer.h
    native/tests/test_main.cpp
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    native/tests/utilities/JobModelTest.cpp
)
