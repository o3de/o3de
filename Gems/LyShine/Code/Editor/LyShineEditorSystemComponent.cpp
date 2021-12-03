/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineEditorSystemComponent.h"
#include "EditorWindow.h"

// UI_ANIMATION_REVISIT, added includes so that we can register the UI Animation system on startup
#include "Animation/UiAnimViewSequenceManager.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <LyViewPaneNames.h>
#include <ISystem.h>
#include <IConsole.h>

#include <QScreen>
#include <QApplication>

namespace LyShineEditor
{
    bool IsCanvasEditorEnabled()
    {
        bool isCanvasEditorEnabled = false;

        const ICVar* isCanvasEditorEnabledCVar = (gEnv && gEnv->pConsole) ? gEnv->pConsole->GetCVar("sys_enableCanvasEditor") : NULL;
        if (isCanvasEditorEnabledCVar && (isCanvasEditorEnabledCVar->GetIVal() == 1))
        {
            isCanvasEditorEnabled = true;
        }

        return isCanvasEditorEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineEditorSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                auto editInfo = ec->Class<LyShineEditorSystemComponent>("UI Canvas Editor", "UI Canvas Editor System Component");
                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "UI")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiCanvasEditorService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiCanvasEditorService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("LyShineService", 0xae98ab29));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LyShineEditorSystemComponent::LyShineEditorSystemComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LyShineEditorSystemComponent::~LyShineEditorSystemComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEventsBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        LyShine::LyShineRequestBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::Deactivate()
    {
        if (IsCanvasEditorEnabled())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::UnregisterViewPane(LyViewPane::UiEditor);

            CUiAnimViewSequenceManager::Destroy();
        }
        LyShine::LyShineRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::NotifyRegisterViews()
    {
        if (IsCanvasEditorEnabled())
        {
            // Calculate default editor size and position.
            // For landscape screens, use 75% of the screen. For portrait screens, use 95% of screen width and 4:3 aspect ratio
            QRect deskRect = QApplication::primaryScreen()->availableGeometry();
            float availableWidth = (float)deskRect.width() * ((deskRect.width() > deskRect.height()) ? 0.75f : 0.95f);
            float availableHeight = (float)deskRect.height() * 0.75f;
            float editorWidth = availableWidth;
            float editorHeight = (deskRect.width() > deskRect.height()) ? availableHeight : (editorWidth * 3.0f / 4.0f);
            if ((availableWidth / availableHeight) > (editorWidth / editorHeight))
            {
                editorWidth = editorWidth * availableHeight / editorHeight;
                editorHeight = availableHeight;
            }
            else
            {
                editorWidth = availableWidth;
                editorHeight = editorHeight * availableWidth / editorWidth;
            }
            int x = (int)((float)deskRect.left() + (((float)deskRect.width() - availableWidth) / 2.0f) + ((availableWidth - editorWidth) / 2.0f));
            int y = (int)((float)deskRect.top() + (((float)deskRect.height() - availableHeight) / 2.0f) + ((availableHeight - editorHeight) / 2.0f));

            AzToolsFramework::ViewPaneOptions opt;
            opt.isPreview = true;
            opt.paneRect = QRect(x, y, (int)editorWidth, (int)editorHeight);
            opt.isDeletable = true; // we're in a plugin; make sure we can be deleted
            opt.showOnToolsToolbar = true;
            opt.toolbarIcon = ":/Menu/ui_editor.svg";
            // opt.canHaveMultipleInstances = true; // uncomment this when CUiAnimViewSequenceManager::CanvasUnloading supports multiple canvases
            AzToolsFramework::RegisterViewPane<EditorWindow>(LyViewPane::UiEditor, LyViewPane::CategoryTools, opt);

            CUiAnimViewSequenceManager::Create();

            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& /*sourceUUID*/, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        if (AZStd::wildcard_match("*.uicanvas", fullSourceFileName))
        {
            openers.push_back({ "O3DE_UICanvas_Editor",
                "Open in UI Canvas Editor...",
                QIcon(),
                [](const char* fullSourceFileNameInCallback, const AZ::Uuid& /*sourceUUID*/)
            {
                AzToolsFramework::OpenViewPane(LyViewPane::UiEditor);
                QString absoluteName = QString::fromUtf8(fullSourceFileNameInCallback);
                UiEditorDLLBus::Broadcast(&UiEditorDLLInterface::OpenSourceCanvasFile, absoluteName);
            } });
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AzToolsFramework::AssetBrowser::SourceFileDetails LyShineEditorSystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
    {
        if (AZStd::wildcard_match("*.uicanvas", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/UICanvas_16.png");
        }

        if (AZStd::wildcard_match("*.sprite", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/Sprite_16.png");
        }
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::EditUICanvas([[maybe_unused]] const AZStd::string_view& canvasPath)
    {
        AzToolsFramework::OpenViewPane(LyViewPane::UiEditor);
        AZStd::string stringPath = canvasPath;

        if (!stringPath.empty())
        {
            QString absoluteName = stringPath.c_str();
            UiEditorDLLBus::Broadcast(&UiEditorDLLInterface::OpenSourceCanvasFile, absoluteName);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineEditorSystemComponent::OnStopPlayInEditor()
    {
        // reset UI system
        if (gEnv->pLyShine)
        {
            gEnv->pLyShine->Reset();
        }
    }
}
