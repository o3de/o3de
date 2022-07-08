/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <LyShine/LyShineBus.h>

namespace LyShineEditor
{
    class LyShineEditorSystemComponent
        : public AZ::Component
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , protected AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , protected LyShine::LyShineRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LyShineEditorSystemComponent, "{64D08A3F-A682-4CAF-86C1-DA91638494BA}");

        LyShineEditorSystemComponent();
        ~LyShineEditorSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorEvents interface implementation
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetBrowserInteractionNotifications interface implementation
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& /*sourceUUID*/, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LyShineRequestBus interface implementation
        void EditUICanvas(const AZStd::string_view& canvasPath) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorEntityContextNotificationBus
        void OnStopPlayInEditor() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
