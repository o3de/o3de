/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "CryEdit.h"

// Qt
#include <QTimer>

// AzCore
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

// Editor
#include "Core/QtEditorApplication.h"
#include "CheckOutDialog.h"
#include "GameEngine.h"
#include "UndoConfigSpec.h"
#include "ViewManager.h"
#include "EditorViewportCamera.h"

// Atom
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

//////////////////////////////////////////////////////////////////////////
namespace
{
    void PyRunFile(const AZ::ConsoleCommandContainer& args)
    {
        if (args.empty())
        {
            // We expect at least "pyRunFile" and the filename
            AZ_Warning("editor", false, "The pyRunFile requires a file script name.");
            return;
        }
        else if (args.size() == 1)
        {
            // If we only have "pyRunFile filename", there are no args to pass through.
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                args.front().data());
        }
        else
        {
            // We have "pyRunFile filename x y z", so copy everything past filename into a new vector
            AZStd::vector<AZStd::string_view> pythonArgs;
            AZStd::transform(args.begin() + 1, args.end(), std::back_inserter(pythonArgs), [](auto&& value) { return value; });
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                args[0],
                pythonArgs);
        }
    }
    AZ_CONSOLEFREEFUNC("pyRunFile", PyRunFile, AZ::ConsoleFunctorFlags::Null, "Runs the Python script from the console.");

    //! We have explicitly not exposed this CloseCurrentLevel API to Python Scripting since the Editor
    //! doesn't officially support it (it doesn't exist in the File menu). It is used for cases
    //! where a level with legacy entities are unable to be converted and so the level that has
    //! been opened needs to be closed, but it hasn't been fully tested for a normal workflow.
    void CloseCurrentLevel()
    {
        CCryEditDoc* currentLevel = GetIEditor()->GetDocument();
        if (currentLevel && currentLevel->IsDocumentReady())
        {
            // This closes the current document (level)
            currentLevel->OnNewDocument();

            // Then we need to tell the game engine there is no level to render anymore
            if (GetIEditor()->GetGameEngine())
            {
                GetIEditor()->GetGameEngine()->SetLevelPath("");
                GetIEditor()->GetGameEngine()->SetLevelLoaded(false);

                CViewManager* pViewManager = GetIEditor()->GetViewManager();
                CViewport* pGameViewport = pViewManager ? pViewManager->GetGameViewport() : nullptr;
                if (pGameViewport)
                {
                    pGameViewport->SetViewTM(Matrix34::CreateIdentity());
                }
            }
        }
    }

    AZStd::string PyGetGameFolderAsString()
    {
        return Path::GetEditingGameDataFolder();
    }

    bool PyOpenLevel(const char* pLevelName)
    {
        const char* oldExtension = EditorUtils::LevelFile::GetOldCryFileExtension();
        const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

        QString levelPath = pLevelName;

        if (!QFile::exists(levelPath))
        {
            // if the input path can't be found, let's automatically add on the game folder and the levels
            QString levelsDir = QString("%1/%2").arg(Path::GetEditingGameDataFolder().c_str()).arg("Levels");

            // now let's check if they pre-pended directories (Samples/SomeLevelName)
            QString levelFileName = levelPath;
            QStringList splitLevelPath = levelPath.contains('/') ? levelPath.split('/') : levelPath.split('\\');
            if (splitLevelPath.length() > 1)
            {
                // take the last one as the level directory name and the level file name in that directory
                levelFileName = splitLevelPath.last();
            }

            levelPath = levelsDir / levelPath / levelFileName;

            // make sure the level path includes the cry extension, if needed
            if (!levelFileName.endsWith(oldExtension) && !levelFileName.endsWith(defaultExtension))
            {
                QString newLevelPath = levelPath + defaultExtension;
                QString oldLevelPath = levelPath + oldExtension;

                // Check if there is a .cry file, otherwise assume it is a new .ly file
                if (QFileInfo(oldLevelPath).exists())
                {
                    levelPath = oldLevelPath;
                }
                else
                {
                    levelPath = newLevelPath;
                }
            }

            if (!QFile::exists(levelPath))
            {
                return false;
            }
        }
        const bool addToMostRecentFileList = false;
        auto newDocument = CCryEditApp::instance()->OpenDocumentFile(levelPath.toUtf8().data(),
            addToMostRecentFileList, COpenSameLevelOptions::ReopenLevelIfSame);

        return newDocument != nullptr && !newDocument->IsLevelLoadFailed();
    }

    bool PyOpenLevelNoPrompt(const char* pLevelName)
    {
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        return PyOpenLevel(pLevelName);
    }

    bool PyReloadCurrentLevel()
    {
        if (GetIEditor()->IsLevelLoaded())
        {
            // Close the current level so that the subsequent call to open the same level will be allowed
            QString currentLevelPath = GetIEditor()->GetDocument()->GetLevelPathName();
            CloseCurrentLevel();

            return PyOpenLevel(currentLevelPath.toUtf8().constData());
        }

        return false;
    }

    int PyCreateLevel(const char* levelName, [[maybe_unused]] int resolution, [[maybe_unused]] int unitSize, [[maybe_unused]] bool bUseTerrain)
    {
        QString qualifiedName;
        return CCryEditApp::instance()->CreateLevel(levelName, qualifiedName);
    }

    int PyCreateLevelNoPrompt(const char* levelName, [[maybe_unused]] int heightmapResolution, [[maybe_unused]] int heightmapUnitSize,
        [[maybe_unused]] int terrainExportTextureSize, [[maybe_unused]] bool useTerrain)
    {
        // If a level was open, ignore any unsaved changes if it had been modified
        if (GetIEditor()->IsLevelLoaded())
        {
            GetIEditor()->GetDocument()->SetModifiedFlag(false);
        }

        QString qualifiedName;
        return CCryEditApp::instance()->CreateLevel(levelName, qualifiedName);
    }

    const char* PyGetCurrentLevelName()
    {
        // Using static member to capture temporary data
        static AZ::IO::FixedMaxPathString tempLevelName;
        tempLevelName = GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data();
        return tempLevelName.c_str();
    }

    const char* PyGetCurrentLevelPath()
    {
        // Using static member to capture temporary data
        static AZ::IO::FixedMaxPathString tempLevelPath;
        tempLevelPath = GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data();
        return tempLevelPath.c_str();
    }

    void Command_LoadPlugins()
    {
        GetIEditor()->LoadPlugins();
    }

    AZ::Vector3 PyGetCurrentViewPosition()
    {
        auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
        if (viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetDefaultViewportContext();
            AZ::Transform transform = viewportContext->GetCameraTransform();
            return transform.GetTranslation();
        }
        return AZ::Vector3();
    }

    AZ::Vector3 PyGetCurrentViewRotation()
    {
        auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
        if (viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetDefaultViewportContext();
            AZ::Transform transform = viewportContext->GetCameraTransform();
            return transform.GetRotation().GetEulerDegrees();
        }
        return AZ::Vector3();
    }

    void PySetCurrentViewPosition(float x, float y, float z)
    {
        auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
        if (viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetDefaultViewportContext();
            AZ::Transform transform = viewportContext->GetCameraTransform();
            transform.SetTranslation(x, y, z);
            viewportContextRequests->GetDefaultViewportContext()->SetCameraTransform(transform);
        }
    }

    void PySetCurrentViewRotation(float x, [[maybe_unused]] float y, float z)
    {
        auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
        if (viewportContextRequests)
        {
            AZ::RPI::ViewportContextPtr viewportContext = viewportContextRequests->GetDefaultViewportContext();
            AZ::Transform transform = viewportContext->GetCameraTransform();
            transform.SetRotation(AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(x, y, z)));
            viewportContextRequests->GetDefaultViewportContext()->SetCameraTransform(transform);
        }
    }
}

namespace
{
    void PyStartProcessDetached(const char* process, const char* args)
    {
        CCryEditApp::instance()->StartProcessDetached(process, args);
    }

    void PyLaunchLUAEditor(const char* files)
    {
        CCryEditApp::instance()->OpenLUAEditor(files);
    }

    bool PyCheckOutDialogEnableForAll(bool isEnable)
    {
        return CCheckOutDialog::EnableForAll(isEnable);
    }
}

namespace
{
    bool g_runScriptResult = false; // true -> success, false -> failure
}

namespace
{
    void PySetResultToSuccess()
    {
        g_runScriptResult = true;
    }

    void PySetResultToFailure()
    {
        g_runScriptResult = false;
    }

    void PyIdleEnable(bool enable)
    {
        if (Editor::EditorQtApplication::instance())
        {
            Editor::EditorQtApplication::instance()->EnableOnIdle(enable);
        }
    }

    bool PyIdleIsEnabled()
    {
        if (!Editor::EditorQtApplication::instance())
        {
            return false;
        }
        return Editor::EditorQtApplication::instance()->OnIdleEnabled();
    }

    void PyIdleWait(double timeInSec)
    {
        const bool wasIdleEnabled = PyIdleIsEnabled();
        if (!wasIdleEnabled)
        {
            PyIdleEnable(true);
        }

        clock_t start = clock();
        do
        {
            QEventLoop loop;
            QTimer::singleShot(timeInSec * 1000, &loop, &QEventLoop::quit);
            loop.exec();
        } while ((double)(clock() - start) / CLOCKS_PER_SEC < timeInSec);

        if (!wasIdleEnabled)
        {
            PyIdleEnable(false);
        }
    }

    void PyIdleWaitFrames(uint32 frames)
    {
        struct Ticker : public AZ::TickBus::Handler
        {
            Ticker(QEventLoop* loop, uint32 targetFrames) : m_loop(loop), m_targetFrames(targetFrames)
            {
                AZ::TickBus::Handler::BusConnect();
            }
            ~Ticker() override
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override
            {
                AZ_UNUSED(deltaTime);
                AZ_UNUSED(time);
                if (++m_elapsedFrames >= m_targetFrames)
                {
                    m_loop->quit();
                }
            }
            QEventLoop* m_loop = nullptr;
            uint32 m_elapsedFrames = 0;
            uint32 m_targetFrames = 0;
        };

        QEventLoop loop;
        Ticker ticker(&loop, frames);
        loop.exec();
    }
}


inline namespace Commands
{
    void PySetConfigSpec(int spec, int platform)
    {
        CUndo undo("Set Config Spec");
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoConficSpec());
        }
        GetIEditor()->SetEditorConfigSpec((ESystemConfigSpec)spec, (ESystemConfigPlatform)platform);
    }

    int PyGetConfigSpec()
    {
        return static_cast<int>(GetIEditor()->GetEditorConfigSpec());
    }

    int PyGetConfigPlatform()
    {
        return static_cast<int>(GetIEditor()->GetEditorConfigPlatform());
    }

    bool PyAttachDebugger()
    {
        return AZ::Debug::Trace::AttachDebugger();
    }

    bool PyWaitForDebugger(float timeoutSeconds = -1.f)
    {
        return AZ::Debug::Trace::WaitForDebugger(timeoutSeconds);
    }

    AZStd::string PyGetFileAlias(AZStd::string alias)
    {
        return AZ::IO::FileIOBase::GetInstance()->GetAlias(alias.c_str());
    }
}

namespace AzToolsFramework
{
    void CryEditPythonHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("open_level", ::PyOpenLevel, nullptr, "Opens a level."));
            addLegacyGeneral(behaviorContext->Method("open_level_no_prompt", ::PyOpenLevelNoPrompt, nullptr, "Opens a level. Doesn't prompt user about saving a modified level."));
            addLegacyGeneral(behaviorContext->Method("reload_current_level", ::PyReloadCurrentLevel, nullptr, "Re-loads the current level. If no level is loaded, then does nothing."));
            addLegacyGeneral(behaviorContext->Method("create_level", ::PyCreateLevel, nullptr, "Creates a level with the parameters of 'levelName', 'resolution', 'unitSize' and 'bUseTerrain'."));
            addLegacyGeneral(behaviorContext->Method("create_level_no_prompt", ::PyCreateLevelNoPrompt, nullptr, "Creates a level with the parameters of 'levelName', 'resolution', 'unitSize' and 'bUseTerrain'."));
            addLegacyGeneral(behaviorContext->Method("get_game_folder", PyGetGameFolderAsString, nullptr, "Gets the path to the Game folder of current project."));
            addLegacyGeneral(behaviorContext->Method("get_current_level_name", PyGetCurrentLevelName, nullptr, "Gets the name of the current level."));
            addLegacyGeneral(behaviorContext->Method("get_current_level_path", PyGetCurrentLevelPath, nullptr, "Gets the fully specified path of the current level."));

            addLegacyGeneral(behaviorContext->Method("load_all_plugins", ::Command_LoadPlugins, nullptr, "Loads all available plugins."));
            addLegacyGeneral(behaviorContext->Method("get_current_view_position", PyGetCurrentViewPosition, nullptr, "Returns the position of the current view as a Vec3."));
            addLegacyGeneral(behaviorContext->Method("get_current_view_rotation", PyGetCurrentViewRotation, nullptr, "Returns the rotation of the current view as a Vec3 of Euler angles in degrees."));
            addLegacyGeneral(behaviorContext->Method("set_current_view_position", PySetCurrentViewPosition, nullptr, "Sets the position of the current view as given x, y, z coordinates."));
            addLegacyGeneral(behaviorContext->Method("set_current_view_rotation", PySetCurrentViewRotation, nullptr, "Sets the rotation of the current view as given x, y, z Euler angles in degrees."));

            addLegacyGeneral(behaviorContext->Method("export_to_engine", CCryEditApp::Command_ExportToEngine, nullptr, "Exports the current level to the engine."));
            addLegacyGeneral(behaviorContext->Method("set_config_spec", PySetConfigSpec, nullptr, "Sets the system config spec and platform."));
            addLegacyGeneral(behaviorContext->Method("get_config_platform", PyGetConfigPlatform, nullptr, "Gets the system config platform."));
            addLegacyGeneral(behaviorContext->Method("get_config_spec", PyGetConfigSpec, nullptr, "Gets the system config spec."));

            addLegacyGeneral(behaviorContext->Method("set_result_to_success", PySetResultToSuccess, nullptr, "Sets the result of a script execution to success. Used only for Sandbox AutoTests."));
            addLegacyGeneral(behaviorContext->Method("set_result_to_failure", PySetResultToFailure, nullptr, "Sets the result of a script execution to failure. Used only for Sandbox AutoTests."));

            addLegacyGeneral(behaviorContext->Method("idle_enable", PyIdleEnable, nullptr, "Enables/Disables idle processing for the Editor. Primarily used for auto-testing."));
            addLegacyGeneral(behaviorContext->Method("is_idle_enabled", PyIdleIsEnabled, nullptr, "Returns whether or not idle processing is enabled for the Editor. Primarily used for auto-testing."));
            addLegacyGeneral(behaviorContext->Method("idle_is_enabled", PyIdleIsEnabled, nullptr, "Returns whether or not idle processing is enabled for the Editor. Primarily used for auto-testing."));
            addLegacyGeneral(behaviorContext->Method("idle_wait", PyIdleWait, nullptr, "Waits idling for a given seconds. Primarily used for auto-testing."));
            addLegacyGeneral(behaviorContext->Method("idle_wait_frames", PyIdleWaitFrames, nullptr, "Waits idling for a frames. Primarily used for auto-testing."));

            addLegacyGeneral(behaviorContext->Method("start_process_detached", PyStartProcessDetached, nullptr, "Launches a detached process with an optional space separated list of arguments."));
            addLegacyGeneral(behaviorContext->Method("launch_lua_editor", PyLaunchLUAEditor, nullptr, "Launches the Lua editor, may receive a list of space separate file paths, or an empty string to only open the editor."));

            addLegacyGeneral(behaviorContext->Method("attach_debugger", PyAttachDebugger, nullptr, "Prompts for attaching the debugger"));
            addLegacyGeneral(behaviorContext->Method("wait_for_debugger", PyWaitForDebugger, behaviorContext->MakeDefaultValues(-1.f), "Pauses this thread execution until the debugger has been attached"));

            addLegacyGeneral(behaviorContext->Method("get_file_alias", PyGetFileAlias, nullptr, "Retrieves path for IO alias"));

            // this will put these methods into the 'azlmbr.legacy.checkout_dialog' module
            auto addCheckoutDialog = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/CheckoutDialog")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.checkout_dialog");
            };
            addCheckoutDialog(behaviorContext->Method("enable_for_all", PyCheckOutDialogEnableForAll, nullptr, "Enables the 'Apply to all' button in the checkout dialog; useful for allowing the user to apply a decision to check out files to multiple, related operations."));

            behaviorContext->EnumProperty<ESystemConfigSpec::CONFIG_AUTO_SPEC>("SystemConfigSpec_Auto")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigSpec::CONFIG_LOW_SPEC>("SystemConfigSpec_Low")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigSpec::CONFIG_MEDIUM_SPEC>("SystemConfigSpec_Medium")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigSpec::CONFIG_HIGH_SPEC>("SystemConfigSpec_High")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigSpec::CONFIG_VERYHIGH_SPEC>("SystemConfigSpec_VeryHigh")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_INVALID_PLATFORM>("SystemConfigPlatform_InvalidPlatform")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_PC>("SystemConfigPlatform_Pc")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_MAC>("SystemConfigPlatform_Mac")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_OSX_METAL>("SystemConfigPlatform_OsxMetal")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_ANDROID>("SystemConfigPlatform_Android")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_IOS>("SystemConfigPlatform_Ios")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<ESystemConfigPlatform::CONFIG_PROVO>("SystemConfigPlatform_Provo")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

}
