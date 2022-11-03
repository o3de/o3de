/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManager.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    //! System Component to handle the Action Manager system and subsystems.
    class ActionManagerSystemComponent final
        : public AZ::Component
        , public AzToolsFramework::EditorEventsBus::Handler
    {
    public:
        AZ_COMPONENT(ActionManagerSystemComponent, "{47925132-7373-42EE-9131-F405EE4B0F1A}");

        ActionManagerSystemComponent() = default;
        virtual ~ActionManagerSystemComponent();

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EditorEvents overrides ...
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        AZStd::unique_ptr<ActionManager> m_actionManager = nullptr;
        AZStd::unique_ptr<HotKeyManager> m_hotKeyManager = nullptr;
        AZStd::unique_ptr<MenuManager> m_menuManager = nullptr;
        AZStd::unique_ptr<ToolBarManager> m_toolBarManager = nullptr;

        QWidget* m_defaultParentObject = nullptr;
    };

} // namespace AzToolsFramework
