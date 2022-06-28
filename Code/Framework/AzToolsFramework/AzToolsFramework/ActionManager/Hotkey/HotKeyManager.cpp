/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>

namespace AzToolsFramework
{
    HotKeyManager::HotKeyManager()
    {
        AZ::Interface<HotKeyManagerInterface>::Register(this);
    }

    HotKeyManager::~HotKeyManager()
    {
        AZ::Interface<HotKeyManager>::Unregister(this);
    }

} // namespace AzToolsFramework
