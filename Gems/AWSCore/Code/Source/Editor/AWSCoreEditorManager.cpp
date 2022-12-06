/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/AWSCoreEditorManager.h>
#include <Editor/UI/AWSCoreEditorMenu.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AWSCore
{
    AWSCoreEditorManager::AWSCoreEditorManager()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "AWSCoreEditorManager - could not get ActionManagerInterface on RegisterActions.");


    }

    AWSCoreEditorManager::~AWSCoreEditorManager()
    {
        delete m_awsCoreEditorMenu;
    }

    AWSCoreEditorMenu* AWSCoreEditorManager::GetAWSCoreEditorMenu() const
    {
        return m_awsCoreEditorMenu;
    }
} // namespace AWSCore
