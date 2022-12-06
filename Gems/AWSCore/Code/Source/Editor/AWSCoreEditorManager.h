/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

namespace AWSCore
{
    class AWSCoreEditorMenu;

    class AWSCoreEditorManager
    {
    public:
        static constexpr const char AWS_MENU_TEXT[] = "&AWS";

        AWSCoreEditorManager();
        virtual ~AWSCoreEditorManager();

    private:

    };
} // namespace AWSCore
