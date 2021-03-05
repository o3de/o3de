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
#include "GameEffectSystem_precompiled.h"
#include "GameEffectSystemGem.h"

namespace
{
    /**
     * Console command function for reloading the game effect system's data.
     */
    void CmdReloadGameFx([[maybe_unused]] IConsoleCmdArgs* pArgs)
    {
        IGameEffectSystem* iGameEffectSystem = nullptr;
        GameEffectSystemRequestBus::BroadcastResult(iGameEffectSystem, &GameEffectSystemRequestBus::Events::GetIGameEffectSystem);
        if (iGameEffectSystem)
        {
            CGameEffectsSystem* cGameEffectsSystem = reinterpret_cast<CGameEffectsSystem*>(iGameEffectSystem);
            cGameEffectsSystem->ReloadData();
        }
    }
}

GameEffectSystemGem::GameEffectSystemGem()
    : CryHooksModule()
    , m_gameEffectSystem(nullptr)
    , g_gameFXSystemDebug(0)
{
    GameEffectSystemRequestBus::Handler::BusConnect();
}

GameEffectSystemGem::~GameEffectSystemGem()
{
    GameEffectSystemRequestBus::Handler::BusDisconnect();
}

void GameEffectSystemGem::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
        // Put your init code here
        // All other Gems will exist at this point
        REGISTER_CVAR(g_gameFXSystemDebug, 0, 0, "Toggles game effects system debug state");
        REGISTER_COMMAND("g_reloadGameFx", &CmdReloadGameFx, 0, "Reload all game fx");

        m_gameEffectSystem = new CGameEffectsSystem();
        m_gameEffectSystem->Initialize();
        m_gameEffectSystem->LoadData();
        break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
        if (m_gameEffectSystem)
        {
            m_gameEffectSystem->ReleaseData();
            m_gameEffectSystem->Destroy();

            delete m_gameEffectSystem;
            m_gameEffectSystem = nullptr;
        }
        break;
    }
}

IGameEffectSystem* GameEffectSystemGem::GetIGameEffectSystem()
{
    return m_gameEffectSystem;
}

AZ_DECLARE_MODULE_CLASS(Gem_GameEffectSystem, GameEffectSystemGem)
