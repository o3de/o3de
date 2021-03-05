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
    Mocks/Containers/MockScene.h
    Mocks/DataTypes/GraphData/MockIMeshData.h
    Mocks/DataTypes/MockIGraphObject.h
    Mocks/DataTypes/Groups/MockIGroup.h
    Mocks/DataTypes/Groups/MockIMeshGroup.h
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
    Tests/Export/MaterialIOTests.cpp
)
