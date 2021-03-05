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
    UI/LegacyFramework/MainWindowSavedState.h
    UI/LegacyFramework/MainWindowSavedState.cpp
    UI/LegacyFramework/UIFramework.hxx
    UI/LegacyFramework/UIFramework.cpp
    UI/LegacyFramework/UIFrameworkAPI.h
    UI/LegacyFramework/UIFrameworkAPI.cpp
    UI/LegacyFramework/UIFrameworkPreferences.cpp
    UI/LegacyFramework/Resources/sharedResources.qrc
    UI/LegacyFramework/Core/EditorContextBus.h
    UI/LegacyFramework/Core/EditorFrameworkAPI.h
    UI/LegacyFramework/Core/EditorFrameworkAPI.cpp
    UI/LegacyFramework/Core/EditorFrameworkApplication.h
    UI/LegacyFramework/Core/EditorFrameworkApplication.cpp
    UI/LegacyFramework/Core/IPCComponent.h
    UI/LegacyFramework/Core/IPCComponent.cpp
    UI/LegacyFramework/CustomMenus/CustomMenusAPI.h
    UI/LegacyFramework/CustomMenus/CustomMenusComponent.cpp
    UI/UICore/OverwritePromptDialog.hxx
    UI/UICore/OverwritePromptDialog.cpp
    UI/UICore/OverwritePromptDialog.ui
    UI/UICore/SaveChangesDialog.hxx
    UI/UICore/SaveChangesDialog.cpp
    UI/UICore/SaveChangesDialog.ui
    Process/internal/ProcessWatcher_Win.cpp
    Process/internal/ProcessCommon_Win.h
    Process/internal/ProcessCommunicator_Win.cpp
    ToolsFileUtils/ToolsFileUtils_win.cpp
)
