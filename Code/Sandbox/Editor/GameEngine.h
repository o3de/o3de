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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : The game engine for editor


#ifndef CRYINCLUDE_EDITOR_GAMEENGINE_H
#define CRYINCLUDE_EDITOR_GAMEENGINE_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Outcome/Outcome.h>
#include "LogFile.h"
#include "CryListenerSet.h"
#include "Util/ModalWindowDismisser.h"
#endif

class CStartupLogoDialog;
struct IInitializeUIInfo;

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>

namespace AzToolsFramework
{
    //! Operates the Editor's camera
    class IEditorCameraController
    {
    public:
        AZ_RTTI(IEditorCameraController, "{AEF60D3E-10A1-4161-9379-F68C69A5959C}");

        IEditorCameraController() = default;
        IEditorCameraController(IEditorCameraController&&) = delete;
        IEditorCameraController& operator=(IEditorCameraController&&) = delete;
        virtual ~IEditorCameraController() = default;

        virtual void SetCurrentViewPosition([[maybe_unused]] const AZ::Vector3& position) {}
        virtual void SetCurrentViewRotation([[maybe_unused]] const AZ::Vector3& rotation) {}
    };
}

class ThreadedOnErrorHandler : public QObject
{
    Q_OBJECT
public:
    explicit ThreadedOnErrorHandler(ISystemUserCallback* callback);
    ~ThreadedOnErrorHandler();

public slots:
    bool OnError(const char* error);

private:
    ISystemUserCallback* m_userCallback;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
//! This class serves as a high-level wrapper for CryEngine game.
class SANDBOX_API CGameEngine
    : public IEditorNotifyListener
    , private AzToolsFramework::IEditorCameraController
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CGameEngine();
    ~CGameEngine(void);
    //! Initialize System.
    //! @return successful outcome if initialization succeeded. or failed outcome with error message.
    AZ::Outcome<void, AZStd::string> Init(
        bool bPreviewMode,
        bool bTestMode,
        bool bShaderCacheGen,
        const char* sCmdLine,
        IInitializeUIInfo* logo,
        HWND hwndForInputSystem);
    //! Initialize game.
    //! @return true if initialization succeeded, false otherwise
    bool InitGame(const char* sGameDLL);
    //! Load new terrain level into 3d engine.
    //! Also load AI triangulation for this level.
    bool LoadLevel(
        bool bDeleteAIGraph,
        bool bReleaseResources);
    //!* Reload level if it was already loaded.
    bool ReloadLevel();
    //! Request to switch In/Out of game mode on next update.
    //! The switch will happen when no sub systems are currently being updated.
    //! @param inGame When true editor switch to game mode.
    void RequestSetGameMode(bool inGame);
    //! Switch In/Out of AI and Physics simulation mode.
    //! @param enabled When true editor switch to simulation mode.
    void SetSimulationMode(bool enabled, bool bOnlyPhysics = false);
    //! Get current simulation mode.
    bool GetSimulationMode() const { return m_bSimulationMode; };
    //! Returns true if level is loaded.
    bool IsLevelLoaded() const { return m_bLevelLoaded; };
    //! Assign new level path name.
    void SetLevelPath(const QString& path);
    //! Return name of currently loaded level.
    const QString& GetLevelName() const { return m_levelName; };
    //! Return extension of currently loaded level.
    const QString& GetLevelExtension() const { return m_levelExtension; };
    //! Get fully specified level path.
    const QString& GetLevelPath() const { return m_levelPath; };
    //! Query if engine is in game mode.
    bool IsInGameMode() const { return m_bInGameMode; };
    //! Force level loaded variable to true.
    void SetLevelLoaded(bool bLoaded) { m_bLevelLoaded = bLoaded; }
    //! Force level just created variable to true.
    void SetLevelCreated(bool bJustCreated) { m_bJustCreated = bJustCreated; }
    //! Query ISystem interface.
    ISystem* GetSystem() { return m_pISystem; };
    //! Set player position in game.
    //! @param bEyePos If set then given position is position of player eyes.
    void SetPlayerViewMatrix(const Matrix34& tm, bool bEyePos = true);
    //! When set, player in game will be every frame synchronized with editor camera.
    void SyncPlayerPosition(bool bEnable);
    bool IsSyncPlayerPosition() const { return m_bSyncPlayerPosition; };
    //! Set game's current Mod name.
    void SetCurrentMOD(const char* sMod);
    //! Returns game's current Mod name.
    QString GetCurrentMOD() const;
    //! Called every frame.
    void Update();
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    void OnTerrainModified(const Vec2& modPosition, float modAreaRadius, bool fullTerrain);
    void OnAreaModified(const AABB& modifiedArea);

    void ExecuteQueuedEvents();

    //! mutex used by other threads to lock up the PAK modification,
    //! so only one thread can modify the PAK at once
    static CryMutex& GetPakModifyMutex()
    {
        //! mutex used to halt copy process while the export to game
        //! or other pak operation is done in the main thread
        static CryMutex s_pakModifyMutex;
        return s_pakModifyMutex;
    }

    inline HMODULE GetGameModule() const
    {
        return m_gameDll;
    }

private:
    AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
    void SetCurrentViewPosition(const AZ::Vector3& position) override;
    void SetCurrentViewRotation(const AZ::Vector3& rotation) override;
    AZ_POP_DISABLE_OVERRIDE_WARNING

    void SetGameMode(bool inGame);
    void SwitchToInGame();
    void SwitchToInEditor();
    static void HandleQuitRequest(IConsoleCmdArgs*);

    CLogFile m_logFile;
    QString m_levelName;
    QString m_levelExtension;
    QString m_levelPath;
    QString m_MOD;
    bool m_bLevelLoaded;
    bool m_bInGameMode;
    bool m_bSimulationMode;
    bool m_bSyncPlayerPosition;
    bool m_bJustCreated;
    bool m_bIgnoreUpdates;
    ISystem* m_pISystem;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Matrix34 m_playerViewTM;
    struct SSystemUserCallback* m_pSystemUserCallback;
    HMODULE m_hSystemHandle;
    HMODULE m_gameDll;
    enum EPendingGameMode
    {
        ePGM_NotPending,
        ePGM_SwitchToInGame,
        ePGM_SwitchToInEditor,
    };
    EPendingGameMode m_ePendingGameMode;
    AZStd::unique_ptr<class ModalWindowDismisser> m_modalWindowDismisser;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


#endif // CRYINCLUDE_EDITOR_GAMEENGINE_H
