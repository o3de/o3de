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
    Include/AtomToolsFramework/DynamicProperty/DynamicProperty.h
    Include/AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h
    Include/AtomToolsFramework/Inspector/InspectorWidget.h
    Include/AtomToolsFramework/Inspector/InspectorRequestBus.h
    Include/AtomToolsFramework/Inspector/InspectorNotificationBus.h
    Include/AtomToolsFramework/Inspector/InspectorGroupWidget.h
    Include/AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h
    Include/AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h
    Include/AtomToolsFramework/Util/MaterialPropertyUtil.h
    Include/AtomToolsFramework/Util/Util.h
    Include/AtomToolsFramework/Viewport/RenderViewportWidget.h
    Source/DynamicProperty/DynamicProperty.cpp
    Source/DynamicProperty/DynamicPropertyGroup.cpp
    Source/Inspector/InspectorWidget.cpp
    Source/Inspector/InspectorWidget.ui
    Source/Inspector/InspectorWidget.qrc
    Source/Inspector/InspectorGroupWidget.cpp
    Source/Inspector/InspectorGroupHeaderWidget.cpp
    Source/Inspector/InspectorPropertyGroupWidget.cpp
    Source/Util/MaterialPropertyUtil.cpp
    Source/Util/Util.cpp
    Source/Viewport/RenderViewportWidget.cpp
)
