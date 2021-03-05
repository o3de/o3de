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
#ifndef _GEM_GAMEEFFECTSYSTEM_H_
#define _GEM_GAMEEFFECTSYSTEM_H_

#include "GameEffectsSystem.h"

class GameEffectSystemGem
    : public CryHooksModule
    , public GameEffectSystemRequestBus::Handler
{
public:
    AZ_RTTI(GameEffectSystemGem, "{44350C39-A90B-46EB-AC1C-DB505113F4A6}", CryHooksModule);

public:
    GameEffectSystemGem();
    ~GameEffectSystemGem() override;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    IGameEffectSystem* GetIGameEffectSystem() override;

private:
    CGameEffectsSystem* m_gameEffectSystem;
    int g_gameFXSystemDebug;
};

#endif//_GEM_GAMEEFFECTSYSTEM_H_
