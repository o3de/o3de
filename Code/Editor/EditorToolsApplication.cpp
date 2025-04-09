/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorToolsApplication.h"

// Qt
#include <QMessageBox>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>

// Editor
#include "MainWindow.h"
#include "Controls/ReflectedPropertyControl/ReflectedVar.h"
#include "CryEdit.h"
#include "DisplaySettingsPythonFuncs.h"
#include "GameEngine.h"
#include "PythonEditorFuncs.h"
#include "TrackView/TrackViewPythonFuncs.h"

namespace EditorInternal
{
    EditorToolsApplication::EditorToolsApplication(AZ::ComponentApplicationSettings componentAppSettings)
        : EditorToolsApplication(nullptr, nullptr, AZStd::move(componentAppSettings))
    {
    }
    EditorToolsApplication::EditorToolsApplication(int* argc, char*** argv)
        : EditorToolsApplication(argc, argv, {})
    {
    }

    EditorToolsApplication::EditorToolsApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
        : ToolsApplication(argc, argv, AZStd::move(componentAppSettings))
    {
        EditorToolsApplicationRequests::Bus::Handler::BusConnect();
        AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler::BusConnect();
        AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler::BusConnect();
    }

    EditorToolsApplication::~EditorToolsApplication()
    {
        AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler::BusDisconnect();
        EditorToolsApplicationRequests::Bus::Handler::BusDisconnect();
        Stop();
    }


    bool EditorToolsApplication::IsStartupAborted() const
    {
        return m_StartupAborted;
    }

    void EditorToolsApplication::RegisterCoreComponents()
    {
        AzToolsFramework::ToolsApplication::RegisterCoreComponents();

        // Expose Legacy Python Bindings to Behavior Context for EditorPythonBindings Gem
        RegisterComponentDescriptor(AzToolsFramework::CryEditPythonHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::CryEditDocFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::MainWindowEditorFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PythonEditorComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PythonEditorFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TrackViewComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TrackViewFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ViewPanePythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ViewportTitleDlgPythonFuncsHandler::CreateDescriptor());
    }

    AZ::ComponentTypeList EditorToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();

        components.emplace_back(azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>());

        // Add new Bus-based Python Bindings
        components.emplace_back(azrtti_typeid<AzToolsFramework::DisplaySettingsComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::PythonEditorComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::TrackViewComponent>());

        return components;
    }


    void EditorToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzToolsFramework::ToolsApplication::StartCommon(systemEntity);

        m_StartupAborted = m_moduleManager->m_quitRequested;

        if (systemEntity->GetState() != AZ::Entity::State::Active)
        {
            m_StartupAborted = true;
        }
    }

    bool EditorToolsApplication::Start()
    {
        AzFramework::Application::StartupParameters params;

        // Must be done before creating QApplication, otherwise asserts when we alloc
        AzToolsFramework::ToolsApplication::Start({}, params);
        if (IsStartupAborted() || !m_systemEntity)
        {
            AzToolsFramework::ToolsApplication::Stop();
            return false;
        }
        return true;
    }

    void EditorToolsApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Editor | AZ::ApplicationTypeQuery::Masks::Tool;
    };

    void EditorToolsApplication::CreateReflectionManager()
    {
        ToolsApplication::CreateReflectionManager();

        GetSerializeContext()->CreateEditContext();
    }

    void EditorToolsApplication::Reflect(AZ::ReflectContext* context)
    {
        ToolsApplication::Reflect(context);

        // Reflect property control classes to the serialize context...
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ReflectedVarInit::setupReflection(serializeContext);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorToolsApplicationRequestBus>("EditorToolsApplicationRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Event("OpenLevel", &EditorToolsApplicationRequests::OpenLevel)
                ->Event("OpenLevelNoPrompt", &EditorToolsApplicationRequests::OpenLevelNoPrompt)
                ->Event("CreateLevel", &EditorToolsApplicationRequests::CreateLevel)
                ->Event("CreateLevelNoPrompt", &EditorToolsApplicationRequests::CreateLevelNoPrompt)
                ->Event("GetGameFolder", &EditorToolsApplicationRequests::GetGameFolder)
                ->Event("GetCurrentLevelName", &EditorToolsApplicationRequests::GetCurrentLevelName)
                ->Event("GetCurrentLevelPath", &EditorToolsApplicationRequests::GetCurrentLevelPath)
                ->Event("Exit", &EditorToolsApplicationRequests::Exit)
                ->Event("ExitNoPrompt", &EditorToolsApplicationRequests::ExitNoPrompt)
                ;
        }
    }

    AZStd::string EditorToolsApplication::GetGameFolder() const
    {
        return Path::GetEditingGameDataFolder();
    }

    bool EditorToolsApplication::OpenLevel(AZStd::string_view levelName)
    {
        AZStd::string levelPath = levelName;

        if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
        {
            // now let's check if they pre-pended directories (Samples/SomeLevelName)
            AZStd::string levelFileName;
            {
                AZStd::size_t found = levelPath.find_last_of("/\\");
                if (found == AZStd::string::npos)
                {
                    levelFileName = levelPath.substr(found + 1);
                }
                else
                {
                    levelFileName = levelPath;
                }
            }

            // if the input path can't be found, let's automatically add on the game folder and the levels
            AzFramework::StringFunc::Path::Join(levelPath.c_str(), levelFileName.c_str(), levelPath);
            AzFramework::StringFunc::Path::Join(DefaultLevelFolder, levelPath.c_str(), levelPath);
            AzFramework::StringFunc::Path::Join(GetGameFolder().c_str(), levelPath.c_str(), levelPath);

            // make sure the level path includes the cry extension, if needed
            if (!levelFileName.ends_with(GetOldCryLevelExtension()) && !levelFileName.ends_with(GetLevelExtension()))
            {
                AZStd::size_t levelPathLength = levelPath.length();
                levelPath += GetOldCryLevelExtension();

                // Check if there is a .cry file, otherwise assume it is a new .ly file
                if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
                {
                    levelPath.replace(levelPathLength, sizeof(GetOldCryLevelExtension()) - 1, GetLevelExtension());
                }
            }

            if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
            {
                return false;
            }
        }

        auto previousDocument = GetIEditor()->GetDocument();
        QString previousPathName = (previousDocument != nullptr) ? previousDocument->GetLevelPathName() : "";
        auto newDocument = CCryEditApp::instance()->OpenDocumentFile(levelPath.c_str(), true, COpenSameLevelOptions::ReopenLevelIfSame);

        // the underlying document pointer doesn't change, so we can't check that; use the path name's instead
        return newDocument && !newDocument->IsLevelLoadFailed();
    }

    bool EditorToolsApplication::OpenLevelNoPrompt(AZStd::string_view levelName)
    {
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        return OpenLevel(levelName);
    }

    int EditorToolsApplication::CreateLevel(AZStd::string_view templateName, AZStd::string_view levelName, bool /*bUseTerrain*/)
    {
        // Clang warns about a temporary being created in a function's argument list, so fullyQualifiedLevelName before the call
        QString fullyQualifiedLevelName;
        return CCryEditApp::instance()->CreateLevel(QString::fromUtf8(templateName.data(), static_cast<int>(templateName.size())),
                                                    QString::fromUtf8(levelName.data(), static_cast<int>(levelName.size())),
                                                    fullyQualifiedLevelName);
    }

    int EditorToolsApplication::CreateLevelNoPrompt(AZStd::string_view templateName, AZStd::string_view levelName, int /*terrainExportTextureSize*/, bool /*useTerrain*/)
    {
        // If a level was open, ignore any unsaved changes if it had been modified
        if (GetIEditor()->IsLevelLoaded())
        {
            GetIEditor()->GetDocument()->SetModifiedFlag(false);
        }

        // Clang warns about a temporary being created in a function's argument list, so fullyQualifiedLevelName before the call
        QString fullyQualifiedLevelName;
        return CCryEditApp::instance()->CreateLevel(QString::fromUtf8(templateName.data(), static_cast<int>(templateName.size())),
                                                    QString::fromUtf8(levelName.data(), static_cast<int>(levelName.size())),
                                                    fullyQualifiedLevelName);
    }

    AZStd::string EditorToolsApplication::GetCurrentLevelName() const
    {
        return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data());
    }

    AZStd::string EditorToolsApplication::GetCurrentLevelPath() const
    {
        return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data());
    }

    const char* EditorToolsApplication::GetLevelExtension() const
    {
        return ".prefab";
    }

    const char* EditorToolsApplication::GetOldCryLevelExtension() const
    {
        return ".cry";
    }

    void EditorToolsApplication::Exit()
    {
        // Adding a single-shot QTimer to PyExit delays the QApplication::closeAllWindows call until
        // all the events in the event queue have been processed. Calling QApplication::closeAllWindows instead
        // of MainWindow::close ensures the Metal render window is cleaned up on macOS.
        QTimer::singleShot(0, qApp, &QApplication::closeAllWindows);
    }

    void EditorToolsApplication::ExitNoPrompt()
    {
        // Set the level to "unmodified" so that it doesn't prompt to save on exit.
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        Exit();
    }

    AzToolsFramework::ViewportInteraction::KeyboardModifiers EditorToolsApplication::QueryKeyboardModifiers()
    {
        return AzToolsFramework::ViewportInteraction::BuildKeyboardModifiers(QGuiApplication::queryKeyboardModifiers());
    }

    AZStd::chrono::milliseconds EditorToolsApplication::EditorViewportInputTimeNow()
    {
        const auto now = AZStd::chrono::steady_clock::now();
        return AZStd::chrono::time_point_cast<AZStd::chrono::milliseconds>(now).time_since_epoch();
    }
} // namespace EditorInternal
