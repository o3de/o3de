/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <IEditor.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>


namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerEditorSystemComponent final
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AzFramework::GameEntityContextEventBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(MultiplayerEditorSystemComponent, "{9F335CC0-5574-4AD3-A2D8-2FAEF356946C}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerEditorSystemComponent();
        ~MultiplayerEditorSystemComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! EditorEvents::Bus::Handler overrides.
        //! @{
        void NotifyRegisterViews() override;
        //! @}

    private:

        //! AZ::TickBus::Handler overrides.
        //! @{
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //! @}
        
        //! EditorEvents::Handler overrides
        //! @{
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
        //! @}
        
        //!  GameEntityContextEventBus::Handler overrides
        //! @{
        void OnGameEntitiesStarted() override; 
        //! @}

        IEditor* m_editor = nullptr;
        AzFramework::ProcessWatcher* m_serverProcess = nullptr;
    };
}
