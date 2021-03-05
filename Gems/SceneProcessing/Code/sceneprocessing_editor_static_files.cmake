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
    Include/Config/SceneProcessingConfigBus.h
    Source/SceneProcessingModule.h
    Source/Config/Components/SceneProcessingConfigSystemComponent.h
    Source/Config/Components/SceneProcessingConfigSystemComponent.cpp
    Source/Config/Components/SoftNameBehavior.h
    Source/Config/Components/SoftNameBehavior.cpp
    Source/Exporting/Components/TangentGenerateComponent.h
    Source/Exporting/Components/TangentGenerateComponent.cpp
    Source/Exporting/Components/TangentPreExportComponent.h
    Source/Exporting/Components/TangentPreExportComponent.cpp
    Source/Exporting/Components/TangentGenerators/MikkTGenerator.h
    Source/Exporting/Components/TangentGenerators/MikkTGenerator.cpp
    Source/Config/SettingsObjects/SoftNameSetting.h
    Source/Config/SettingsObjects/SoftNameSetting.cpp
    Source/Config/SettingsObjects/NodeSoftNameSetting.h
    Source/Config/SettingsObjects/NodeSoftNameSetting.cpp
    Source/Config/SettingsObjects/FileSoftNameSetting.h
    Source/Config/SettingsObjects/FileSoftNameSetting.cpp
    Source/Config/Widgets/GraphTypeSelector.h
    Source/Config/Widgets/GraphTypeSelector.cpp
    Source/SceneBuilder/SceneBuilderComponent.h
    Source/SceneBuilder/SceneBuilderComponent.cpp
    Source/SceneBuilder/SceneBuilderWorker.h
    Source/SceneBuilder/SceneBuilderWorker.cpp
    Source/SceneBuilder/SceneSerializationHandler.h
    Source/SceneBuilder/SceneSerializationHandler.cpp
    Source/SceneBuilder/TraceMessageHook.h
    Source/SceneBuilder/TraceMessageHook.cpp
)
