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
#ifndef _EFFECTS_GAMEEFFECTSSYSTEM_H_
#define _EFFECTS_GAMEEFFECTSSYSTEM_H_
#pragma once

// Includes
#include "GameEffectSystem/IGameEffectSystem.h"
#include "GameEffectSystem/GameEffectsSystemDefines.h"
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

//==================================================================================================
// Name: CGameEffectsSystem
// Desc: System to handle game effects, game render nodes and game render elements
//           Game effect: separates out effect logic from game logic
//           Game render node: handles the render object in 3d space
//           Game render element: handles the rendering of the object
//           CVar activation system: system used to have data driven cvars activated in game
//effects
//           Post effect activation system: system used to have data driven post effects
//activated in game effects
// Author: James Chilvers
//==================================================================================================
class CGameEffectsSystem
    : public IGameEffectSystem
    , public AzFramework::InputChannelEventListener
    , public ISoftCodeListener
    , public AZ::TickBus::Handler
{
    friend struct SGameEffectSystemStaticData;

public:
    void Destroy();
    void Initialize();
    void LoadData();
    void ReleaseData();

    template <class T>
    T* CreateEffect() // Use if dynamic memory allocation is required for the game effect
    { // Using this function then allows easy changing of memory allocator for all dynamically
      // created effects
        T* newEffect = new T;
        return newEffect;
    }

    // Each effect automatically registers and unregisters itself
    SC_API void RegisterEffect(IGameEffect* effect) override;
    SC_API void UnRegisterEffect(IGameEffect* effect) override;

    void Update(float frameTime);

    SC_API void RegisterGameRenderNode(IGameRenderNodePtr& pGameRenderNode);
    SC_API void UnregisterGameRenderNode(IGameRenderNodePtr& pGameRenderNode);

    SC_API void RegisterGameRenderElement(IGameRenderElementPtr& pGameRenderElement);
    SC_API void UnregisterGameRenderElement(IGameRenderElementPtr& pGameRenderElement);

#ifdef SOFTCODE_ENABLED
    SC_API void*
    CreateSoftCodeInstance(const char* pTypeName) override; // Create soft code instance using libs
    SC_API void
    RegisterSoftCodeLib(ITypeLibrary* pLib) override; // Register soft code lib for creation of instances
#endif

    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

    // ISoftCodeListener implementation
    void InstanceReplaced(void* pOldInstance, void* pNewInstance) override;
    void GameRenderNodeInstanceReplaced(void* pOldInstance, void* pNewInstance) override;
    void GameRenderElementInstanceReplaced(void* pOldInstance, void* pNewInstance) override;

    // SGameRulesListener implementation
    void GameRulesInitialise();
    // #TODO: Have this receive events from GameRules
    void EnteredGame();// override;

    void ReloadData();

    CGameEffectsSystem();
    virtual ~CGameEffectsSystem();

    // AZ::TickBus::Handler implementation
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

private:
    void Reset();
    void AutoReleaseAndDeleteFlaggedEffects(IGameEffect* effectList);
    void AutoDeleteEffects(IGameEffect* effectList);
    void SetPostEffectCVarCallbacks();
    static void PostEffectCVarCallback(ICVar* cvar);

#if DEBUG_GAME_FX_SYSTEM
    void DrawDebugDisplay();
    void OnActivateDebugView(int debugView);
    void OnDeActivateDebugView(int debugView);

    int GetDebugView() const { return m_debugView; }
    SC_API IGameEffect* GetDebugEffect(const char* pEffectName) const;

    static int s_currentDebugEffectId;

    int m_debugView;
#endif

    static int s_postEffectCVarNameOffset;

#ifdef SOFTCODE_ENABLED
    std::vector<ITypeLibrary*> m_softCodeTypeLibs;

    typedef std::vector<IGameRenderNodePtr*> TGameRenderNodeVec;
    TGameRenderNodeVec m_gameRenderNodes;
    CGameRenderNodeSoftCodeListener* m_gameRenderNodeSoftCodeListener;

    typedef std::vector<IGameRenderElementPtr*> TGameRenderElementVec;
    TGameRenderElementVec m_gameRenderElements;
    CGameRenderElementSoftCodeListener* m_gameRenderElementSoftCodeListener;
#endif

    IGameEffect* m_effectsToUpdate;
    IGameEffect* m_effectsNotToUpdate;
    // If in update loop, this is the next effect to be updated this will get changed if the effect is unregistered
    IGameEffect* m_nextEffectToUpdate;
    bool m_isInitialised;
    bool s_hasLoadedData;
}; //------------------------------------------------------------------------------------------------

#endif//_EFFECTS_GAMEEFFECTSSYSTEM_H_
