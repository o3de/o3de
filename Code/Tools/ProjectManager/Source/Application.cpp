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

#include <Application.h>
#include <ProjectUtils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Logging/LoggingComponent.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

#include <QApplication>
#include <QDir>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    Application::~Application()
    {
        TearDown();
    }

    bool Application::Init()
    {
        constexpr char* applicationName{ "O3DE" };

        QApplication::setOrganizationName(applicationName);
        QApplication::setOrganizationDomain("o3de.org");

        QCoreApplication::setApplicationName(applicationName);
        QCoreApplication::setApplicationVersion("1.0");

        // Use the LogComponent for non-dev logging to user/log/O3DE.log
        RegisterComponentDescriptor(AzFramework::LogComponent::CreateDescriptor());

        AZ::IO::FixedMaxPath path = AZ::Utils::GetExecutableDirectory();
        // DevWriteStorage is where the event log is written during development
        m_settingsRegistry.get()->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage, path.LexicallyNormal().Native());

        // The event log will be saved in eventlogger/EventLogO3DE.azsl
        m_settingsRegistry.get()->Set(AZ::SettingsRegistryMergeUtils::BuildTargetNameKey, applicationName);

        Start(AzFramework::Application::Descriptor());

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);

        // Create the actual Qt Application - this needs to happen before using QMessageBox
        m_app.reset(new QApplication(*GetArgC(), *GetArgV()));

        if(!InitLog(applicationName))
        {
            AZ_Warning("ProjectManager", false, "Failed to init logging");
        }

        m_pythonBindings = AZStd::make_unique<PythonBindings>(GetEngineRoot());
        if (!m_pythonBindings || !m_pythonBindings->PythonStarted())
        {
            QMessageBox::critical(nullptr, QObject::tr("Failed to start Python"),
                QObject::tr("This tool requires an O3DE engine with a Python runtime, "
                    "but either Python is missing or mis-configured. Please rename "
                    "your python/runtime folder to python/runtime_bak, then run "
                    "python/get_python.bat to restore the Python runtime folder."));
            return false;
        }

        const AZ::CommandLine *commandLine = GetCommandLine();
        AZ_Assert(commandLine, "Failed to get command line");

        ProjectManagerScreen startScreen = ProjectManagerScreen::Projects;
        if(commandLine->HasSwitch("screen"))
        {
            QString screenOption = commandLine->GetSwitchValue("screen", 0).c_str();
            ProjectManagerScreen screen = ProjectUtils::GetProjectManagerScreen(screenOption);
            if (screen != ProjectManagerScreen::Invalid)
            {
                startScreen = screen;
            }
        }

        AZ::IO::FixedMaxPath projectPath;
        if (commandLine->HasSwitch("project-path"))
        {
            projectPath = commandLine->GetSwitchValue("project-path", 0).c_str();
        }

        m_mainWindow.reset(new ProjectManagerWindow(nullptr, projectPath, startScreen));

        return true;
    }

    bool Application::InitLog(const char* logName)
    {
        if (!m_entity)
        {
            // this entity exists because we need a home for LogComponent
            // and cannot use the system entity because we need to be able to call SetLogFileBaseName 
            // so the log will be named O3DE.log
            m_entity = aznew AZ::Entity("Application Entity");
            if (m_entity)
            {
                AzFramework::LogComponent* logger = aznew AzFramework::LogComponent();
                AZ_Assert(logger, "Failed to create LogComponent");
                logger->SetLogFileBaseName(logName);
                m_entity->AddComponent(logger);
                m_entity->Init();
                m_entity->Activate();
            }
        }

        return m_entity != nullptr;
    }

    void Application::TearDown()
    {
        if (m_entity)
        {
            m_entity->Deactivate();
            delete m_entity;
            m_entity = nullptr;
        }

        m_pythonBindings.reset();
        m_mainWindow.reset();
        m_app.reset();
    }

    bool Application::Run()
    {
        // Set up the Style Manager
        AzQtComponents::StyleManager styleManager(qApp);
        styleManager.initialize(qApp, GetEngineRoot());

        // setup stylesheets and hot reloading 
        AZ::IO::FixedMaxPath engineRoot(GetEngineRoot());
        QDir rootDir(engineRoot.c_str());
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectManager/Resources");
        const auto qrcPath = QStringLiteral(":/ProjectManager/style");
        AzQtComponents::StyleManager::addSearchPaths("style", pathOnDisk, qrcPath, engineRoot);

        // set stylesheet after creating the main window or their styles won't get updated
        AzQtComponents::StyleManager::setStyleSheet(m_mainWindow.data(), QStringLiteral("style:ProjectManager.qss"));

        qApp->setWindowIcon(QIcon("style:o3de_editor.ico"));

        // the decoration wrapper is intended to remember window positioning and sizing 
        auto wrapper = new AzQtComponents::WindowDecorationWrapper();
        wrapper->setGuest(m_mainWindow.data());
        wrapper->show();
        m_mainWindow->show();

        qApp->setQuitOnLastWindowClosed(true);

        // Run the application
        return qApp->exec();
    }

}
