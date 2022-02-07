#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(SceneAPI) # Needs to go before AssetProcessor since it provides a builder
add_subdirectory(AssetProcessor)
add_subdirectory(AWSNativeSDKInit)
add_subdirectory(AzTestRunner)
add_subdirectory(CrashHandler)
add_subdirectory(PythonBindingsExample)
add_subdirectory(RemoteConsole)
add_subdirectory(DeltaCataloger)
add_subdirectory(SerializeContextTools)
add_subdirectory(AssetBundler)
add_subdirectory(GridHub)
add_subdirectory(Standalone)
add_subdirectory(TestImpactFramework)
add_subdirectory(ProjectManager)
add_subdirectory(BundleLauncher)
