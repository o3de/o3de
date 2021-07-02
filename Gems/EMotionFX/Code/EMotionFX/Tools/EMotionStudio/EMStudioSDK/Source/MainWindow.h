/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionStudio/EMStudioSDK/Source/EMStudioConfig.h>
#include <EMotionStudio/EMStudioSDK/Source/GUIOptions.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginOptionsBus.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/CommandManagerCallback.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <MCore/Source/CommandLine.h>
#include <MysticQt/Source/RecentFiles.h>
#include <Editor/ActorEditorBus.h>

#include <AzQtComponents/Components/DockMainWindow.h>
#include <QDialog>
#endif

// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QDropEvent)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMessageBox)
QT_FORWARD_DECLARE_CLASS(QAbstractButton)
QT_FORWARD_DECLARE_CLASS(QTextEdit)

struct SelectionItem;

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace EMotionFX
{
    class AnimGraph;
    class MotionSet;
}

namespace MCore
{
    class CommandGroup;
}

namespace MysticQt
{
    class KeyboardShortcutManager;
}

namespace AzQtComponents
{
    class FancyDocking;
}

namespace EMStudio
{
    // forward declaration
    class DirtyFileManager;
    class EMStudioPlugin;
    class FileManager;
    class MainWindow;
    class NativeEventFilter;
    class NodeSelectionWindow;
    class PreferencesWindow;
    class UndoMenuCallback;


    class ErrorWindow
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(ErrorWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        explicit ErrorWindow(QWidget* parent = nullptr);
        void Init(const AZStd::vector<AZStd::string>& errors);
        
    private:
        QTextEdit* mTextEdit = nullptr;
    };

    // the main window
    class EMSTUDIO_API MainWindow
        : public AzQtComponents::DockMainWindow
        , private PluginOptionsNotificationsBus::Router
        , public EMotionFX::ActorEditorRequestBus::Handler
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MainWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        ~MainWindow();

        void UpdateCreateWindowMenu();
        void UpdateLayoutsMenu();
        void UpdateUndoRedo();
        void DisableUndoRedo();
        static void Reflect(AZ::ReflectContext* context);
        void Init();

        MCORE_INLINE QMenu* GetLayoutsMenu()                                    { return mLayoutsMenu; }

        void LoadActor(const char* fileName, bool replaceCurrentScene);
        void LoadCharacter(const AZ::Data::AssetId& actorAssetId, const AZ::Data::AssetId& animgraphId, const AZ::Data::AssetId& motionSetId);
        void LoadFile(const AZStd::string& fileName, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);
        void LoadFiles(const AZStd::vector<AZStd::string>& filenames, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);

        void Activate(const AZ::Data::AssetId& actorAssetId, const EMotionFX::AnimGraph* animGraph, const EMotionFX::MotionSet* motionSet);

        MysticQt::RecentFiles* GetRecentWorkspaces()                            { return &mRecentWorkspaces; }

        GUIOptions& GetOptions()                                                { return mOptions; }

        void Reset(bool clearActors = true, bool clearMotionSets = true, bool clearMotions = true, bool clearAnimGraphs = true, MCore::CommandGroup* commandGroup = nullptr);

        // settings
        void SavePreferences();
        void LoadPreferences();

        void UpdateResetAndSaveAllMenus();

        void UpdateSaveActorsMenu();
        void EnableMergeActorMenu();
        void DisableMergeActorMenu();
        void EnableSaveSelectedActorsMenu();
        void DisableSaveSelectedActorsMenu();

        void OnWorkspaceSaved(const char* filename);

        MCORE_INLINE QComboBox* GetApplicationModeComboBox()                    { return mApplicationMode; }
        DirtyFileManager*   GetDirtyFileManager() const                         { return mDirtyFileManager; }
        FileManager*        GetFileManager() const                              { return mFileManager; }
        PreferencesWindow*  GetPreferencesWindow() const                        { return mPreferencesWindow; }

        uint32 GetNumLayouts() const                                            { return mLayoutNames.GetLength(); }
        const char* GetLayoutName(uint32 index) const                           { return mLayoutNames[index].c_str(); }
        const char* GetCurrentLayoutName() const;

        static const char* GetEMotionFXPaneName();
        MysticQt::KeyboardShortcutManager* GetShortcutManager() const           { return mShortcutManager; }

        AzQtComponents::FancyDocking* GetFancyDockingManager() const            { return m_fancyDockingManager; }

        QMessageBox* GetRemoveLayoutDialog();

        void AddRecentActorFile(const QString& fileName);

        void LoadKeyboardShortcuts();

    public slots:
        void OnAutosaveTimeOut();
        void LoadLayoutAfterShow();
        void RaiseFloatingWidgets();
        void LoadCharacterFiles();

        void OnSaveLayoutDialogAccept();
        void OnSaveLayoutDialogReject();

    protected:
        void moveEvent(QMoveEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void LoadDefaultLayout();

    private:
        // ActorEditorRequests
        EMotionFX::ActorInstance* GetSelectedActorInstance() override;
        EMotionFX::Actor* GetSelectedActor() override;

        void BroadcastSelectionNotifications();
        EMotionFX::Actor*           m_prevSelectedActor;
        EMotionFX::ActorInstance*   m_prevSelectedActorInstance;

        QMenu*                  mCreateWindowMenu;
        QMenu*                  mLayoutsMenu;
        QAction*                m_undoAction;
        QAction*                m_redoAction;

        // keyboard shortcut manager
        MysticQt::KeyboardShortcutManager* mShortcutManager;

        // layouts (application modes)
        MCore::Array<AZStd::string> mLayoutNames;
        bool mLayoutLoaded;

        // menu actions
        QAction*                mResetAction;
        QAction*                mSaveAllAction;
        QAction*                mMergeActorAction;
        QAction*                mSaveSelectedActorsAction;
#ifdef EMFX_DEVELOPMENT_BUILD
        QAction*                mSaveSelectedActorAsAttachmentsAction;
#endif

        // application mode
        QComboBox*              mApplicationMode;

        PreferencesWindow*      mPreferencesWindow;

        FileManager*  mFileManager;

        MysticQt::RecentFiles   mRecentActors;
        MysticQt::RecentFiles   mRecentWorkspaces;

        // dirty files
        DirtyFileManager*       mDirtyFileManager;

        void SetWindowTitleFromFileName(const AZStd::string& fileName);

        // drag & drop support
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        AZStd::string           mDroppedActorFileName;

        // General options
        GUIOptions                              mOptions;
        bool                                    mLoadingOptions;
        
        QTimer*                                 mAutosaveTimer;
        
        AZStd::vector<AZStd::string>            mCharacterFiles;

        NativeEventFilter*                      mNativeEventFilter;

        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;

        void OnOptionChanged(const AZStd::string& optionChanged) override;

        UndoMenuCallback*                       m_undoMenuCallback = nullptr;

        AzQtComponents::FancyDocking*           m_fancyDockingManager = nullptr;

        QMessageBox*                            m_reallyRemoveLayoutDialog = nullptr;
        QString                                 m_removeLayoutNameText;
        QString                                 m_layoutFileBeingRemoved;

        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandImportMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveWorkspaceCallback);
        CommandImportActorCallback*         mImportActorCallback;
        CommandRemoveActorCallback*         mRemoveActorCallback;
        CommandRemoveActorInstanceCallback* mRemoveActorInstanceCallback;
        CommandImportMotionCallback*        mImportMotionCallback;
        CommandRemoveMotionCallback*        mRemoveMotionCallback;
        CommandCreateMotionSetCallback*     mCreateMotionSetCallback;
        CommandRemoveMotionSetCallback*     mRemoveMotionSetCallback;
        CommandLoadMotionSetCallback*       mLoadMotionSetCallback;
        CommandCreateAnimGraphCallback*     mCreateAnimGraphCallback;
        CommandRemoveAnimGraphCallback*     mRemoveAnimGraphCallback;
        CommandLoadAnimGraphCallback*       mLoadAnimGraphCallback;
        CommandSelectCallback*              mSelectCallback;
        CommandUnselectCallback*            mUnselectCallback;
        CommandClearSelectionCallback*      m_clearSelectionCallback;
        CommandSaveWorkspaceCallback*       mSaveWorkspaceCallback;

        class MainWindowCommandManagerCallback : public MCore::CommandManagerCallback
        {
        public:
            MainWindowCommandManagerCallback();

            bool NeedToClearRecorder(MCore::Command* command, const MCore::CommandLine& commandLine) const;

            //////////////////////////////////////////////////////////////////////////////////////
            /// CommandManagerCallback implementation
            void OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override;
            void OnPostExecuteCommand(MCore::CommandGroup* /*group*/, MCore::Command* /*command*/, const MCore::CommandLine& /*commandLine*/, bool /*wasSuccess*/, const AZStd::string& /*outResult*/) override { }
            void OnPreUndoCommand(MCore::Command* command, const MCore::CommandLine& commandLine);
            void OnPreExecuteCommandGroup(MCore::CommandGroup* /*group*/, bool /*undo*/) override { }
            void OnPostExecuteCommandGroup(MCore::CommandGroup* /*group*/, bool /*wasSuccess*/) override { }
            void OnAddCommandToHistory(uint32 /*historyIndex*/, MCore::CommandGroup* /*group*/, MCore::Command* /*command*/, const MCore::CommandLine& /*commandLine*/) override { }
            void OnRemoveCommand(uint32 /*historyIndex*/) override { }
            void OnSetCurrentCommand(uint32 /*index*/) override { }
            void OnShowErrorReport(const AZStd::vector<AZStd::string>& errors) override;
        private:
            AZStd::vector<AZStd::string> m_skipClearRecorderCommands;
            AZStd::unique_ptr<ErrorWindow> m_errorWindow = nullptr;
        };

        MainWindowCommandManagerCallback m_mainWindowCommandManagerCallback;

    public slots:
        void OnFileOpenActor();
        void OnFileSaveSelectedActors();
        void OnReset();
        void OnFileMergeActor();
        void OnOpenDroppedActor();
        void OnRecentFile(QAction* action);
        void OnMergeDroppedActor();
        void OnFileNewWorkspace();
        void OnFileOpenWorkspace();
        void OnFileSaveWorkspace();
        void OnFileSaveWorkspaceAs();
        void OnWindowCreate(bool checked);
        void OnLayoutSaveAs();
        void OnRemoveLayout();
        void OnLoadLayout();
        void OnUndo();
        void OnRedo();
        void OnOpenAutosaveFolder();
        void OnOpenSettingsFolder();
        void OnPreferences();
        void OnSaveAll();
        void ApplicationModeChanged(int index);
        void ApplicationModeChanged(const QString& text);
        void OnUpdateRenderPlugins();
        void OnRemoveLayoutButtonClicked(QAbstractButton* button);

     signals:
        void HardwareChangeDetected();
    };

} // namespace EMStudio
