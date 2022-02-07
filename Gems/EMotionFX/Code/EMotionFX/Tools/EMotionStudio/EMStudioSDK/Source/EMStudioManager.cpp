/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioManager.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "RecoverFilesWindow.h"
#include "MotionEventPresetManager.h"
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Commands.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>

// include MCore related
#include <MCore/Source/LogManager.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionManager.h>

#include <Source/Editor/SkeletonModel.h>

// include Qt related things
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPainterPath>
#include <QSettings>
#include <QSplashScreen>
#include <QStandardPaths>
#include <QTextStream>
#include <QUuid>
AZ_POP_DISABLE_WARNING

// include AzCore required headers
#include <AzFramework/API/ApplicationAPI.h>

namespace EMStudio
{
    //--------------------------------------------------------------------------
    // class EMStudioManager
    //--------------------------------------------------------------------------
    AZ_CLASS_ALLOCATOR_IMPL(EMStudioManager, AZ::SystemAllocator, 0)

    // constructor
    EMStudioManager::EMStudioManager(QApplication* app, [[maybe_unused]] int& argc, [[maybe_unused]] char* argv[])
    {
        // Flag that we have an editor around
        EMotionFX::GetEMotionFX().SetIsInEditorMode(true);

        m_htmlLinkString.reserve(32768);
        m_eventProcessingCallback = nullptr;
        m_autoLoadLastWorkspace = false;
        m_avoidRendering = false;

        m_app = app;

        AZ::AllocatorInstance<UIAllocator>::Create();
        
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
        }
        else
        {
            MainWindow::Reflect(serializeContext);
        }

        MCore::GetLogManager().AddLogCallback(new MCore::AzLogCallback());
        MCore::GetLogManager().SetLogLevels(MCore::LogCallback::LOGLEVEL_ALL);

        // Register editor specific commands.
        m_commandManager = new CommandSystem::CommandManager();
        m_commandManager->RegisterCommand(new CommandSaveActorAssetInfo());
        m_commandManager->RegisterCommand(new CommandSaveMotionAssetInfo());
        m_commandManager->RegisterCommand(new CommandSaveMotionSet());
        m_commandManager->RegisterCommand(new CommandSaveAnimGraph());
        m_commandManager->RegisterCommand(new CommandSaveWorkspace());
        m_commandManager->RegisterCommand(new CommandEditorLoadAnimGraph());
        m_commandManager->RegisterCommand(new CommandEditorLoadMotionSet());

        m_eventPresetManager         = new MotionEventPresetManager();
        m_pluginManager              = new PluginManager();
        m_layoutManager              = new LayoutManager();
        m_notificationWindowManager  = new NotificationWindowManager();
        m_compileDate = AZStd::string::format("%s", MCORE_DATE);

        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();

        // log some information
        LogInfo();

        AZ::Interface<EMStudioManager>::Register(this);
    }

    // destructor
    EMStudioManager::~EMStudioManager()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();

        if (m_eventProcessingCallback)
        {
            EMStudio::GetCommandManager()->RemoveCallback(m_eventProcessingCallback, false);
            delete m_eventProcessingCallback;
        }

        // delete all animgraph instances etc
        ClearScene();

        delete m_eventPresetManager;
        delete m_pluginManager;
        delete m_layoutManager;
        delete m_notificationWindowManager;
        delete m_mainWindow;
        delete m_commandManager;

        AZ::AllocatorInstance<UIAllocator>::Destroy();

        AZ::Interface<EMStudioManager>::Unregister(this);
    }

    MainWindow* EMStudioManager::GetMainWindow()
    {
        if (m_mainWindow.isNull())
        {
            m_mainWindow = new MainWindow();
            m_mainWindow->Init();
        }
        return m_mainWindow;
    }


    // clear everything
    void EMStudioManager::ClearScene()
    {
        GetMainWindow()->Reset();
        EMotionFX::GetAnimGraphManager().RemoveAllAnimGraphInstances(true);
        EMotionFX::GetAnimGraphManager().RemoveAllAnimGraphs(true);
        EMotionFX::GetMotionManager().Clear(true);
    }


    // log info
    void EMStudioManager::LogInfo()
    {
        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("EMotion Studio Core - Information");
        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("Compilation date: %s", GetCompileDate());
        MCore::LogInfo("-----------------------------------------------");
    }


    int EMStudioManager::ExecuteApp()
    {
        MCORE_ASSERT(m_app);
        MCORE_ASSERT(m_mainWindow);

#if !defined(EMFX_EMSTUDIOLYEMBEDDED)
        // try to load all plugins
        AZStd::string pluginDir = MysticQt::GetAppDir() + "Plugins/";

        m_pluginManager->LoadPluginsFromDirectory(pluginDir.c_str());
#endif // EMFX_EMSTUDIOLYEMBEDDED

        // Give a chance to every plugin to reflect data
        const size_t numPlugins = m_pluginManager->GetNumPlugins();
        if (numPlugins)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            }
            else
            {
                for (size_t i = 0; i < numPlugins; ++i)
                {
                    EMStudioPlugin* plugin = m_pluginManager->GetPlugin(i);
                    plugin->Reflect(serializeContext);
                }
            }
        }
        
        // Register the command event processing callback.
        m_eventProcessingCallback = new EventProcessingCallback();
        EMStudio::GetCommandManager()->RegisterCallback(m_eventProcessingCallback);

        // Update the main window create window item with, so that it shows all loaded plugins.
        m_mainWindow->UpdateCreateWindowMenu();

        // Set the recover save path.
        MCore::FileSystem::s_secureSavePath = GetManager()->GetRecoverFolder().c_str();

        // Show the main dialog and wait until it closes.
        MCore::LogInfo("EMotion Studio initialized...");

#if !defined(EMFX_EMSTUDIOLYEMBEDDED)
        m_mainWindow->show();
#endif // EMFX_EMSTUDIOLYEMBEDDED

        // Show the recover window in case we have some .recover files in the recovery folder.
        const QString secureSavePath = MCore::FileSystem::s_secureSavePath.c_str();
        const QStringList recoverFileList = QDir(secureSavePath).entryList(QStringList("*.recover"), QDir::Files);
        if (!recoverFileList.empty())
        {
            // Add each recover file to the array.
            const size_t numRecoverFiles = recoverFileList.size();
            AZStd::vector<AZStd::string> recoverStringArray;
            recoverStringArray.reserve(numRecoverFiles);
            AZStd::string recoverFilename, backupFilename;
            for (int i = 0; i < numRecoverFiles; ++i)
            {
                recoverFilename = QString(secureSavePath + recoverFileList[i]).toUtf8().data();

                backupFilename = recoverFilename;
                AzFramework::StringFunc::Path::StripExtension(backupFilename);

                // Add the recover filename only if the backup file exists.
                if (AZ::IO::FileIOBase::GetInstance()->Exists(backupFilename.c_str()))
                {
                    recoverStringArray.push_back(recoverFilename);
                }
            }

            // Show the recover files window only in case there is a valid file to recover.
            if (!recoverStringArray.empty())
            {
                RecoverFilesWindow* recoverFilesWindow = new RecoverFilesWindow(m_mainWindow, recoverStringArray);
                recoverFilesWindow->exec();
            }
        }

        m_app->processEvents();

#if !defined(EMFX_EMSTUDIOLYEMBEDDED)
        // execute the application
        return m_app->exec();
#else
        return 0;
#endif // EMFX_EMSTUDIOLYEMBEDDED
    }

    void EMStudioManager::SetWidgetAsInvalidInput(QWidget* widget)
    {
        widget->setStyleSheet("border: 1px solid red;");
    }


    const char* EMStudioManager::ConstructHTMLLink(const char* text, const MCore::RGBAColor& color)
    {
        int32 r = aznumeric_cast<int32>(color.m_r * 256);
        int32 g = aznumeric_cast<int32>(color.m_g * 256);
        int32 b = aznumeric_cast<int32>(color.m_b * 256);

        m_htmlLinkString = AZStd::string::format("<qt><style>a { color: rgb(%i, %i, %i); } a:hover { color: rgb(40, 40, 40); }</style><a href='%s'>%s</a></qt>", r, g, b, text, text);
        return m_htmlLinkString.c_str();
    }


    void EMStudioManager::MakeTransparentButton(QToolButton* button, const char* iconFileName, const char* toolTipText, uint32 width, uint32 height)
    {
        button->setObjectName("TransparentButton");
        button->setToolTip(toolTipText);
        button->setMinimumSize(width, height);
        button->setMaximumSize(width, height);
        button->setIcon(MysticQt::GetMysticQt()->FindIcon(iconFileName));
    }

    void EMStudioManager::MakeTransparentButton(QPushButton* button, const char* iconFileName, const char* toolTipText, uint32 width, uint32 height)
    {
        button->setObjectName("TransparentButton");
        button->setToolTip(toolTipText);
        button->setMinimumSize(width, height);
        button->setMaximumSize(width, height);
        button->setIcon(MysticQt::GetMysticQt()->FindIcon(iconFileName));
    }


    void EMStudioManager::MakeTransparentMenuButton(QPushButton* button, const char* iconFileName, const char* toolTipText, uint32 width, uint32 height)
    {
        button->setToolTip(toolTipText);
        button->setMinimumSize(width, height);
        button->setMaximumSize(width, height);
        button->setIcon(MysticQt::GetMysticQt()->FindIcon(iconFileName));

        button->setObjectName("EMFXMenuButton");
        button->setStyleSheet("QPushButton#EMFXMenuButton::menu-indicator \
                               { \
                                   subcontrol-position: right bottom; \
                                   subcontrol-origin: padding; \
                                   left: 0px; \
                                   top: -2px; \
                               }");
    }


    QLabel* EMStudioManager::MakeSeperatorLabel(uint32 width, uint32 height)
    {
        QLabel* seperatorLabel = new QLabel("");
        seperatorLabel->setObjectName("SeperatorLabel");
        seperatorLabel->setMinimumSize(width, height);
        seperatorLabel->setMaximumSize(width, height);

        return seperatorLabel;
    }


    void EMStudioManager::SetVisibleJointIndices(const AZStd::unordered_set<size_t>& visibleJointIndices)
    {
        m_visibleJointIndices = visibleJointIndices;
    }

    void EMStudioManager::SetSelectedJointIndices(const AZStd::unordered_set<size_t>& selectedJointIndices)
    {
        m_selectedJointIndices = selectedJointIndices;
    }

    void EMStudioManager::JointSelectionChanged()
    {
        AZ::Outcome<const QModelIndexList&> selectedRowIndicesOutcome;
        EMotionFX::SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &EMotionFX::SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!selectedRowIndicesOutcome.IsSuccess())
        {
            return;
        }

        m_selectedJointIndices.clear();

        const QModelIndexList& selectedRowIndices = selectedRowIndicesOutcome.GetValue();
        if (selectedRowIndices.empty())
        {
            return;
        }

        for (const QModelIndex& selectedIndex : selectedRowIndices)
        {
            EMotionFX::Node* joint = selectedIndex.data(EMotionFX::SkeletonModel::ROLE_POINTER).value<EMotionFX::Node*>();
            m_selectedJointIndices.emplace(joint->GetNodeIndex());
        }
    }


    // before executing a command
    void EMStudioManager::EventProcessingCallback::OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(group);
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        //  EMStudio::GetApp()->blockSignals(true);
        EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::WaitCursor));
        //EMStudio::GetApp()->processEvents();
    }


    // after executing a command
    void EMStudioManager::EventProcessingCallback::OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult)
    {
        MCORE_UNUSED(group);
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        MCORE_UNUSED(wasSuccess);
        MCORE_UNUSED(outResult);
        //EMStudio::GetApp()->processEvents();
        //  EMStudio::GetApp()->blockSignals(false);
        EMStudio::GetApp()->restoreOverrideCursor();
    }


    AZStd::string EMStudioManager::GetAppDataFolder() const
    {
        AZStd::string appDataFolder = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0).toUtf8().data();
        appDataFolder += "/EMotionStudio/";

        QDir dir(appDataFolder.c_str());
        dir.mkpath(appDataFolder.c_str());

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, appDataFolder);
        return appDataFolder.c_str();
    }


    AZStd::string EMStudioManager::GetRecoverFolder() const
    {
        // Set the recover path
        const AZStd::string recoverPath = GetAppDataFolder() + "Recover" + AZ_CORRECT_FILESYSTEM_SEPARATOR;

        // create all folders needed
        QDir dir;
        dir.mkpath(recoverPath.c_str());

        // return the recover path
        return recoverPath;
    }


    AZStd::string EMStudioManager::GetAutosavesFolder() const
    {
        // Set the autosaves path
        const AZStd::string autosavesPath = GetAppDataFolder() + "Autosaves" + AZ_CORRECT_FILESYSTEM_SEPARATOR;

        // create all folders needed
        QDir dir;
        dir.mkpath(autosavesPath.c_str());

        // return the autosaves path
        return autosavesPath;
    }


    EMStudioManager* EMStudioManager::GetInstance()
    {
        return AZ::Interface<EMStudioManager>().Get();
    }


    // function to add a gizmo to the manager
    MCommon::TransformationManipulator* EMStudioManager::AddTransformationManipulator(MCommon::TransformationManipulator* manipulator)
    {
        // check if manipulator exists
        if (manipulator == nullptr)
        {
            return nullptr;
        }

        // add and return the manipulator
        m_transformationManipulators.emplace_back(manipulator);
        return manipulator;
    }


    // remove the given gizmo from the array
    void EMStudioManager::RemoveTransformationManipulator(MCommon::TransformationManipulator* manipulator)
    {
        if (const auto it = AZStd::find(begin(m_transformationManipulators), end(m_transformationManipulators), manipulator); it != end(m_transformationManipulators))
        {
            m_transformationManipulators.erase(it);
        }
    }


    // returns the gizmo array
    AZStd::vector<MCommon::TransformationManipulator*>* EMStudioManager::GetTransformationManipulators()
    {
        return &m_transformationManipulators;
    }


    // new temporary helper function for text drawing
    void EMStudioManager::RenderText(QPainter& painter, const QString& text, const QColor& textColor, const QFont& font, const QFontMetrics& fontMetrics, Qt::Alignment textAlignment, const QRect& rect)
    {
        painter.setFont(font);
        painter.setPen(Qt::NoPen);
        painter.setBrush(textColor);

        const float textWidth       = aznumeric_cast<float>(fontMetrics.horizontalAdvance(text));
        const float halfTextWidth   = aznumeric_cast<float>(textWidth * 0.5 + 0.5);
        const float halfTextHeight  = aznumeric_cast<float>(fontMetrics.height() * 0.5 + 0.5);
        const QPoint rectCenter     = rect.center();

        QPoint textPos;
        textPos.setY(aznumeric_cast<int>(rectCenter.y() + halfTextHeight - 1));

        switch (textAlignment)
        {
        case Qt::AlignLeft:
        {
            textPos.setX(rect.left() - 2);
            break;
        }

        case Qt::AlignRight:
        {
            textPos.setX(aznumeric_cast<int>(rect.right() - textWidth + 1));
            break;
        }

        default:
        {
            textPos.setX(aznumeric_cast<int>(rectCenter.x() - halfTextWidth + 1));
        }
        }

        QPainterPath path;
        path.addText(textPos, font, text);
        painter.drawPath(path);
    }

    // shortcuts
    QApplication* GetApp()
    {
        return EMStudioManager::GetInstance()->GetApp();
    }
    EMStudioManager* GetManager()
    {
        return EMStudioManager::GetInstance();
    }

    bool HasMainWindow()
    {
        return EMStudioManager::GetInstance()->HasMainWindow();
    }

    MainWindow* GetMainWindow()
    {
        return EMStudioManager::GetInstance()->GetMainWindow();
    }

    PluginManager* GetPluginManager()
    {
        return EMStudioManager::GetInstance()->GetPluginManager();
    }

    LayoutManager* GetLayoutManager()
    {
        return EMStudioManager::GetInstance()->GetLayoutManager();
    }

    NotificationWindowManager* GetNotificationWindowManager()
    {
        return EMStudioManager::GetInstance()->GetNotificationWindowManager();
    }

    MotionEventPresetManager* GetEventPresetManager()
    {
        return EMStudioManager::GetInstance()->GetEventPresetManger();
    }

    CommandSystem::CommandManager* GetCommandManager()
    {
        return EMStudioManager::GetInstance()->GetCommandManager();
    }
} // namespace EMStudio
