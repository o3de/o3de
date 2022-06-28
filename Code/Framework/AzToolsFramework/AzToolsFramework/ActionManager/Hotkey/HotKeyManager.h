/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>

namespace AzToolsFramework
{   
    //! HotKey Manager class definition.
    //! Handles Editor HotKey and allows access across tools.
    class HotKeyManager
        : private HotKeyManagerInterface
    {
    public:
        HotKeyManager();
        virtual ~HotKeyManager();

    private:

    };

} // namespace AzToolsFramework
