#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Mocks/MockBehaviorUtils.h
    Mocks/Containers/MockScene.h
    Mocks/DataTypes/GraphData/MockIMeshData.h
    Mocks/DataTypes/MockIGraphObject.h
    Mocks/DataTypes/Groups/MockIGroup.h
    Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h
    Mocks/Events/MockAssetImportRequest.h
    Tests/TestsMain.cpp
    Tests/Containers/SceneTests.cpp
    Tests/Containers/SceneBehaviorTests.cpp
    Tests/Containers/SceneGraphTests.cpp
    Tests/Containers/SceneManifestTests.cpp
    Tests/Events/AssetImporterRequestTests.cpp
    Tests/Containers/Views/IteratorTestsBase.h
    Tests/Containers/Views/IteratorConformityTests.cpp
    Tests/Containers/Views/ConvertIteratorTests.cpp
    Tests/Containers/Views/FilterIteratorTests.cpp
    Tests/Containers/Views/PairIteratorTests.cpp
    Tests/Containers/Views/SceneGraphUpwardsIteratorTests.cpp
    Tests/Containers/Views/SceneGraphChildIteratorTests.cpp
    Tests/Containers/Views/SceneGraphDownwardsIteratorTests.cpp
    Tests/Containers/Utilities/FiltersTests.cpp
    Tests/Utilities/SceneGraphSelectorTests.cpp
    Tests/Utilities/PatternMatcherTests.cpp
    Tests/Utilities/CoordinateSystemConverterTests.cpp
)
