/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include "DrillerContext.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <QMessageBox>
#include <AzToolsFramework/UI/UICore/SaveChangesDialog.hxx>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <Source/Telemetry/TelemetryBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

namespace Driller
{
    const char* DrillerDebugName = "Profiler";
    const char* DrillerInfoName = "Profiler";

    class DrillerSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(DrillerSavedState, "{CBA064FC-B144-4B9D-92B8-F696B0A15E4D}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(DrillerSavedState, AZ::SystemAllocator, 0);

        bool m_MainDrillerWindowIsVisible;
        bool m_MainDrillerWindowIsOpen;

        DrillerSavedState()
            : m_MainDrillerWindowIsVisible(true)
            , m_MainDrillerWindowIsOpen(true) {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<DrillerSavedState>()
                    ->Version(1)
                    ->Field("m_MainDrillerWindowIsVisible", &DrillerSavedState::m_MainDrillerWindowIsVisible)
                    ->Field("m_MainDrillerWindowIsOpen", &DrillerSavedState::m_MainDrillerWindowIsOpen);
            }
        }
    };

    AZ::Uuid ContextID("FB8B7094-63FF-4CD1-9857-3AEFA8E2CFDC");


    //////////////////////////////////////////////////////////////////////////
    //Context
    Context::Context()
        : m_pDrillerMainWindow(NULL)
    {
    }

    Context::~Context()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Context::Init()
    {
    }

    void Context::Activate()
    {
        ContextInterface::Handler::BusConnect(ContextID);
        LegacyFramework::CoreMessageBus::Handler::BusConnect();

        AzToolsFramework::MainWindowDescription desc;
        desc.name = "Profiler";
        desc.ContextID = ContextID;
        desc.hotkeyDesc = AzToolsFramework::HotkeyDescription(AZ_CRC("DrillerOpen", 0x1cbbd497), "Ctrl+Shift+D", "Open Profiler", "General", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW);
        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, AddComponentInfo, desc);

        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = desc.name;
        AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor,
            &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);
    }

    void Context::Deactivate()
    {
        LegacyFramework::CoreMessageBus::Handler::BusDisconnect();
        ContextInterface::Handler::BusDisconnect(ContextID);
    }

    void Context::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            DrillerMainWindow::Reflect(context);
            DrillerSavedState::Reflect(context);

            serialize->Class<Context, AZ::Component>()
                ->Version(1)
            ;
        }
    }

    void Context::ApplicationDeactivated()
    {
    }

    void Context::ApplicationActivated()
    {
    }

    void Context::ApplicationShow(AZ::Uuid id)
    {
        if (ContextID == id)
        {
            ProvisionalShowAndFocus(true);
        }
    }
    void Context::ApplicationHide(AZ::Uuid id)
    {
        if (ContextID == id)
        {
            m_pDrillerMainWindow->hide();
            AZStd::intrusive_ptr<DrillerSavedState> newState = AZ::UserSettings::CreateFind<DrillerSavedState>(AZ_CRC("LUA DRILLER CONTEXT STATE", 0x95052376), AZ::UserSettings::CT_GLOBAL);
            newState->m_MainDrillerWindowIsVisible = false;
        }
    }

    void Context::ApplicationCensus()
    {
        AZStd::intrusive_ptr<DrillerSavedState> newState = AZ::UserSettings::CreateFind<DrillerSavedState>(AZ_CRC("LUA DRILLER CONTEXT STATE", 0x95052376), AZ::UserSettings::CT_GLOBAL);
        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, ApplicationCensusReply, newState->m_MainDrillerWindowIsVisible);
    }


    //////////////////////////////////////////////////////////////////////////
    // EditorFramework CoreMessages
    void Context::OnRestoreState()
    {
        const AZStd::string k_launchString = "launch";
        const AZStd::string k_drillerString = "driller";

        bool GUIMode = true;
        EBUS_EVENT_RESULT(GUIMode, LegacyFramework::FrameworkApplicationMessages::Bus, IsRunningInGUIMode);
        if (!GUIMode)
        {
            return;
        }

        const AzFramework::CommandLine* commandLine = nullptr;

        EBUS_EVENT_RESULT(commandLine, LegacyFramework::FrameworkApplicationMessages::Bus, GetCommandLineParser);

        bool forceShow = false;
        bool forceHide = false;

        if (commandLine->HasSwitch(k_launchString))
        {
            forceHide = true;

            size_t numSwitchValues = commandLine->GetNumSwitchValues(k_launchString);

            for (size_t i = 0; i < numSwitchValues; ++i)
            {
                AZStd::string inputValue = commandLine->GetSwitchValue(k_launchString, i);

                if (inputValue.compare(k_drillerString) == 0)
                {
                    forceShow = true;
                    forceHide = false;
                }
            }
        }

        ProvisionalShowAndFocus(forceShow, forceHide);
    }

    bool Context::OnGetPermissionToShutDown() // until everyone returns true, we can't shut down.
    {
        AZ_TracePrintf(DrillerDebugName, "Context::OnGetPermissionToShutDown()\n");

        if (m_pDrillerMainWindow)
        {
            if (!m_pDrillerMainWindow->OnGetPermissionToShutDown())
            {
                return false;
            }
        }

        return true;
    }

    // until everyone returns true, we can't shut down.
    bool Context::CheckOkayToShutDown()
    {
        if (m_pDrillerMainWindow)
        {
            // confirmation that we're quitting.
            if (m_pDrillerMainWindow->isVisible())
            {
                m_pDrillerMainWindow->setEnabled(false);
                m_pDrillerMainWindow->hide();
            }
        }

        return true;
    }

    void Context::OnSaveState()
    {
        // notify main view to persist?
        if (m_pDrillerMainWindow)
        {
            m_pDrillerMainWindow->SaveWindowState();
        }
    }

    void Context::OnDestroyState()
    {
        if (m_pDrillerMainWindow)
        {
            delete m_pDrillerMainWindow;
        }
        m_pDrillerMainWindow = NULL;
    }

    //////////////////////////////////////////////////////////////////////////
    // Utility
    void Context::ProvisionalShowAndFocus(bool forcedShow, bool forcedHide)
    {
        AZStd::intrusive_ptr<DrillerSavedState> newState = AZ::UserSettings::CreateFind<DrillerSavedState>(AZ_CRC("LUA DRILLER CONTEXT STATE", 0x95052376), AZ::UserSettings::CT_GLOBAL);

        if (forcedShow)
        {
            newState->m_MainDrillerWindowIsOpen = true;
            newState->m_MainDrillerWindowIsVisible = true;
        }
        else if (forcedHide)
        {
            newState->m_MainDrillerWindowIsOpen = false;
            newState->m_MainDrillerWindowIsVisible = false;
        }

        if (newState->m_MainDrillerWindowIsOpen)
        {
            if (newState->m_MainDrillerWindowIsVisible)
            {
                if (!m_pDrillerMainWindow)
                {
                    m_pDrillerMainWindow = aznew DrillerMainWindow();
                }

                m_pDrillerMainWindow->show();
                m_pDrillerMainWindow->raise();
                m_pDrillerMainWindow->activateWindow();
                m_pDrillerMainWindow->setFocus();
            }
            else
            {
                if (m_pDrillerMainWindow)
                {
                    m_pDrillerMainWindow->hide();
                }
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    //ContextInterface

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////

    void Context::ShowDrillerView()
    {
        ProvisionalShowAndFocus(true);
    }
}
