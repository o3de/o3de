/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/AWSCoreEditorManager.h>
#include <Editor/UI/AWSCoreEditorMenu.h>

namespace AWSCore
{
    AWSCoreEditorManager::AWSCoreEditorManager()
        : m_awsCoreEditorMenu(new AWSCoreEditorMenu(AWS_MENU_TEXT))
    {
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
