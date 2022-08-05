/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ActionManagerInternalInterface;
    class HotKeyManagerInterface;

    //! HotKey Manager class definition.
    //! Handles Editor HotKeys and allows access across tools.
    class HotKeyManager
        : public HotKeyManagerInterface
    {
    public:
        HotKeyManager();
        virtual ~HotKeyManager();

        // HotKeyManagerInterface overrides ...
        HotKeyManagerOperationResult SetActionHotKey(const AZStd::string& actionIdentifier, const AZStd::string& hotKey) override;

    private:
        struct HotKeyMapping final
        {
            AZ_CLASS_ALLOCATOR(HotKeyMapping, AZ::SystemAllocator, 0);
            AZ_RTTI(HotKeyMapping, "{3A928602-A2B2-4B58-A2C6-2DF73351D35D}");

            AZStd::unordered_map<AZStd::string, AZStd::string> m_actionToHotKeyMap;
        };

        HotKeyMapping m_hotKeyMapping;

        inline static ActionManagerInterface* m_actionManagerInterface = nullptr;
        inline static ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
